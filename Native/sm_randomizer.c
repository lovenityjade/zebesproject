#include "sm_randomizer.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Use Unreal's existing interpreter when present, otherwise load the runtime.
 * No subprocess, network service, or external launcher. A fresh subinterpreter
 * per request isolates VARIA's mutable module globals and random state. */
#ifdef _WIN32
static SRWLOCK lock = SRWLOCK_INIT;
#define LOCK() AcquireSRWLockExclusive(&lock)
#define UNLOCK() ReleaseSRWLockExclusive(&lock)
#define SYMBOL(name) ((void *)GetProcAddress((HMODULE)runtime, name))
#else
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() pthread_mutex_lock(&lock)
#define UNLOCK() pthread_mutex_unlock(&lock)
#define SYMBOL(name) dlsym(runtime, name)
#endif
static char error[256];
static void *runtime;
static int (*is_initialized)(void);
static void (*initialize)(void);
static void *(*save_thread)(void);
static int (*ensure_gil)(void);
static void (*release_gil)(int);
static void *(*thread_get)(void);
static void *(*thread_swap)(void *);
static void *(*new_interpreter)(void);
static void (*end_interpreter)(void *);
static int (*run_string)(const char *);
static void *(*add_module)(const char *);
static void *(*module_dict)(void *);
static void *(*dict_get)(void *, const char *);
static const char *(*as_utf8)(void *);
static long (*as_long)(void *);
static int ready;

static int prepare(void) {
  if (ready) return 1;
#ifdef _WIN32
  /* Reuse an interpreter already loaded by Unreal's editor or a test host.
   * The standalone game only loads the bundled interpreter, never one on PATH. */
  const wchar_t *names[] = {L"python314.dll", L"python313.dll", L"python312.dll", L"python311.dll", NULL};
  for (int i=0; names[i] && !runtime; ++i) runtime = GetModuleHandleW(names[i]);
  if (!runtime) {
    HMODULE self = NULL;
    wchar_t path[32768];
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)&runtime, &self)) return 0;
    DWORD length = GetModuleFileNameW(self, path, 32768);
    if (!length || length >= 32768) return 0;
    wchar_t *slash = wcsrchr(path, L'\\');
    const wchar_t *suffix = L"\\..\\..\\Runtime\\Python\\python313.dll";
    if (!slash || (size_t)(slash-path)+wcslen(suffix)+1 > 32768) return 0;
    wcscpy(slash, suffix);
    runtime = LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!runtime) { snprintf(error,sizeof(error),"Bundled Python runtime unavailable (Windows error %lu)",GetLastError()); return 0; }
  }
#else
  runtime = RTLD_DEFAULT;
  const char *bundled = getenv("SM_PYTHON_LIBRARY");
  if (bundled && *bundled) {
    // AppRun selects its bundled runtime explicitly. Never silently fall back
    // to a different system Python when a packaged installation is broken.
    if (*bundled != '/') { snprintf(error,sizeof(error),"Bundled Python path must be absolute"); return 0; }
    if (dlsym(RTLD_DEFAULT, "Py_IsInitialized")) {
      snprintf(error,sizeof(error),"Cannot select bundled Python in a host with Python already loaded"); return 0;
    }
    runtime = dlopen(bundled, RTLD_NOW | RTLD_GLOBAL);
    if (!runtime) { snprintf(error,sizeof(error),"Bundled Python runtime unavailable: %.190s",dlerror()); return 0; }
  } else if (!dlsym(runtime, "Py_IsInitialized")) {
    const char *libs[] = {"libpython3.14.so.1.0", "libpython3.13.so.1.0",
                        "libpython3.12.so.1.0", "libpython3.11.so.1.0", NULL};
    for (int i=0; libs[i] && !runtime; ++i)
      runtime = dlopen(libs[i], RTLD_NOW | RTLD_GLOBAL);
    if (!runtime) { snprintf(error,sizeof(error),"Python runtime unavailable"); return 0; }
  }
#endif
#define API(target, symbol) do { *(void **)(&target)=SYMBOL(symbol); \
  if (!target) { snprintf(error,sizeof(error),"Missing runtime API: %s",symbol); return 0; } } while (0)
  API(is_initialized,"Py_IsInitialized"); API(initialize,"Py_Initialize");
  API(save_thread,"PyEval_SaveThread"); API(ensure_gil,"PyGILState_Ensure");
  API(release_gil,"PyGILState_Release"); API(thread_get,"PyThreadState_Get");
  API(thread_swap,"PyThreadState_Swap"); API(new_interpreter,"Py_NewInterpreter");
  API(end_interpreter,"Py_EndInterpreter"); API(run_string,"PyRun_SimpleString");
  API(add_module,"PyImport_AddModule"); API(module_dict,"PyModule_GetDict");
  API(dict_get,"PyDict_GetItemString"); API(as_utf8,"PyUnicode_AsUTF8");
  API(as_long,"PyLong_AsLong");
#undef API
  if (!is_initialized()) { initialize(); save_thread(); }
  ready=1;
  return 1;
}
static char *hex(const char *input) {
  size_t n=strlen(input); char *out=malloc(n*2+1);
  if (!out) return NULL;
  for (size_t i=0;i<n;i++) { out[i*2]="0123456789abcdef"[(unsigned char)input[i]>>4]; out[i*2+1]="0123456789abcdef"[input[i]&15]; }
  out[n*2]=0; return out;
}
static int execute(const char *root,const char *request,char *result,int capacity,int service) {
  if (!root || !request || strlen(root)>4096 || strlen(request)>262144 || capacity<0) return 0;
  LOCK();
  error[0]=0; int count=0;
  if (!prepare()) goto done;
  char *rh=hex(root), *qh=hex(request);
  size_t size=(rh?strlen(rh):0)+(qh?strlen(qh):0)+640;
  char *script=malloc(size);
  if (!rh || !qh || !script) { free(rh);free(qh);free(script);goto done; }
  int gil=ensure_gil(); void *previous=thread_get();
  for(int attempt=0;attempt<8;attempt++) {
    snprintf(script,size,
      "import sys,json\n"
      // Unreal's exported OpenSSL symbols can interpose on system _hashlib
      // with an incompatible ABI. CPython's bundled HACL hash implementations
      // produce identical digests without loading the OpenSSL extension.
      "sys.modules['_hashlib']=None\n"
      "sys.path.insert(0,bytes.fromhex('%s').decode('utf-8'))\n"
      "from %s import %s as execute_json\n"
      "_sm_result=execute_json(bytes.fromhex('%s').decode('utf-8'),%d)\n"
      "_sm_retry=int(json.loads(_sm_result).get('retry',False))\n",rh,
      service==2?"sm_preflight":service==1?"sm_live_tracker":"sm_integration",
      service==2?"validate_json":service==1?"evaluate_json":"generate_json",qh,attempt);
    void *sub=new_interpreter(); int retry=0;
    if(sub) {
      if(!run_string(script)) {
        void *dict=module_dict(add_module("__main__"));
        void *value=dict_get(dict,"_sm_result");
        const char *json=value?as_utf8(value):NULL;
        void *retry_value=dict_get(dict,"_sm_retry");
        retry=retry_value?(int)as_long(retry_value):0;
        if(json) {count=(int)strlen(json)+1;if(result&&capacity>=count)memcpy(result,json,count);}
      }
      end_interpreter(sub);
    }
    thread_swap(previous);
    if(!retry)break;
  }
  release_gil(gil);free(script);free(rh);free(qh);
  if (!count) snprintf(error,sizeof(error),"Randomizer interpreter failed; see log");
done:
  UNLOCK(); return count;
}
const char *sm_randomizer_error(void) { return error; }

int sm_randomizer_generate(const char *root,const char *request,char *result,int capacity){return execute(root,request,result,capacity,0);}
int sm_tracker_evaluate(const char *root,const char *request,char *result,int capacity){return execute(root,request,result,capacity,1);}
int sm_randomizer_validate(const char *root,const char *request,char *result,int capacity){return execute(root,request,result,capacity,2);}

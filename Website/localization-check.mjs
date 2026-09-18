// Disposable remote Chromium review. Uses no browser profile or player save.
import {spawn,execFileSync} from 'node:child_process';
import {mkdtemp,readFile,writeFile,mkdir,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const root=path.resolve(process.argv[2]), out=path.join(root,'Website/validation');
await mkdir(out,{recursive:true});
const profile=await mkdtemp(path.join(tmpdir(),'sm-title-browser-'));
const chrome=spawn('/home/nobara-user/.cache/ms-playwright/chromium-1243/chrome-linux64/chrome',[
 '--headless','--no-sandbox','--disable-gpu','--hide-scrollbars','--remote-debugging-port=0',
 '--no-first-run','--no-default-browser-check','--user-data-dir='+profile,'about:blank'
],{stdio:'ignore'});
let ws;
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
try {
 let port;
 for(let i=0;i<100;i++){try{port=(await readFile(path.join(profile,'DevToolsActivePort'),'utf8')).split('\n')[0];break;}catch{await sleep(100);}}
 if(!port)throw Error('Chromium did not start');
 const target=await (await fetch(`http://127.0.0.1:${port}/json/new?about:blank`,{method:'PUT'})).json();
 ws=new WebSocket(target.webSocketDebuggerUrl);await new Promise((r,j)=>{ws.onopen=r;ws.onerror=j;});
 let seq=0;const pending=new Map(),errors=[];
 ws.onmessage=event=>{const m=JSON.parse(event.data);if(m.id){const p=pending.get(m.id);pending.delete(m.id);m.error?p?.reject(Error(JSON.stringify(m.error))):p?.resolve(m.result);}if(m.method==='Runtime.exceptionThrown')errors.push(m.params.exceptionDetails);};
 const call=(method,params={})=>new Promise((resolve,reject)=>{const id=++seq;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));});
 const evaluate=async expression=>{const r=await call('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;};

 await call('Page.enable');await call('Runtime.enable');await call('Network.enable');
 const failures=[];ws.addEventListener('message',event=>{const m=JSON.parse(event.data);if(m.method==='Network.responseReceived'&&m.params.response.status>=400)failures.push({url:m.params.response.url,status:m.params.response.status});});
 const base='http://127.0.0.1:28743/zebes/';
 // Intercept public API only: exercise empty, populated and failure states without submissions.
 await call('Fetch.enable',{patterns:[{urlPattern:'*zebes/api/leaderboard*'}]});
 let apiMode='empty';
 ws.addEventListener('message',async event=>{const m=JSON.parse(event.data);if(m.method!=='Fetch.requestPaused')return;
  const data=apiMode==='rows'?{total:1,page_size:20,runs:[{rank:1,display_name:'Vanilla',igt_frames:3600,real_ms:60000,video_url:'https://youtu.be/test'}]}:{total:0,page_size:20,runs:[]};
  await call('Fetch.fulfillRequest',{requestId:m.params.requestId,responseCode:apiMode==='fail'?503:200,responseHeaders:[{name:'Content-Type',value:'application/json'}],body:Buffer.from(JSON.stringify(data)).toString('base64')});
 });
 for(const page of ['fr/','fr/help.html'])for(const [name,width,height] of [['desktop',1440,1000],['mobile',390,844]]) {
  await call('Emulation.setDeviceMetricsOverride',{width,height,deviceScaleFactor:1,mobile:name==='mobile'});
  await call('Page.navigate',{url:base+page});await sleep(500);
  for(let i=0;i<150;i++){if(await evaluate('document.readyState==="complete"&&document.documentElement.lang==="fr-CA"'))break;await sleep(100);}
  await evaluate('document.fonts.ready');await sleep(300);
  if(!await evaluate('document.documentElement.scrollWidth<=innerWidth'))throw Error('Overflow '+page+name);
  if(!await evaluate('document.querySelector("[data-language-switch]").textContent==="EN"'))throw Error('Language link');
  if(page==='fr/'){
   await evaluate('document.querySelector("#leaderboard").scrollIntoView();loadLeaderboard()');await sleep(600);
   if(!await evaluate('document.querySelector("#board-status").textContent==="Résultats accessibles"'))throw Error('Empty result language '+name+' '+await evaluate('document.querySelector("#board-status").textContent')+JSON.stringify(errors));
   apiMode='rows';await evaluate('loadLeaderboard()');await sleep(200);
   if(!await evaluate('document.querySelector("#board-rows td:nth-child(2)").textContent==="Vanilla"'))throw Error('Player name translated');
   if(!await evaluate('document.querySelector("#board-status").textContent.includes("joueur classé")'))throw Error('Count language');
   await evaluate('document.querySelector("#open-submission").click()');
   if(!await evaluate('document.querySelector("#submission-dialog").open'))throw Error('Submission dialog');
   await evaluate('document.querySelector("#submission-dialog").close()');
   await evaluate('window.scrollTo(0,0)');
   apiMode='empty';
  }else{
   await evaluate('document.querySelector("#map-tracker details summary").click()');
   if(!await evaluate('document.querySelector("#map-tracker details").open'))throw Error('FAQ');
  }
  await evaluate('Promise.all(Array.from(document.images).filter(i=>i.complete).map(i=>i.decode().catch(()=>{})))');
  await evaluate('window.scrollTo(0,0)');await sleep(400);
  if(page.includes('help'))console.log(await evaluate('JSON.stringify(Array.from(document.querySelectorAll(".help-intro,.help-note,.help-topics")).slice(0,3).map(e=>({c:e.className,r:e.getBoundingClientRect().toJSON(),pos:getComputedStyle(e).position})))'));
  const shot=await call('Page.captureScreenshot',{format:'png'});await writeFile(path.join(out,'fr-'+(page.includes('help')?'help-':'home-')+name+'.png'),Buffer.from(shot.data,'base64'));
 }
 const url=base+'fr/';
 if(errors.length||failures.length)throw Error(JSON.stringify({errors,failures}));
 console.log(JSON.stringify({ok:true,url,desktop:true,mobile:true,faq:true,mobileMenu:true,errors,failures}));
}finally{ws?.close();chrome.kill();await sleep(300);await rm(profile,{recursive:true,force:true});}

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
 const url=process.argv[3]||'http://127.0.0.1:28742/zebes/';
 await call('Emulation.setDeviceMetricsOverride',{width:1440,height:1000,deviceScaleFactor:1,mobile:false});
 await call('Page.navigate',{url});
 for(let i=0;i<150;i++){if(await evaluate('document.readyState==="complete"&&Boolean(document.querySelector("#motion-toggle"))'))break;await sleep(100);}
 await evaluate('document.fonts.ready');await sleep(500);
 const assert=async(expression,label)=>{if(!await evaluate(expression))throw Error(label);};
 await assert('document.documentElement.scrollWidth<=innerWidth','Desktop page overflows');
 await assert('[...document.querySelectorAll(".hero img")].every(i=>i.complete&&i.naturalWidth>0)','Hero images missing');
 async function screenshot(name,full=false){
  const params={format:'png',captureBeyondViewport:true,clip:await evaluate('({x:0,y:scrollY,width:innerWidth,height:innerHeight,scale:1})')};
  if(full)params.clip=await evaluate('({x:0,y:0,width:innerWidth,height:document.documentElement.scrollHeight,scale:1})');
  const shot=await call('Page.captureScreenshot',params);await writeFile(path.join(out,name+'.png'),Buffer.from(shot.data,'base64'));
 }
 await screenshot('desktop');
 await evaluate('document.querySelector("#leaderboard").scrollIntoView({behavior:"instant"})');
 for(let i=0;i<100;i++){if(await evaluate('document.querySelector("#board-status").textContent==="Mission records online"'))break;await sleep(100);}
 await assert('document.querySelector("#empty-title").textContent==="The first record could be yours."','Empty leaderboard did not load');
 await screenshot('leaderboard');
 await evaluate('document.querySelector("[data-mode=ngplus]").click();document.querySelector("#category").value="qol";document.querySelector("#category").dispatchEvent(new Event("change"))');
 await sleep(300);await assert('document.querySelector("[data-mode=ngplus]").getAttribute("aria-pressed")==="true"','Mode control failed');
 await evaluate('document.querySelector("[data-shot=tracker]").click()');
 await assert('document.querySelector("#gallery-dialog").open','Screenshot dialog did not open');
 await call('Input.dispatchKeyEvent',{type:'keyDown',key:'Escape',code:'Escape',windowsVirtualKeyCode:27});
 await call('Input.dispatchKeyEvent',{type:'keyUp',key:'Escape',code:'Escape',windowsVirtualKeyCode:27});
 await assert('!document.querySelector("#gallery-dialog").open','Escape did not close dialog');
 await evaluate('document.querySelector("#open-submission").click()');
 await assert('document.querySelector("#submission-dialog").open','Submission form did not open');
 await screenshot('submission');
 if(url.startsWith('http://127.0.0.1:28742/')){
  const runId=crypto.randomUUID();
  const fixture=path.join(out,'isolated-run.json');
  await writeFile(fixture,JSON.stringify({schema:1,run_id:runId,ruleset:'zebes-v1',mode:'ngplus',category:'qol',igt_frames:216045,real_ms:3700000,eligible:true,invalid_reason:0}));
  try{
   await evaluate('document.querySelector("#runner-name").value="Website <runner>";document.querySelector("#run-video").value="https://youtu.be/isolated-test";document.querySelector("[name=consent]").checked=true');
   const doc=await call('DOM.getDocument');
   const input=await call('DOM.querySelector',{nodeId:doc.root.nodeId,selector:'#run-file'});
   await call('DOM.setFileInputFiles',{nodeId:input.nodeId,files:[fixture]});
   await evaluate('document.querySelector("#submission-form").requestSubmit()');
   for(let i=0;i<100;i++){if(await evaluate('document.querySelector("#submission-result").textContent.includes("Run received")'))break;await sleep(100);}
   await assert('document.querySelector("#submission-result").textContent.includes("Run received")','Actual form submission failed');
   const pendingBoard=await (await fetch('http://127.0.0.1:28742/zebes/api/leaderboard?mode=ngplus&category=qol')).json();
   if(pendingBoard.total!==0)throw Error('Pending run was exposed');
   execFileSync('python3',[path.join(root,'Server/receiver/review.py'),'--db',path.join(root,'Website/validation-data/runs.sqlite'),'approve',runId,'--new-player','--name','Website <runner>','--note','Isolated synthetic fixture, never a public score.']);
   await evaluate('document.querySelector("#submission-dialog").close();document.querySelector("#refresh-board").click()');
   for(let i=0;i<100;i++){if(await evaluate('document.querySelector("#board-rows").children.length===1'))break;await sleep(100);}
   await assert('document.querySelector("#board-rows").textContent.includes("Website <runner>")','Reviewed score not rendered as plain text');
   await assert('document.querySelector("#board-rows").textContent.includes("01:00:00.75")','Native frame timer incorrect');
   await assert('document.querySelector("#board-rows").querySelectorAll("runner").length===0','Display name interpreted as HTML');
   await evaluate('document.querySelector("#leaderboard").scrollIntoView({behavior:"instant"})');await screenshot('isolated-reviewed-result');
  }finally{
   const cleanup='import sqlite3,sys; db=sqlite3.connect(sys.argv[1]); key=sys.argv[2]; player=db.execute("SELECT player_id FROM runs WHERE run_id=?",(key,)).fetchone(); db.execute("DELETE FROM moderation_events WHERE run_id=?",(key,)); db.execute("DELETE FROM website_submissions WHERE run_id=?",(key,)); db.execute("DELETE FROM runs WHERE run_id=?",(key,)); db.execute("DELETE FROM players WHERE player_id=?",(player[0] if player else None,)); db.execute("DELETE FROM website_rate_limits"); db.commit(); db.close()';
   execFileSync('python3',['-c',cleanup,path.join(root,'Website/validation-data/runs.sqlite'),runId]);
   await rm(fixture,{force:true});
  }
  await evaluate('document.querySelector("#refresh-board").click()');await sleep(200);
 }

 await evaluate('document.querySelector("#submission-dialog").close()');
 // Load every lazy screenshot before a full-page capture.
 await evaluate('[...document.images].forEach(i=>i.loading="eager")');await sleep(500);
 await assert('[...document.images].filter(i=>i.hasAttribute("src")).every(i=>i.complete&&i.naturalWidth>0)','Page image missing');
 await evaluate('scrollTo({top:0,behavior:"instant"})');await screenshot('desktop-full',true);
 await call('Emulation.setDeviceMetricsOverride',{width:390,height:844,deviceScaleFactor:1,mobile:true});
 await call('Emulation.setEmulatedMedia',{features:[{name:'prefers-reduced-motion',value:'reduce'}]});
 await evaluate('scrollTo({top:0,behavior:"instant"})');await sleep(200);
 await assert('document.documentElement.scrollWidth<=innerWidth','Mobile page overflows');
 await assert('document.body.classList.contains("motion-paused")','Reduced motion not respected');
 await screenshot('mobile');
 await evaluate('document.querySelector(".menu-toggle").click()');
 await assert('document.querySelector(".menu-toggle").getAttribute("aria-expanded")==="true"','Mobile navigation failed');
 await evaluate('document.querySelector("#site-nav a").click()');
 await assert('document.querySelector(".menu-toggle").getAttribute("aria-expanded")==="false"','Mobile navigation not closed after link');
 await screenshot('mobile-full',true);
 if(errors.length)throw Error(JSON.stringify(errors));
 if(failures.length)throw Error(JSON.stringify(failures));
 const report={passed:true,isolatedSubmissionReviewFlow:url.startsWith('http://127.0.0.1:28742/'),url,host:'gaming-pc',desktop:[1440,1000],mobile:[390,844],runtimeErrors:errors,networkFailures:failures,cases:['asset loading','no horizontal overflow','live empty leaderboard','mode and category filters','screenshot lightbox and Escape','submission dialog','mobile navigation','reduced motion']};
 await writeFile(path.join(out,'browser-verification.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));
}finally{ws?.close();chrome.kill('SIGTERM');await sleep(300);await rm(profile,{recursive:true,force:true});}

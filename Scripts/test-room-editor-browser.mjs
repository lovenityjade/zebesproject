// Real browser interaction on gaming-pc; isolated profile and patch directory.
import fs from 'node:fs/promises';
import os from 'node:os';
import assert from 'node:assert/strict';
assert.equal(os.hostname(),'gaming-pc');
const targets=await (await fetch('http://127.0.0.1:19222/json')).json();
const target=targets.find(t=>t.type==='page'&&t.url.startsWith('http://127.0.0.1:18766'));
assert.ok(target,'Editor browser tab absent');
const socket=new WebSocket(target.webSocketDebuggerUrl);let seq=0;const pending=new Map(),errors=[];
socket.onmessage=event=>{const message=JSON.parse(event.data);if(message.id){const p=pending.get(message.id);pending.delete(message.id);message.error?p.reject(Error(JSON.stringify(message.error))):p.resolve(message.result);}else if(message.method==='Runtime.exceptionThrown')errors.push(message.params.exceptionDetails);};
await new Promise((resolve,reject)=>{socket.onopen=resolve;socket.onerror=reject;});
function cdp(method,params={}){return new Promise((resolve,reject)=>{const id=++seq;pending.set(id,{resolve,reject});socket.send(JSON.stringify({id,method,params}));});}
async function evaluate(expression){const r=await cdp('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;}
async function waitFor(expression){for(let i=0;i<100;i++){if(await evaluate(expression))return;await new Promise(r=>setTimeout(r,200));}throw Error(`Timeout: ${expression}`);}
async function point(selector,fx=.5,fy=.5){return evaluate(`(()=>{const r=document.querySelector(${JSON.stringify(selector)}).getBoundingClientRect();return {x:r.x+r.width*${fx},y:r.y+r.height*${fy}}})()`);}
async function mouse(type,p,button='left',buttons=0){await cdp('Input.dispatchMouseEvent',{type,...p,button,buttons,clickCount:type==='mouseMoved'?0:1});}
async function click(selector){const p=await point(selector);await mouse('mousePressed',p,'left',1);await mouse('mouseReleased',p);}
async function select(selector,value){await evaluate(`(()=>{const e=document.querySelector(${JSON.stringify(selector)});e.value=${JSON.stringify(String(value))};e.dispatchEvent(new Event('change',{bubbles:true}));})()`);}
async function paint(x,y,button='left'){
  const p=await evaluate(`(()=>{const r=document.querySelector('#scene').getBoundingClientRect(),z=Number(document.querySelector('#zoom').value);return {x:r.x+(${x}+8+.5)*16*z,y:r.y+(${y}+8+.5)*16*z};})()`);
  await mouse('mousePressed',p,button,button==='right'?2:1);await mouse('mouseReleased',p,button);
}
await cdp('Runtime.enable');await cdp('Page.enable');
await cdp('Emulation.setDeviceMetricsOverride',{width:1440,height:1000,deviceScaleFactor:1,mobile:false});
await cdp('Page.navigate',{url:'http://127.0.0.1:18766'});
await waitFor(`!document.querySelector('#save').disabled && document.querySelector('#tilesetInfo').textContent.includes('Tileset')`);
await select('#zone','Brinstar');
await waitFor(`!document.querySelector('#save').disabled && Number(document.querySelector('#room').value)===${0x9f11}`);
assert.equal(await evaluate(`Number(document.querySelector('#room').value)`),0x9f11);
await evaluate(`document.querySelector('#viewport').scrollLeft=0;document.querySelector('#viewport').scrollTop=0`);
// Select a visible original tile by clicking its palette cell.
const swatch=await evaluate(`(()=>{const c=document.querySelector('#palette'),d=c.getContext('2d').getImageData(0,0,c.width,c.height).data;for(let i=1;i<32;i++){let n=0;for(let y=0;y<32;y++)for(let x=0;x<32;x++){const p=(((i>>3)*32+y)*256+(i%8)*32+x)*4;if(d[p]+d[p+1]+d[p+2]>50&&d[p+3])n++;}if(n>80){const r=c.getBoundingClientRect();return {x:r.x+((i%8)*32+16)*r.width/256,y:r.y+((i>>3)*32+16)*r.height/c.height};}}})()`);
assert.ok(swatch);await mouse('mousePressed',swatch,'left',1);await mouse('mouseReleased',swatch);
await paint(-1,3);await paint(1,3);
assert.equal(await evaluate(`document.querySelector('#undo').disabled`),false);
await click('#undo');assert.equal(await evaluate(`document.querySelector('#redo').disabled`),false);await click('#redo');
await select('#layer',1);await click('#flipX');await paint(2,3);
await select('#layer',0);await paint(1,3,'right');
await click('#collision');
const screenshot=await cdp('Page.captureScreenshot',{format:'png'});
await fs.mkdir('Docs/RoomEditor',{recursive:true});await fs.writeFile('Docs/RoomEditor/editor-browser.png',Buffer.from(screenshot.data,'base64'));
await click('#save');await waitFor(`document.querySelector('#status').textContent.includes('enregistré')`);
const patch=await evaluate(`fetch('/api/patch?room='+document.querySelector('#room').value+'&state='+document.querySelector('#state').value).then(r=>r.json())`);
assert.equal(patch.cells.length,2);assert.ok(patch.cells.some(c=>c.x===-1&&c.layer===0));assert.ok(patch.cells.some(c=>c.layer===1&&(c.tile&1024)));
const rejected=await evaluate(`fetch('/api/patch',{method:'POST',headers:{'Content-Type':'application/json','X-Editor-Token':document.querySelector('meta[name="editor-token"]').content},body:JSON.stringify({room:${patch.room},state:${patch.state},cells:[],collision:[8]})}).then(r=>r.status)`);
assert.equal(rejected,400);
await cdp('Page.reload');await waitFor(`!document.querySelector('#save').disabled && document.querySelector('#saveState').textContent.startsWith('2 retouches')`);
const persisted=await evaluate(`fetch('/api/patch?room='+document.querySelector('#room').value+'&state='+document.querySelector('#state').value).then(r=>r.json())`);
assert.deepEqual(persisted,patch);assert.deepEqual(errors,[]);
await fs.writeFile('Docs/RoomEditor/browser-verification.json',JSON.stringify({passed:true,room:patch.room,state:patch.state,paintedCells:2,undoRedo:true,eraser:true,backgroundFlip:true,persistedAfterReload:true,collisionWriteRejected:true,browserErrors:errors},null,2));
console.log('SM_ROOM_EDITOR_BROWSER_PASS');socket.close();

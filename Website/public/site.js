'use strict';
const french = document.documentElement.lang === 'fr-CA';
function t(key, values = {}) {
  const text = french ? (window.ZebesFrench?.[key] ?? key) : key;
  return text.replace(/\{(\w+)\}/g, (match, name) => Object.hasOwn(values, name) ? String(values[name]) : match);
}
const $ = selector => document.querySelector(selector);
const $$ = selector => [...document.querySelectorAll(selector)];
const state = {mode:'vanilla',category:'noqol',page:1,total:0,request:0,controller:null};
const prefersReduced = matchMedia('(prefers-reduced-motion: reduce)');
let paused = prefersReduced.matches;
try { paused = prefersReduced.matches || localStorage.getItem('zebes-motion') === 'paused'; } catch {}
function setMotion(value) {
  paused=value;document.body.classList.toggle('motion-paused',paused);
  $('#motion-toggle').textContent=prefersReduced.matches?t('Reduced motion'):(paused?t('Enable motion'):t('Pause motion'));
  $('#motion-toggle').disabled=prefersReduced.matches;
  $('#motion-toggle').setAttribute('aria-pressed',String(paused));
}
setMotion(paused);
$('#motion-toggle').addEventListener('click',()=>{setMotion(!paused);try{localStorage.setItem('zebes-motion',paused?'paused':'enabled');}catch{}});
prefersReduced.addEventListener('change',event=>setMotion(event.matches));
for(let i=0;i<36;i++){
  const star=document.createElement('i');star.className='star';
  star.style.left=((i*73.31)%100)+'%';star.style.top=((i*43.27)%88)+'%';star.style.animationDelay=(-(i*1.83)%11)+'s';$('.stars').append(star);
}
$('.hero').addEventListener('pointermove',event=>{
  if(paused||prefersReduced.matches||event.pointerType!=='mouse')return;
  const bounds=event.currentTarget.getBoundingClientRect();
  $('.hero-art').style.setProperty('--art-x',((event.clientX-bounds.left)/bounds.width-.5)*9+'px');
  $('.hero-art').style.setProperty('--art-y',((event.clientY-bounds.top)/bounds.height-.5)*5+'px');
});
$('.menu-toggle').addEventListener('click',()=>{
  const open=$('.menu-toggle').getAttribute('aria-expanded')!=='true';
  $('.menu-toggle').setAttribute('aria-expanded',String(open));$('#site-nav').classList.toggle('is-open',open);
});
$('#site-nav').addEventListener('click',event=>{if(event.target.closest('a')){$('.menu-toggle').setAttribute('aria-expanded','false');$('#site-nav').classList.remove('is-open');}});
document.addEventListener('keydown',event=>{if(event.key==='Escape'){$('.menu-toggle').setAttribute('aria-expanded','false');$('#site-nav').classList.remove('is-open');}});

function openDialog(dialog){dialog.showModal();document.body.classList.add('dialog-open');}
$$('dialog').forEach(dialog=>{
  dialog.querySelector('[data-close]').addEventListener('click',()=>dialog.close());
  dialog.addEventListener('close',()=>document.body.classList.remove('dialog-open'));
  dialog.addEventListener('click',event=>{if(event.target===dialog){const r=dialog.getBoundingClientRect();if(event.clientX<r.left||event.clientX>r.right||event.clientY<r.top||event.clientY>r.bottom)dialog.close();}});
});
$$('[data-shot]').forEach(button=>button.addEventListener('click',()=>{
  const image=button.querySelector('img');$('#gallery-image').src=image.src;$('#gallery-image').alt=image.alt;
  $('#gallery-caption').textContent=image.alt;openDialog($('#gallery-dialog'));
}));
$('#open-submission').addEventListener('click',()=>openDialog($('#submission-dialog')));

function formatFrames(frames){
  const total=Math.floor(frames/60),hours=Math.floor(total/3600),minutes=Math.floor(total/60)%60,seconds=total%60;
  return `${String(hours).padStart(2,'0')}:${String(minutes).padStart(2,'0')}:${String(seconds).padStart(2,'0')}.${String(Math.floor((frames%60)*100/60)).padStart(2,'0')}`;
}
function formatReal(ms){if(!ms)return '—';const seconds=Math.floor(ms/1000);return [Math.floor(seconds/3600),Math.floor(seconds/60)%60,seconds%60].map(n=>String(n).padStart(2,'0')).join(':');}
function safeVideo(url){try{const u=new URL(url);return u.protocol==='https:'&&['youtube.com','www.youtube.com','m.youtube.com','youtu.be','twitch.tv','www.twitch.tv','videos.twitch.tv'].includes(u.hostname)&&!u.username&&!u.password&&!u.port;}catch{return false;}}
async function loadLeaderboard(){
  const request=++state.request;state.controller?.abort();state.controller=new AbortController();
  $('#board-status').textContent=t('Updating mission records…');$('#previous-page').disabled=true;$('#next-page').disabled=true;
  $('#refresh-board').disabled=true;
  const timeout=setTimeout(()=>state.controller?.abort(),12000);
  try{
    const query=new URLSearchParams({mode:state.mode,category:state.category,page:String(state.page)});
    const response=await fetch('/zebes/api/leaderboard?'+query,{signal:state.controller.signal,headers:{Accept:'application/json'}});
    if(!response.ok)throw new Error(t('Rankings unavailable'));
    const data=await response.json();if(request!==state.request)return;
    if(!Array.isArray(data.runs)||!Number.isInteger(data.total))throw new Error('Invalid leaderboard');
    $('#board-rows').replaceChildren();state.total=data.total;
    for(const run of data.runs){
      const tr=document.createElement('tr');
      for(const value of [String(run.rank).padStart(2,'0'),run.display_name,formatFrames(run.igt_frames),formatReal(run.real_ms)]){
        const cell=document.createElement('td');cell.textContent=value;tr.append(cell);
      }
      const proof=document.createElement('td');if(run.video_url&&safeVideo(run.video_url)){
        const link=document.createElement('a');link.href=run.video_url;link.target='_blank';link.rel='noopener noreferrer';link.textContent=t('Watch ↗');link.setAttribute('aria-label',t('Watch {name}’s run',{name:run.display_name}));proof.append(link);
      }else proof.textContent=t('Reviewed');tr.append(proof);$('#board-rows').append(tr);
    }
    $('#board-status').textContent=data.total?t(data.total===1?'{count} ranked runner · Reviewed runs':'{count} ranked runners · Reviewed runs',{count:data.total}):t('Mission records online');
    $('#board-empty').hidden=data.runs.length>0;
    $('#empty-title').textContent=t('The first record could be yours.');
    $('#empty-description').textContent=t('No verified runs in this category yet. Finish a speedrun and send your record for review to leave your mark.');
    $('#board-page').textContent=data.total?t('Page {page} of {pages} · Best time per runner',{page:state.page,pages:Math.ceil(data.total/data.page_size)}):t('Verified runs only · Best time per runner');
    $('#previous-page').disabled=state.page<=1;$('#next-page').disabled=state.page*data.page_size>=data.total;
  }catch(error){
    if(request!==state.request)return;
    $('#board-rows').replaceChildren();$('#board-empty').hidden=false;
    $('#board-status').textContent=t('Connection interrupted');$('#empty-title').textContent=t('Signal temporarily lost.');
    $('#empty-description').textContent=t('The rankings could not be loaded. Use the refresh button to reconnect; your run records are safe.');
    $('#board-page').textContent=t('Rankings unavailable');
  }finally{clearTimeout(timeout);if(request===state.request)$('#refresh-board').disabled=false;}
}
$$('[data-mode]').forEach(button=>button.addEventListener('click',()=>{
  state.mode=button.dataset.mode;state.page=1;$$('[data-mode]').forEach(b=>b.setAttribute('aria-pressed',String(b===button)));loadLeaderboard();
}));
$('#category').addEventListener('change',event=>{state.category=event.target.value;state.page=1;loadLeaderboard();});
$('#refresh-board').addEventListener('click',loadLeaderboard);
$('#previous-page').addEventListener('click',()=>{if(state.page>1){state.page--;loadLeaderboard();}});
$('#next-page').addEventListener('click',()=>{state.page++;loadLeaderboard();});
if('IntersectionObserver' in window){const observer=new IntersectionObserver(entries=>{if(entries.some(e=>e.isIntersecting)){loadLeaderboard();observer.disconnect();}},{rootMargin:'300px'});observer.observe($('#leaderboard'));}else loadLeaderboard();

async function readRun(){
  const file=$('#run-file').files[0];if(!file)throw new Error(t('Choose your completed run record.'));
  if(file.size>8192)throw new Error(t('Run records must be smaller than 8 KB. Choose the .run-<uuid>.json file, not a save or ROM.'));
  let run;try{run=JSON.parse(await file.text());}catch{throw new Error(t('This file is not a valid JSON run record.'));}
  if(!run||run.schema!==1||run.ruleset!=='zebes-v1'||!['vanilla','ngplus'].includes(run.mode)||!['qol','noqol'].includes(run.category)||!Number.isInteger(run.igt_frames)||run.igt_frames<=0)throw new Error(t('Choose a completed Vanilla or New Game+ speedrun record from The Zebes Project.'));
  if(run.eligible!==true||run.invalid_reason!==0)throw new Error(t('This run was marked ineligible by the game and cannot enter the leaderboard.'));
  return run;
}
$('#run-file').addEventListener('change',async()=>{const target=$('#record-summary');target.classList.remove('error-text');try{const run=await readRun();target.textContent=`${run.mode==='vanilla'?t('Vanilla'):t('New Game+')} · ${run.category==='noqol'?t('No QoL'):t('QoL')} · ${formatFrames(run.igt_frames)}`;}catch(error){target.classList.add('error-text');target.textContent=t(error.message);}});
$('#submission-form').addEventListener('submit',async event=>{
  event.preventDefault();const button=event.currentTarget.querySelector('[type=submit]'),result=$('#submission-result');
  if(button.disabled)return;button.disabled=true;result.classList.remove('error-text');result.textContent=t('Preparing your mission debrief…');
  try{
    const run=await readRun(),video=$('#run-video').value.trim();
    if(!safeVideo(video))throw new Error(t('Use an HTTPS link to your complete run on YouTube or Twitch.'));
    const response=await fetch('/zebes/api/submissions',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({run,display_name:$('#runner-name').value.trim(),video_url:video,consent:true}),signal:AbortSignal.timeout(15000)});
    let data;try{data=await response.json();}catch{throw new Error(t('The receiver is temporarily unavailable. Please try again later.'));}
    if(!response.ok)throw new Error(data.error||t('Your submission could not be received. Please try again later.'));
    result.textContent=`${data.duplicate?t('This run is already in the review queue.'):t('Run received. It will appear after review.')} ${t('Reference: {id}',{id:data.run_id})}`;
    event.target.reset();$('#record-summary').textContent='';
  }catch(error){result.classList.add('error-text');result.textContent=error.name==='TimeoutError'?t('The response took too long. You can safely retry the same run record.'):t(error.message);}
  finally{button.disabled=false;}
});

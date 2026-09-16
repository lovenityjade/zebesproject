'use strict';
const $=id=>document.getElementById(id), ctx=$('scene').getContext('2d'), pctx=$('palette').getContext('2d');
let catalog=[], room=null, atlas=null, cells=new Map(), tile=0, tool='brush', margin=8;
let undo=[],redo=[],stroke=null,lastCell=null,dirty=false,loading=false,epoch=0,paletteIds=[];
const key=(layer,x,y)=>`${layer}:${x}:${y}`;
function status(text,error=false){$('status').textContent=text;$('status').classList.toggle('error',error);}
async function api(url,options){const response=await fetch(url,options);const result=await response.json();if(!response.ok)throw Error(result.error||response.statusText);return result;}
function options(select,rows,value,label){select.replaceChildren(...rows.map(row=>new Option(label(row),value(row))));}
function changed(){dirty=true;buttons();}
function buttons(){
  $('undo').disabled=!undo.length||loading;$('redo').disabled=!redo.length||loading;$('save').disabled=!room||loading;
  $('saveState').textContent=dirty?'Retouches non enregistrées':`${cells.size} retouche${cells.size===1?'':'s'} enregistrée${cells.size===1?'':'s'}`;
  $('saveState').classList.toggle('dirty',dirty);
}
function chooseTool(name){tool=name;for(const id of ['brush','eraser','picker'])$(id).classList.toggle('active',id===name);}
function brushTile(){return tile|($('flipX').checked?1024:0)|($('flipY').checked?2048:0);}
function tileImage(context,value,x,y,scale){
  const id=value&1023,flipX=!!(value&1024),flipY=!!(value&2048);
  context.save();context.translate(x+(flipX?16*scale:0),y+(flipY?16*scale:0));context.scale(flipX?-1:1,flipY?-1:1);
  context.drawImage(atlas,(id%32)*16,Math.floor(id/32)*16,16,16,0,0,16*scale,16*scale);context.restore();
}
function drawPalette(){
  if(!room||!atlas)return;
  paletteIds=$('used').checked?room.used:Array.from({length:1024},(_,i)=>i);
  $('palette').height=Math.ceil(paletteIds.length/8)*32;pctx.imageSmoothingEnabled=false;
  paletteIds.forEach((id,i)=>{tileImage(pctx,id,(i%8)*32,Math.floor(i/8)*32,2);if(id===tile){pctx.strokeStyle='#fff086';pctx.lineWidth=2;pctx.strokeRect((i%8)*32+1,Math.floor(i/8)*32+1,30,30);}});
  $('selection').textContent=`Tile ${tile.toString(16).toUpperCase().padStart(3,'0')} · ${$('layer').value==='0'?'avant':'arrière'}`;
}
function draw(){
  if(!room||!atlas)return;
  const z=Number($('zoom').value),unit=16*z,w=(room.width+margin*2)*unit,h=(room.height+margin*2)*unit;
  if($('scene').width!==w||$('scene').height!==h){$('scene').width=w;$('scene').height=h;}
  ctx.imageSmoothingEnabled=false;ctx.fillStyle='#0c1015';ctx.fillRect(0,0,w,h);
  ctx.fillStyle='#000';ctx.fillRect(margin*unit,margin*unit,room.width*unit,room.height*unit);
  for(const layer of [1,0]) {
    if(!$(layer===0?'showFront':'showBack').checked)continue;
    const base=room[layer===0?'foreground':'background'];
    for(let y=-margin;y<room.height+margin;y++)for(let x=-margin;x<room.width+margin;x++) {
      const painted=$('original').checked?null:cells.get(key(layer,x,y));
      let value=painted?.tile;
      if(value===undefined&&x>=0&&y>=0&&x<room.width&&y<room.height)value=base[y*room.width+x];
      if(value!==undefined)tileImage(ctx,value,(x+margin)*unit,(y+margin)*unit,z);
    }
  }
  if($('collision').checked)for(let y=0;y<room.height;y++)for(let x=0;x<room.width;x++) {
    const type=room.collision[y*room.width+x];if(!type)continue;
    ctx.fillStyle=type===8?'#ef6ba94d':type===1?'#edcb644d':'#ee8e494d';ctx.fillRect((x+margin)*unit,(y+margin)*unit,unit,unit);
    if(z>=2){ctx.fillStyle='#fff';ctx.font='10px monospace';ctx.fillText(type.toString(16).toUpperCase(),(x+margin)*unit+2,(y+margin)*unit+11);}
  }
  if($('grid').checked){ctx.strokeStyle='#a4bac51c';ctx.lineWidth=1;ctx.beginPath();for(let x=0;x<=w;x+=unit){ctx.moveTo(x+.5,0);ctx.lineTo(x+.5,h);}for(let y=0;y<=h;y+=unit){ctx.moveTo(0,y+.5);ctx.lineTo(w,y+.5);}ctx.stroke();}
  ctx.strokeStyle='#80d1da';ctx.lineWidth=2;ctx.strokeRect(margin*unit,margin*unit,room.width*unit,room.height*unit);
  buttons();
}
function center(){if(room){const unit=16*Number($('zoom').value);$('viewport').scrollLeft=Math.max(0,margin*unit-24);$('viewport').scrollTop=Math.max(0,margin*unit-24);}}
function coordinates(event){const rect=$('scene').getBoundingClientRect(),u=16*Number($('zoom').value);return {x:Math.floor((event.clientX-rect.left)/u)-margin,y:Math.floor((event.clientY-rect.top)/u)-margin};}
function paintAt(x,y,erase){
  if(x < -margin||y < -margin||x>=room.width+margin||y>=room.height+margin)return;
  const layer=Number($('layer').value),k=key(layer,x,y),before=cells.get(k)||null;
  if(!stroke.has(k))stroke.set(k,before);
  if(erase)cells.delete(k);else cells.set(k,{x,y,layer,tile:brushTile()});
}
function lineTo(point,erase){
  let {x,y}=lastCell||point;const dx=Math.abs(point.x-x),sx=x<point.x?1:-1,dy=-Math.abs(point.y-y),sy=y<point.y?1:-1;let err=dx+dy;
  for(;;){paintAt(x,y,erase);if(x===point.x&&y===point.y)break;const e=2*err;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}
  lastCell=point;draw();
}
function finishStroke(){
  if(!stroke)return;
  const changes=[...stroke].map(([k,before])=>({k,before,after:cells.get(k)||null})).filter(c=>JSON.stringify(c.before)!==JSON.stringify(c.after));
  if(changes.length){undo.push(changes);if(undo.length>100)undo.shift();redo=[];changed();}
  stroke=null;lastCell=null;buttons();
}
function history(back){
  finishStroke();const from=back?undo:redo,to=back?redo:undo;if(!from.length)return;
  const changes=from.pop();for(const c of changes){const value=back?c.before:c.after;if(value)cells.set(c.k,value);else cells.delete(c.k);}to.push(changes);changed();draw();
}
$('scene').addEventListener('pointerdown',event=>{
  if(!room||loading||![0,2].includes(event.button))return;event.preventDefault();$('scene').focus();const point=coordinates(event);
  if(tool==='picker'&&event.button===0){const layer=Number($('layer').value),value=cells.get(key(layer,point.x,point.y))?.tile??(point.x>=0&&point.y>=0&&point.x<room.width&&point.y<room.height?room[layer===0?'foreground':'background'][point.y*room.width+point.x]:undefined);
    if(value!==undefined){tile=value&1023;$('flipX').checked=!!(value&1024);$('flipY').checked=!!(value&2048);drawPalette();chooseTool('brush');}return;}
  stroke=new Map();lastCell=null;$('scene').setPointerCapture(event.pointerId);lineTo(point,event.button===2||tool==='eraser');
});
$('scene').addEventListener('pointermove',event=>{if(!room)return;const p=coordinates(event);$('position').textContent=`Case ${p.x}, ${p.y} · ${p.x>=0&&p.y>=0&&p.x<room.width&&p.y<room.height?'dans la salle':'prolongement'}`;if(stroke)lineTo(p,!!(event.buttons&2)||tool==='eraser');});
for(const event of ['pointerup','pointercancel','lostpointercapture'])$('scene').addEventListener(event,finishStroke);
$('scene').addEventListener('contextmenu',event=>event.preventDefault());
$('palette').onclick=event=>{const rect=$('palette').getBoundingClientRect(),x=(event.clientX-rect.left)*256/rect.width,y=(event.clientY-rect.top)*$('palette').height/rect.height;const id=paletteIds[Math.floor(y/32)*8+Math.floor(x/32)];if(id!==undefined){tile=id;chooseTool('brush');drawPalette();}};
async function load(){
  const request=++epoch;loading=true;room=null;buttons();status('Lecture des tiles et du décor original…');
  try {
    const next=await api(`/api/room?room=${$('room').value}&state=${$('state').value}`);
    const image=new Image();image.src=`/api/atlas?tileset=${next.tileset}`;
    const [patch]=await Promise.all([api(`/api/patch?room=${next.id}&state=${next.state}`),image.decode()]);
    if(request!==epoch)return;
    room=next;atlas=image;
    try{localStorage.setItem('sm-editor-location',JSON.stringify({zone:room.zone,room:room.id,state:room.state}));}catch{}
    cells=new Map(patch.cells.map(({x,y,layer,tile})=>[key(layer,x,y),{x,y,layer,tile}]));undo=[];redo=[];dirty=false;
    tile=room.used[0]||0;loading=false;$('tilesetInfo').textContent=`Tileset ${room.tileset.toString(16).toUpperCase()} · ${room.width} × ${room.height} cases`;
    drawPalette();draw();center();status(`${room.name} · palette et tiles de l’état choisi. Collisions en lecture seule.`);
  } catch(error){if(request===epoch){loading=false;status(error.message,true);buttons();}}
}
function discardOK(){return !dirty||window.confirm('Des retouches ne sont pas enregistrées. Changer de salle ou d’état sans les enregistrer ?');}
function states(preferredState){const meta=catalog.find(r=>r.id===Number($('room').value));options($('state'),meta.states,s=>s.id,s=>`${s.label} · ${s.id.toString(16).toUpperCase()}`);$('state').value=meta.states.some(s=>s.id===preferredState)?preferredState:meta.states.at(-1).id;load();}
function rooms(preferredRoom,preferredState){const list=catalog.filter(r=>r.zone===$('zone').value&&r.supported);options($('room'),list,r=>r.id,r=>`${r.name} · ${r.id.toString(16).toUpperCase()}`);if(list.some(r=>r.id===preferredRoom))$('room').value=preferredRoom;else if(list.some(r=>r.id===0x9f11))$('room').value=0x9f11;states(preferredState);}
$('zone').onchange=()=>{if(discardOK())rooms();else $('zone').value=room.zone;};
$('room').onchange=()=>{if(discardOK())states();else $('room').value=room.id;};
$('state').onchange=()=>{if(discardOK())load();else $('state').value=room.state;};
$('save').onclick=async()=>{if(!room||loading)return;finishStroke();const savedEpoch=epoch,snapshot=JSON.stringify({room:room.id,state:room.state,cells:[...cells.values()]});try{const result=await api('/api/patch',{method:'POST',headers:{'Content-Type':'application/json','X-Editor-Token':document.querySelector('meta[name="editor-token"]').content},body:snapshot});if(savedEpoch===epoch&&snapshot===JSON.stringify({room:room.id,state:room.state,cells:[...cells.values()]}))dirty=false;buttons();status(`${result.file} enregistré · Ctrl+F8 dans le jeu pour recharger les décors.`);}catch(error){status(error.message,true);}};
for(const name of ['brush','eraser','picker'])$(name).onclick=()=>chooseTool(name);
$('undo').onclick=()=>history(true);$('redo').onclick=()=>history(false);$('fit').onclick=center;
for(const name of ['grid','collision','original','showBack','showFront'])$(name).onchange=draw;
$('zoom').onchange=()=>{draw();center();};$('used').onchange=drawPalette;$('layer').onchange=drawPalette;
window.addEventListener('beforeunload',event=>{if(dirty){event.preventDefault();event.returnValue='';}});
window.addEventListener('keydown',event=>{
  if((event.ctrlKey||event.metaKey)&&event.key.toLowerCase()==='s'){event.preventDefault();$('save').click();return;}
  if(['INPUT','SELECT','TEXTAREA'].includes(document.activeElement.tagName))return;
  if(event.ctrlKey||event.metaKey){if(event.key.toLowerCase()==='z'){event.preventDefault();history(!event.shiftKey);}return;}
  const select={b:'brush',e:'eraser',i:'picker'}[event.key.toLowerCase()];if(select)chooseTool(select);
});
api('/api/rooms').then(data=>{catalog=data;options($('zone'),[...new Set(data.filter(r=>r.supported).map(r=>r.zone))],x=>x,x=>x);let saved={};try{saved=JSON.parse(localStorage.getItem('sm-editor-location'))||{};}catch{}if(data.some(r=>r.zone===saved.zone&&r.supported))$('zone').value=saved.zone;rooms(saved.room,saved.state);}).catch(error=>status(error.message,true));

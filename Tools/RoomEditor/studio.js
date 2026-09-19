/* Original pixels stay in the ROM. Custom images and notes stay in the patch. */
let nativeBackground=null,paletteRequest=0;
let studioPlanes=[],studioEvents=[],customAtlas=null,customId=null,customDirty=false,customCount=0;
let paletteAtlas=null,paletteSet=-1,customSelected=false,planeImages=new Map(),previewClock=0,previewRunning=false,previewSamus=null;
let eventStart=null,eventEditing=-1,eventPending=null,pixelWork=null,pixelHistory=[],pixelPainting=false;
const canvas=(w,h)=>Object.assign(document.createElement('canvas'),{width:w,height:h});
const planeDefaults=id=>({id,image:null,speedX:100,speedY:100,offsetX:0,offsetY:0,repeatX:true,repeatY:true});
function studioReset(patch){
 studioPlanes=structuredClone(patch.parallax||[]);studioEvents=structuredClone(patch.events||[]);customId=patch.tilesetImage||null;
 customDirty=false;customAtlas=null;customCount=0;customSelected=false;paletteSet=room.tileset;paletteAtlas=atlas;previewSamus=null;planeImages.clear();
 $('paletteSource').value='room';$('allSets').hidden=true;
 options($('planeTarget'),[...room.planes,{id:-1,name:'Fond complet'}],p=>p.id,p=>p.name);
 $('sceneStudio').hidden=!room.planes.length;nativeBackground=null;
 if(studioPlanes.length)$('planeTarget').value=studioPlanes[0].id;
 $('previewX').value=0;$('previewY').value=0;
 $('previewX').max=Math.max(0,room.width*16-400);$('previewY').max=Math.max(0,room.height*16-224);
 planeControls();
}
async function studioLoad(patch,request){
 studioReset(patch);
 if(customId){const img=new Image();img.src='/images/'+customId+'.png';await img.decode();if(request!==epoch)return;customAtlas=canvas(img.width,img.height);customAtlas.getContext('2d').drawImage(img,0,0);customCount=patch.tilesetCount||(img.width/16)*(img.height/16);}
 const bg=new Image();bg.src=`/api/background?room=${room.id}&state=${room.state}`;try{await bg.decode();if(request===epoch)nativeBackground=bg;}catch{}
 await Promise.all(studioPlanes.filter(p=>p.image).map(async p=>{const image=new Image();image.src='/images/'+p.image+'.png';await image.decode();if(request===epoch)planeImages.set(p.image,image);}));
}
function tileFrom(source,value){const c=canvas(16,16),cx=c.getContext('2d');tileImage(cx,value,0,0,1,source);return c;}
function addCustom(image){
 if(customCount>=1024)throw Error('La palette personnalisée contient déjà 1024 tiles.');
 const next=canvas(512,Math.ceil((customCount+1)/32)*16),c=next.getContext('2d');
 if(customAtlas)for(let i=0;i<customCount;i++)c.drawImage(customAtlas,(i%(customAtlas.width/16))*16,Math.floor(i/(customAtlas.width/16))*16,16,16,(i%32)*16,Math.floor(i/32)*16,16,16);
 c.drawImage(image,(customCount%32)*16,Math.floor(customCount/32)*16);customAtlas=next;tile=customCount++;customDirty=true;customSelected=true;
 $('paletteSource').value='custom';$('allSets').hidden=true;paletteAtlas=customAtlas;$('flipX').checked=$('flipY').checked=false;changed();drawPalette();draw();return tile;
}
function selectedCanvas(){return tileFrom(customSelected?customAtlas:paletteAtlas,brushTile());}
function studioBrush(){
 if(!customSelected&&paletteSet!==room.tileset){addCustom(selectedCanvas());}
 return {custom:customSelected,secret:$('secret').checked&&$('layer').value==='0',radius:Number($('radius').value),opacity:Number($('opacity').value)};
}
function selectPainted(cell){
 $('secret').checked=!!cell?.secret;$('radius').value=cell?.radius??64;$('opacity').value=cell?.opacity??25;
 customSelected=!!cell?.custom;$('paletteSource').value=customSelected?'custom':'room';paletteAtlas=customSelected?customAtlas:atlas;paletteSet=room.tileset;$('allSets').hidden=true;
}
async function sourceChange(){
 if(!room||loading)return;const choice=++paletteRequest,source=$('paletteSource').value;$('allSets').hidden=source!=='all';customSelected=source==='custom';
 if(customSelected){paletteAtlas=customAtlas;tile=0;drawPalette();return;}
 const id=source==='room'?room.tileset:Number($('allSet').value),request=epoch;const img=new Image();img.src='/api/atlas?tileset='+id;
 try{await img.decode();if(request!==epoch||choice!==paletteRequest||$('paletteSource').value!==source)return;paletteAtlas=img;paletteSet=id;tile=0;drawPalette();}catch{status('Ce tileset n’a pas pu être décodé.',true);}
}
function planeControls(){
 const p=studioPlanes.find(p=>p.id===Number($('planeTarget').value))||planeDefaults(Number($('planeTarget').value));
 for(const k of ['speedX','speedY','offsetX','offsetY'])$(k).value=p[k];for(const k of ['repeatX','repeatY'])$(k).checked=p[k];renderPreview();
}
function updatePlane(){
 if(!room||loading)return;const id=Number($('planeTarget').value);
 let p=studioPlanes.find(p=>p.id===id)||planeDefaults(id);
 studioPlanes=studioPlanes.filter(p=>id===-1?p.id===-1:p.id!==-1);
 if(!studioPlanes.includes(p))studioPlanes.push(p);
 for(const k of ['speedX','speedY','offsetX','offsetY'])p[k]=Number($(k).value);for(const k of ['repeatX','repeatY'])p[k]=$(k).checked;
 changed();renderPreview();return p;
}
async function upload(blob,kind){
 const response=await fetch('/api/image?kind='+kind,{method:'POST',headers:{'Content-Type':'image/png','X-Editor-Token':document.querySelector('meta[name="editor-token"]').content},body:blob});
 const data=await response.json();if(!response.ok)throw Error(data.error);return data;
}
async function studioSerialize(){
 if(customAtlas&&customDirty){const image=await upload(await new Promise(r=>customAtlas.toBlob(r,'image/png')),'tileset');customId=image.id;customDirty=false;}
 return {parallax:studioPlanes,events:studioEvents,tilesetImage:customId,tilesetCount:customCount};
}
function secretAlpha(cell,sx,sy){
 if(!cell.secret)return 1;
 const dx=Math.max(cell.x*16-sx,0,sx-(cell.x+1)*16),dy=Math.max(cell.y*16-sy,0,sy-(cell.y+1)*16);
 const t=Math.min(1,Math.hypot(dx,dy)/(cell.radius||64)),a=(cell.opacity??25)/100;
 return a+(1-a)*t*t*(3-2*t);
}
function studioEventsDraw(c,unit){
 if($('original').checked)return;
 c.save();c.strokeStyle='#ffd77d';c.fillStyle='#ffd77d22';c.lineWidth=2;
 for(const e of studioEvents){const x=(e.x+margin)*unit,y=(e.y+margin)*unit;c.fillRect(x,y,e.width*unit,e.height*unit);c.strokeRect(x,y,e.width*unit,e.height*unit);c.fillStyle='#ffe7a4';c.font='12px system-ui';c.fillText(e.note.slice(0,45),x+3,y+14);c.fillStyle='#ffd77d22';}c.restore();
}
function eventPointer(point){
 eventStart=point;eventEditing=studioEvents.findIndex(e=>point.x>=e.x&&point.x<e.x+e.width&&point.y>=e.y&&point.y<e.y+e.height);
}
function finishEvent(point){
 if(!eventStart)return;
 const x=Math.max(-margin,Math.min(eventStart.x,point.x)),y=Math.max(-margin,Math.min(eventStart.y,point.y));
 eventPending=eventEditing>=0?structuredClone(studioEvents[eventEditing]):{x,y,width:Math.min(room.width+margin-1,Math.max(eventStart.x,point.x))-x+1,height:Math.min(room.height+margin-1,Math.max(eventStart.y,point.y))-y+1,note:''};eventStart=null;
 $('eventLocation').textContent=`Cases ${eventPending.x}, ${eventPending.y} · ${eventPending.width} × ${eventPending.height}`;$('eventNote').value=eventPending.note;$('eventDelete').disabled=eventEditing<0;$('eventDialog').showModal();
}
function renderPreview(){
 if(!room||!atlas||$('sceneStudio').hidden||!$('sceneStudio').open)return;
 const pctx=$('preview').getContext('2d');pctx.imageSmoothingEnabled=false;pctx.fillStyle='#000';pctx.fillRect(0,0,400,224);
 const cameraX=Number($('previewX').value)+(previewRunning?Math.sin(previewClock)*48:0),cx=cameraX-72,cy=Number($('previewY').value)+(previewRunning?Math.cos(previewClock*.7)*16:0);
 const sx=previewSamus?.x??200,sy=previewSamus?.y??112;
 if(nativeBackground){
  for(const band of room.planes){
   const cfg=studioPlanes.find(p=>p.id===-1)||studioPlanes.find(p=>p.id===band.id)||planeDefaults(band.id);
   let shiftX=Math.max(-8,Math.min(8,-(cameraX-Number($('previewX').value))/8));
   let shiftY=Math.max(-6,Math.min(6,-(cy-Number($('previewY').value))/12));
   if(room.id===0x91f8){shiftX=band.id===0?shiftX/3:(band.id===1?shiftX*2/3:shiftX);shiftY=0;}
   shiftX=Math.trunc(shiftX*cfg.speedX/100);shiftY=Math.trunc(shiftY*cfg.speedY/100);
   const nx=(room.scrollX&1)?cameraX*room.scrollX/256:cameraX,ny=(room.scrollY&1)?cy*room.scrollY/256:cy;
   pctx.save();pctx.beginPath();pctx.rect(0,band.start-cy,400,band.end-band.start);pctx.clip();
   let ox=72-Math.round(nx)-shiftX,oy=-Math.round(ny)-shiftY;
   ox=((ox%nativeBackground.width)+nativeBackground.width)%nativeBackground.width-nativeBackground.width;
   oy=((oy%nativeBackground.height)+nativeBackground.height)%nativeBackground.height-nativeBackground.height;
   for(let y=oy;y<224;y+=nativeBackground.height)for(let x=ox;x<400;x+=nativeBackground.width)pctx.drawImage(nativeBackground,x,y);
   pctx.restore();
  }
 }
 for(const l of [1,0]){
  if(!$(l===0?'showFront':'showBack').checked)continue;
  if(l===0){
   for(const cfg of studioPlanes){
    const image=planeImages.get(cfg.image);if(!image)continue;
    const band=room.planes.find(p=>p.id===cfg.id);pctx.save();
    if(band){pctx.beginPath();pctx.rect(0,band.start-cy,400,band.end-band.start);pctx.clip();}
    let ox=cfg.offsetX+72-Math.round(cameraX*cfg.speedX/100),oy=cfg.offsetY-Math.round(cy*cfg.speedY/100);
    const startX=cfg.repeatX?((ox%image.width)+image.width)%image.width-image.width:ox,startY=cfg.repeatY?((oy%image.height)+image.height)%image.height-image.height:oy;
    for(let y=startY;y<224;y+=image.height){for(let x=startX;x<400;x+=image.width){pctx.drawImage(image,x,y);if(!cfg.repeatX)break;}if(!cfg.repeatY)break;}
    pctx.restore();
   }
  }
  const base=room[l===0?'foreground':'background'];
  for(let y=Math.max(-margin,Math.floor(cy/16));y<Math.min(room.height+margin,Math.ceil((cy+224)/16));y++)for(let x=Math.max(-margin,Math.floor(cx/16));x<Math.min(room.width+margin,Math.ceil((cx+400)/16));x++){
   const cell=cells.get(key(l,x,y)),value=x>=0&&y>=0&&x<room.width&&y<room.height?base[y*room.width+x]:undefined;
   if(value!==undefined&&(l===0||!nativeBackground)&&(!cell||cell.custom||cell.secret))tileImage(pctx,value,Math.round(x*16-cx),Math.round(y*16-cy),1);
   if(cell){pctx.save();pctx.globalAlpha=secretAlpha(cell,cx+sx,cy+sy);tileImage(pctx,cell.tile,Math.round(x*16-cx),Math.round(y*16-cy),1,cell.custom?customAtlas:atlas);pctx.restore();}
  }
 }
 pctx.strokeStyle='#81f3e6';pctx.strokeRect(sx-5,sy-12,10,24);pctx.fillStyle='#81f3e6';pctx.font='9px monospace';pctx.fillText('SAMUS',sx-14,sy-16);
}
function animate(t){if(previewRunning){previewClock=t/1000;renderPreview();}requestAnimationFrame(animate);}requestAnimationFrame(animate);
function drawPixel(){const c=$('pixelCanvas').getContext('2d');c.clearRect(0,0,256,256);c.imageSmoothingEnabled=false;c.drawImage(pixelWork,0,0,256,256);c.strokeStyle='#ffffff25';c.beginPath();for(let i=0;i<=256;i+=16){c.moveTo(i,0);c.lineTo(i,256);c.moveTo(0,i);c.lineTo(256,i);}c.stroke();$('pixelUndo').disabled=!pixelHistory.length;}
function pixelAt(e){const r=$('pixelCanvas').getBoundingClientRect(),x=Math.floor((e.clientX-r.left)*16/r.width),y=Math.floor((e.clientY-r.top)*16/r.height);if(x<0||y<0||x>15||y>15)return;const c=pixelWork.getContext('2d');if($('pixelErase').checked)c.clearRect(x,y,1,1);else{c.fillStyle=$('pixelColor').value;c.fillRect(x,y,1,1);}drawPixel();}
function rotate(source){const c=canvas(source.height,source.width),cx=c.getContext('2d');cx.translate(c.width,0);cx.rotate(Math.PI/2);cx.drawImage(source,0,0);return c;}
function catchUI(action){return async()=>{try{if(!room||loading)return;await action();}catch(e){status(e.message,true);}};}
$('paletteSource').onchange=sourceChange;$('allSet').onchange=sourceChange;
fetch('/api/tilesets').then(r=>r.json()).then(rows=>options($('allSet'),rows,r=>r.id,r=>r.name)).catch(e=>status(e.message,true));
$('newTile').onclick=catchUI(()=>addCustom(canvas(16,16)));
$('blackTile').onclick=catchUI(()=>{const c=canvas(16,16);c.getContext('2d').fillRect(0,0,16,16);addCustom(c);});
$('editTile').onclick=catchUI(()=>{pixelWork=tileFrom(customSelected?customAtlas:paletteAtlas,tile);pixelHistory=[];drawPixel();$('pixelDialog').showModal();});
$('pixelCanvas').onpointerdown=e=>{e.preventDefault();pixelHistory.push(pixelWork.getContext('2d').getImageData(0,0,16,16));if(pixelHistory.length>100)pixelHistory.shift();pixelPainting=true;$('pixelCanvas').setPointerCapture(e.pointerId);pixelAt(e);};
$('pixelCanvas').onpointermove=e=>{if(pixelPainting)pixelAt(e);};$('pixelCanvas').onpointerup=$('pixelCanvas').onpointercancel=()=>{pixelPainting=false;};
$('pixelUndo').onclick=()=>{if(pixelHistory.length){pixelWork.getContext('2d').putImageData(pixelHistory.pop(),0,0);drawPixel();}};
$('pixelApply').onclick=catchUI(()=>{if(customSelected){const c=customAtlas.getContext('2d'),x=(tile%(customAtlas.width/16))*16,y=Math.floor(tile/(customAtlas.width/16))*16;c.clearRect(x,y,16,16);c.drawImage(pixelWork,x,y);customDirty=true;changed();drawPalette();draw();}else addCustom(pixelWork);$('pixelDialog').close();});
$('pixelCancel').onclick=()=>$('pixelDialog').close();
$('rotateTile').onclick=catchUI(()=>addCustom(rotate(selectedCanvas())));
$('rotateSet').onclick=catchUI(()=>{if(!paletteAtlas)throw Error('Choisir un tileset.');if(!confirm('Créer une palette personnalisée pivotée de 90° ? Les tiles personnalisées déjà peintes utiliseront cette nouvelle palette.'))return;customAtlas=rotate(paletteAtlas);customCount=customAtlas.width*customAtlas.height/256;customSelected=true;customDirty=true;tile=0;paletteAtlas=customAtlas;$('paletteSource').value='custom';$('allSets').hidden=true;changed();drawPalette();draw();});
$('importTiles').onclick=()=>$('tilesFile').click();$('tilesFile').onchange=catchUI(async()=>{const request=epoch,file=$('tilesFile').files[0];if(!file)return;if(customCount&&!confirm('Remplacer la palette personnalisée ? Les indices déjà peints seront conservés.'))return;const result=await upload(file,'tileset'),img=new Image();img.src=result.url;await img.decode();if(request!==epoch||loading)return;customAtlas=canvas(img.width,img.height);customAtlas.getContext('2d').drawImage(img,0,0);customCount=img.width*img.height/256;customId=result.id;customDirty=false;customSelected=true;paletteAtlas=customAtlas;tile=0;$('paletteSource').value='custom';$('allSets').hidden=true;changed();drawPalette();draw();});
$('exportTiles').onclick=catchUI(()=>{const c=canvas(paletteAtlas.width,paletteAtlas.height);c.getContext('2d').drawImage(paletteAtlas,0,0);const a=document.createElement('a');a.href=c.toDataURL('image/png');a.download='zebes-tileset.png';a.click();});
$('planeTarget').onchange=planeControls;
for(const k of ['speedX','speedY','offsetX','offsetY','repeatX','repeatY'])$(k).onchange=updatePlane;
$('importBackground').onclick=()=>$('backgroundFile').click();$('backgroundFile').onchange=catchUI(async()=>{const request=epoch,target=$('planeTarget').value,file=$('backgroundFile').files[0];if(!file)return;const result=await upload(file,'background'),image=new Image();image.src=result.url;await image.decode();if(request!==epoch||loading||$('planeTarget').value!==target)return;const p=updatePlane();p.image=result.id;planeImages.set(result.id,image);changed();renderPreview();});
$('clearBackground').onclick=catchUI(()=>{studioPlanes=studioPlanes.filter(p=>p.id!==Number($('planeTarget').value));changed();planeControls();});
$('animatePreview').onclick=()=>{previewRunning=!previewRunning;$('animatePreview').textContent=previewRunning?'Arrêter la caméra':'Animer la caméra';renderPreview();};
$('sceneStudio').ontoggle=renderPreview;for(const id of ['previewX','previewY'])$(id).oninput=renderPreview;
$('preview').onpointerdown=e=>{const r=$('preview').getBoundingClientRect();previewSamus={x:(e.clientX-r.left)*400/r.width,y:(e.clientY-r.top)*224/r.height};renderPreview();};
$('eventTool').onclick=()=>chooseTool('event');
$('eventApply').onclick=()=>{const note=$('eventNote').value.trim();if(!note)return;eventPending.note=note;if(eventEditing>=0)studioEvents[eventEditing]=eventPending;else studioEvents.push(eventPending);changed();draw();$('eventDialog').close();};
$('eventDelete').onclick=()=>{if(eventEditing>=0){studioEvents.splice(eventEditing,1);changed();draw();}$('eventDialog').close();};$('eventCancel').onclick=()=>$('eventDialog').close();

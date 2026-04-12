#pragma once

inline const char init_setup_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Initial Setup Panel</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<style>
body {
  font-family: system-ui, sans-serif;
  background: #dddddd;
  margin: 0;
  padding: 20px;
  color: #333;
}

::selection {
  background: transparent;
  color: inherit;
}

#layout {
  display: flex;
  justify-content: center;
  align-items: flex-start;
  gap: 20px;        /* távolság a két oszlop között */
}

#left, #right {
  flex: 0 1 auto;   /* <<< NEM nyúlik szét, NEM lesz keskenyebb */
  display: flex;
  flex-direction: column;
  align-items: center;
}

.card {
  background: #fff;
  border-radius: 8px;
  box-shadow: 0 2px 6px rgba(0,0,0,0.05);
  padding: 20px;
  margin-bottom: 20px;
  min-width: 28rem;
  max-width: 37.5rem;
}

h3 {
  margin-top: 0;
  font-size: 1.2em;
  color: #222;
}

ul {
  list-style: none;
  padding: 0;
  margin: 10px 0;
  border: 1px solid #ddd;
  border-radius: 6px;
  background: #fff;
  max-height: 200px;
  overflow-y: auto;
}

li {
  padding: 8px 12px;
  border-bottom: 1px solid #eee;
  cursor: pointer;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

li:last-child {
  border-bottom: none;
}

li.sel {
  background: #e0f0ff;
}

li.up {
  font-weight: bold;
}

.name {
  flex: 1;
}

.size {
  font-family: monospace;
  color: #888;
  white-space: nowrap;
}

button {
  background: #007bff;
  color: white;
  border: none;
  padding: 8px 14px;
  margin: 5px 4px 0 0;
  border-radius: 6px;
  cursor: pointer;
  font-size: 0.9em;
}

button.red {
  background: #c00;
}

button.disabled {
  background: #ccc;
  cursor: default;
}

#flash-status {
  height: 20px;
  background: #eee;
  border-radius: 6px;
  overflow: hidden;
  margin-bottom: 10px;
  border: 1px solid #ccc;
}

#flash-bar {
  height: 100%;
  background: #007bff;
  width: 0%;
}

#flash-info {
  font-size: 0.9em;
  color: #555;
  margin-bottom: 8px;
}

#drop {
  border: 2px dashed #bbb;
  border-radius: 6px;
  padding: 20px;
  text-align: center;
  color: #888;
  margin-bottom: 10px;
}

/* WiFi form alignment */
label {
  display: block;
  margin-top: 10px;
}

.ip-group input {
  width: 25px;
  text-align: center;
}

.mac-group input {
  width: 25px;
  text-align: center;
}

.port-group input {
  width: 3em;
  text-align: center;
}

.hidden {
  display: none;
}
</style>

</head>

<body style="display:none;">

<center><h1>Initial Setup Panel</h1></center>

<div id="layout">
  <div id="left">

<!-- FLASH FILE MANAGER -->
<div class="card">
  <h3>Flash Files</h3>
  <div>Path: <span id="path">/</span></div>
  <ul id="files"></ul>
  <div>
    <button id="ed" class="disabled" onclick="editFile();">EDIT</button>
    <button onclick="mkdir();">MKDIR</button>
    <button id="rn" class="disabled" onclick="ren();">RENAME</button>
    <button id="rm" class="disabled" onclick="del();">DELETE</button>
    <button id="dl" class="disabled" onclick="download();">DOWNLOAD</button>
    <button class="red" onclick="formatFS();">FORMAT</button>
  </div>
</div>

<div class="card">
  <h3>Upload</h3>

  <div id="flash-info">Used: 0 kB / 0 kB</div>

  <div id="flash-status"><div id="flash-bar"></div></div>

  <br>

  <div id="drop">Drag & Drop files here</div>

  <input type="file" id="f" multiple>
  <button onclick="uploadBtn()">Upload</button>
</div>

<form id="dlForm" method="POST" action="/download" target="_blank">
  <input type="hidden" name="path" id="dlPath">
</form>


</div>
<div id="right">
<!-- WIFI SETUP -->
<div class="card">
  <h3>WiFi Setup</h3>

  <form method="POST" action="/wifisave">

    <label>Mode:
      <select name="mode" onchange="updateMode()">
        <option value="1" ~MODE_AP~>Access Point (AP)</option>
        <option value="2" ~MODE_STA~>Client (STA)</option>
        <option value="3" ~MODE_EXT~ ~EXTENDER_ENABLE~>Range Extender</option>
      </select>
    </label>

    <!-- AP MODE -->
    <div id="ap">
      <h4>AP Mode</h4>

      <label>SSID: 
        <input name="ap_ssid" value="~AP_SSID~">
        <select name="ap_hidden">
          <option value="0" ~AP_HIDDEN_OFF~>Visible</option>
          <option value="1" ~AP_HIDDEN_ON~>Hidden</option>
        </select>
      </label>
        
      <label>PASS: <input name="ap_pass" value="~AP_PASS~"></label>

      <label>Channel: 
        <select name="ap_channel">
          <option value="16">EU AUTO</option>
          <option value="17">USA AUTO</option>
          <option value="1">1</option>
          <option value="2">2</option>
          <option value="3">3</option>
          <option value="4">4</option>
          <option value="5">5</option>
          <option value="6">6</option>
          <option value="7">7</option>
          <option value="8">8</option>
          <option value="9">9</option>
          <option value="10">10</option>
          <option value="11">11</option>
          <option value="12">12</option>
          <option value="13">13</option>
          <option value="14">14 (JP!!!)</option>
        </select>
        <select name="ap_power">
          <option value="1">2dBm</option>
          <option value="2">5dBm</option>
          <option value="3">8dBm</option>
          <option value="4">11dBm</option>
          <option value="5">14dBm</option>
          <option value="6">17dBm</option>
          <option value="7">20dBm</option>
        </select>
      </label>
      
      <label> 
        <select name="ap_captive" onchange="updateDom();">
          <option value="0" ~AP_CAPTIVE_OFF~>Captibe portal: OFF</option>
          <option value="1" ~AP_CAPTIVE_ON~>Captibe portal: ON</option>
        </select>
        <input name="ap_domain" value="~AP_DOMAIN~">
      </label>

      <label>IP:
        <span class="ip-group">
          <input name="ap_ip0" value="~AP_IP0~"> .
          <input name="ap_ip1" value="~AP_IP1~"> .
          <input name="ap_ip2" value="~AP_IP2~"> .
          <input name="ap_ip3" value="~AP_IP3~">
        </span>
      </label>

      <label>Port: 
        <select name="ap_https" onchange="updateAPhttps()">
          <option value="0" ~AP_HTTPS_OFF~>HTTP</option>
          <!--option value="1" ~AP_HTTPS_ON~>HTTPS</option-->
        </select>
        <span class="port-group"><input name="ap_port" value="~AP_PORT~"></span>
      </label>
      
    </div>

    <!-- STA MODE -->
    <div id="sta">
      <h4>STA Mode</h4>

      <label>SSID: <input name="sta_ssid" value="~STA_SSID~"></label>
      <label>PASS: <input name="sta_pass" value="~STA_PASS~"></label>

      <label>DHCP:
        <select name="sta_dhcp" onchange="updateDhcp()">
          <option value="1" ~STA_DHCP_ON~>Enabled</option>
          <option value="0" ~STA_DHCP_OFF~>Static</option>
        </select>
      </label>

      <div id="sta_static_fields">

        <label>IP:
          <span class="ip-group">
            <input name="sta_ip0" value="~STA_IP0~"> .
            <input name="sta_ip1" value="~STA_IP1~"> .
            <input name="sta_ip2" value="~STA_IP2~"> .
            <input name="sta_ip3" value="~STA_IP3~">
          </span>
        </label>

        <label>Gateway:
          <span class="ip-group">
            <input name="sta_gw0" value="~STA_GW0~"> .
            <input name="sta_gw1" value="~STA_GW1~"> .
            <input name="sta_gw2" value="~STA_GW2~"> .
            <input name="sta_gw3" value="~STA_GW3~">
          </span>
        </label>

        <label>Mask:
          <span class="ip-group">
            <input name="sta_m0" value="~STA_M0~"> .
            <input name="sta_m1" value="~STA_M1~"> .
            <input name="sta_m2" value="~STA_M2~"> .
            <input name="sta_m3" value="~STA_M3~">
          </span>
        </label>

        <label>DNS:
          <span class="ip-group">
            <input name="sta_d0" value="~STA_D0~"> .
            <input name="sta_d1" value="~STA_D1~"> .
            <input name="sta_d2" value="~STA_D2~"> .
            <input name="sta_d3" value="~STA_D3~">
          </span>
        </label>

      </div>

      <label >Port: 
        <select name="sta_https" onchange="updateSTAhttps()">
          <option value="0" ~STA_HTTPS_OFF~>HTTP</option>
          <!--option value="1" ~STA_HTTPS_ON~>HTTPS</option-->
        </select>
        <span class="port-group"><input name="sta_port" value="~STA_PORT~"></span>
      </label>
      
    </div>

    <!-- EXT MODE -->
    <div id="ext">
      <h4>EXT Mode
        <select name="ext_channel">
          <option value="16">EU</option>
          <option value="17">USA</option>
        </select>      
      </h4>

      <label>Router SSID: <input name="ext_ssid" value="~EXT_SSID~">&emsp;~EXT_CONNECTED~</label>
      <label>Router PASS: <input name="ext_pass" value="~EXT_PASS~"></label>
      <label>New SSID: 
        <input name="ext_newssid" value="~EXT_NEWSSID~">
        <select name="ext_hidden">
          <option value="0" ~EXT_HIDDEN_OFF~>Visible</option>
          <option value="1" ~EXT_HIDDEN_ON~>Hidden</option>
        </select>
      </label>
      <label>New PASS: <input name="ext_newpass" value="~EXT_NEWPASS~"></label>

      <label>IP:
        <span class="ip-group">
          <input name="ext_ip0" value="~EXT_IP0~"> .
          <input name="ext_ip1" value="~EXT_IP1~"> .
          <input name="ext_ip2" value="~EXT_IP2~"> .
          <input name="ext_ip3" value="~EXT_IP3~">
        </span>
      </label>

      <label>Mask:
        <span class="ip-group">
          <input name="ext_m0" value="~EXT_M0~"> .
          <input name="ext_m1" value="~EXT_M1~"> .
          <input name="ext_m2" value="~EXT_M2~"> .
          <input name="ext_m3" value="~EXT_M3~">
        </span>
      </label>

      <label>DNS:
        <span class="ip-group">
          <input name="ext_d0" value="~EXT_D0~"> .
          <input name="ext_d1" value="~EXT_D1~"> .
          <input name="ext_d2" value="~EXT_D2~"> .
          <input name="ext_d3" value="~EXT_D3~">
        </span>
      </label>

      <label>Start DHCP:
        <span class="ip-group">
          <input name="ext_s0" value="~EXT_S0~"> .
          <input name="ext_s1" value="~EXT_S1~"> .
          <input name="ext_s2" value="~EXT_S2~"> .
          <input name="ext_s3" value="~EXT_S3~">
        </span>
      </label>
      
    </div>

      <label>
        <select name="mac_mode" onchange="updateMac()" style = "margin-right:1em;">
          <option value="0">MAC: Original</option>
          <option value="1">MAC: Random</option>
          <option value="2">MAC: Custom</option>
        </select>
        <span id="macInputs" class="mac-group">
          <input name="mac0" value="~MAC0~"> -
          <input name="mac1" value="~MAC1~"> -
          <input name="mac2" value="~MAC2~"> -
          <input name="mac3" value="~MAC3~"> -
          <input name="mac4" value="~MAC4~"> -
          <input name="mac5" value="~MAC5~">
        </span>
      </label>

    <br>
    <button type="submit">Save</button>

  </form>
</div>

<!-- ADMIN PASSWORD -->
<div class="card">
  <h3>Admin Password</h3>
  <form method="POST" action="/passwdsave">
    <label>Password: 
     <input name="pass_admin" value="~PWD_ADMIN~">
     <button type="submit">Save</button>
     </label>
  </form>
</div>

  </div>
</div>

<center>
  <button onclick="window.location.href='/'">Exit</button>
  <button onclick="window.location.href='/reboot'">Reboot</button>
</center>

<script>

document.addEventListener("DOMContentLoaded", () => {
    function clampNumeric(input, min, max) {
        input.setAttribute("inputmode", "numeric");
        input.addEventListener("input", () => {
            input.value = input.value.replace(/[^0-9]/g, "");
            if (input.value === "") return;
            let v = parseInt(input.value, 10);
            if (v < min || v > max) v = min;
            input.value = v;
        });
    }
    document.querySelectorAll(".ip-group input")
        .forEach(i => clampNumeric(i, 0, 255));
    document.querySelectorAll(".port-group input")
        .forEach(i => clampNumeric(i, 0, 65535));
});

document.addEventListener("DOMContentLoaded", () => {
    document.querySelectorAll(".mac-group input").forEach(input => {
        input.setAttribute("inputmode", "numeric");
        input.addEventListener("input", () => {
            let v = input.value.replace(/[^0-9a-fA-F]/g, "");
            // Nagybetű
            v = v.toUpperCase();
            // Max 2 karakter
            if (v.length > 2) v = v.substring(0, 2);
            input.value = v;
        });
        input.addEventListener("blur", () => {
            let v = input.value;
            if (v.length === 1) {
                input.value = "0" + v;   // pl. "A" → "0A"
            }
            if (v.length === 0) {
                input.value = "00";      // üres → "00"
            }
        });
    });
});


/* MODE SWITCH */
function updateMode() {
  const mode = document.querySelector('select[name="mode"]').value;

  document.getElementById("ap").classList.toggle("hidden", mode !== "1");
  document.getElementById("sta").classList.toggle("hidden", mode !== "2");
  document.getElementById("ext").classList.toggle("hidden", mode !== "3");

  document.querySelector('select[name="ap_channel"]').value = ~AP_CHANNEL~;
  document.querySelector('select[name="ap_power"]').value = ~AP_POWER~;
  document.querySelector('select[name="ext_channel"]').value = ~EXT_CHANNEL~;
  document.querySelector('select[name="mac_mode"]').value = ~MAC_MODE~;

  updateDhcp();
  updateDom();
  updateMac();
}

function updateDhcp() {
  const dhcp = document.querySelector('select[name="sta_dhcp"]').value;
  document.getElementById("sta_static_fields").classList.toggle("hidden", dhcp === "1");
}
function updateDom() {
  const cap = document.querySelector('select[name="ap_captive"]');
  const dom = document.querySelector('input[name="ap_domain"]');
  dom.style.display = cap.value === "1" ? "" : "none";
}
function updateMac() {
    const mode = document.querySelector('select[name="mac_mode"]').value;
    const macInputs = document.getElementById('macInputs');
    if (mode === "2") {
        macInputs.style.display = "inline";
    } else {
        macInputs.style.display = "none";
    }
}
function updateAPhttps() {
  document.querySelector('input[name="ap_port"]').value =
    document.querySelector('select[name="ap_https"]').value === "1" ? "443" : "80";
}
function updateSTAhttps() {
  document.querySelector('input[name="sta_port"]').value =
    document.querySelector('select[name="sta_https"]').value === "1" ? "443" : "80";
}

/* FLASH FILE MANAGER JS — unchanged */
let cwd="/", sel=null, selIsDir=false, selIsSystem=false;

function isProtectedRootFolder(name) {
  return (cwd === "/" && (name === "core" || name === "system"));
}

function fmtSize(sz){
  if(sz<1024) return sz+" B";
  return (sz/1024).toFixed(1)+" kB";
}

function btn(e,dir){
 const edit=document.getElementById('ed');
 const rn=document.getElementById('rn');
 const rm=document.getElementById('rm');
 const dl=document.getElementById('dl');
 const disable = !e || selIsSystem;// || selIsUp;
 toggle(edit,!e||dir);
 toggle(rn,disable);
 toggle(rm,disable);
 toggle(dl,!e);
 function toggle(el,d){ d?el.classList.add('disabled'):el.classList.remove('disabled'); }
}

function renderEntry(f,type){
  let ul=document.getElementById('files');
  let li=document.createElement('li');
  let icon=f.dir?'📁 ':'📄 ';
  if(type==="system") icon='⚙️ ';
  if(type==="core") icon='🔺 ';
  let name=document.createElement('span');
  name.className='name';
  name.textContent=icon+f.name;
  let size=document.createElement('span');
  size.className='size';
  size.textContent=f.dir?"":fmtSize(f.size);
  li.appendChild(name);
  li.appendChild(size);

  const filePath = cwd + (cwd.endsWith('/')?'':'/') + f.name;

  li.onclick=()=>{
    [...ul.children].forEach(x=>x.classList.remove('sel'));
    li.classList.add('sel');

    sel = filePath;
    selIsDir = f.dir;

    selIsSystem = isProtectedRootFolder(f.name);

    btn(true, f.dir);
  };

  li.ondblclick=e=>{
    e.preventDefault();
    if(f.dir){
      cwd=filePath;
      load();
    } else {
      window.open(filePath,'_blank');
    }
  };

  ul.appendChild(li);
}

function load(){
  document.getElementById('path').textContent=cwd;
  loadFlashStatus();

  fetch('/list_s',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'path='+encodeURIComponent(cwd)
  })
  .then(r=>r.json())
  .then(j=>{
    let ul=document.getElementById('files');
    ul.innerHTML=''; sel=null; selIsSystem=false; btn(false,false);

    let li=document.createElement('li');
    li.textContent='📁 ..';
    li.className='up';

    if(cwd !== "/") {
      li.ondblclick = () => {
        cwd = cwd.substring(0, cwd.lastIndexOf('/')) || "/";
        load();
      };
    } else {
      li.onclick = () => {
        [...ul.children].forEach(x=>x.classList.remove('sel'));
        li.classList.add('sel');
        sel = "/";
        selIsDir = true;
        selIsSystem = false;
        btn(false,false);
        document.getElementById('dl').classList.remove('disabled');
      };
    }

    ul.appendChild(li);

    let core=null,system=null,others=[];
    j.forEach(f=>{
      if(cwd==="/"&&f.dir&&f.name==="core") core=f;
      else if(cwd==="/"&&f.dir&&f.name==="system") system=f;
      else others.push(f);
    });

    if(core)renderEntry(core,"core");
    if(system)renderEntry(system,"system");
    others.forEach(f=>renderEntry(f,"normal"));

   document.body.style.display = "block";
  });
}

function loadFlashStatus(){
  fetch('/flashfree').then(r=>r.json()).then(j=>{
    let bar=document.getElementById('flash-bar');
    let info=document.getElementById('flash-info');

    let percent=(j.used/j.total)*100;
    bar.style.width=percent+'%';

    info.textContent = "Used: " + j.used + " kB / " + j.total + " kB";
  });
}

function checkUploadSize(files,ok){
  let total=0;
  for(let i=0;i<files.length;i++) total+=files[i].size;
  fetch('/flashfree').then(r=>r.json()).then(j=>{
    let free=(j.total-j.used)*1024;
    if(total>free){ alert("Not enough flash space"); return; }
    ok();
  });
}

function uploadFiles(files){
  let i=0;
  let timer=setInterval(loadFlashStatus,300);
  (function next(){
    if(i>=files.length){ clearInterval(timer); load(); return; }
    upload(files[i++],next);
  })();
}

function upload(file,done){
  let x=new XMLHttpRequest();
  x.onload=()=>done&&done();
  x.open('POST','/upload?path='+encodeURIComponent(cwd)+'&name='+encodeURIComponent(file.name));
  x.send(file);
}

function uploadBtn(){
  let files=document.getElementById('f').files;
  if(files.length) checkUploadSize(files,()=>uploadFiles(files));
}

const drop = document.getElementById('drop');
drop.ondragover=e=>e.preventDefault();
drop.ondrop=e=>{
  e.preventDefault();
  let files=e.dataTransfer.files;
  if(files.length) checkUploadSize(files,()=>uploadFiles(files));
};

function mkdir(){
  let n=prompt("Dir name:"); if(!n)return;
  fetch('/mkdir',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'path='+encodeURIComponent(cwd+"/"+n)
  }).then(load);
}

function del(){
  if(selIsSystem || !sel || !confirm("Delete?")) return;

  fetch('/delete',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'path='+encodeURIComponent(sel)
  }).then(load);
}

function ren(){
  if(selIsSystem || !sel) return;

  let base = sel.substring(0, sel.lastIndexOf('/'));
  let n = prompt("New name:", sel.split('/').pop());
  if (!n) return;

  fetch('/rename',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'old='+encodeURIComponent(sel)+'&new='+encodeURIComponent(base+'/'+n)
  }).then(load);
}

function download() {
  if (!sel) return;
  document.getElementById("dlPath").value = sel;
  document.getElementById("dlForm").submit();
}

function editFile() {
 if(!sel||selIsDir)return;
 let f=document.createElement('form');
 f.method='POST'; f.action='/edit';
 let i=document.createElement('input');
 i.name='path'; i.value=sel;
 f.appendChild(i); document.body.appendChild(f);
 f.submit(); document.body.removeChild(f);
}

function formatFS(){
  if(!confirm("Format LittleFS? All data will be lost!")) return;

  fetch('/format',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
  }).then(formatAfter);
}

function formatAfter(){
  const yes = confirm("Do you want to keep the current settings?");
  if (!yes) {
    openWithPost("/default"); 
    return;
    }
  openWithPost("/saveall"); 
}

function openWithPost(url, data = {}) {
    const form = document.createElement("form");
    form.method = "POST";
    form.action = url;
    for (const key in data) {
        const input = document.createElement("input");
        input.type = "hidden";
        input.name = key;
        input.value = data[key];
        form.appendChild(input);
    }
    document.body.appendChild(form);
    form.submit();
}

load();
updateMode();


</script>

</body>
</html>

)rawliteral";



inline const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="hu">
<head>
  <meta charset="UTF-8">
  <title>ESPOS</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      margin: 0;
      padding: 0;
      font-family: sans-serif;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      background: #111;
      color: #f5f5f5;
    }
    h1 {
      font-size: 3rem;
      margin-bottom: 2rem;
      letter-spacing: 0.2rem;
    }
    button {
      padding: 0.8rem 1.8rem;
      font-size: 1rem;
      border-radius: 4px;
      border: none;
      cursor: pointer;
      background: #00aaff;
      color: #fff;
    }
    button:active {
      transform: scale(0.97);
    }
  </style>
</head>
<body>
  <h1>ESPOS</h1>
  <button onclick="location.href='/admin'">Megnyitás</button>
</body>
</html>
)rawliteral";


inline const char reboot_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="hu">
<head>
  <meta charset="UTF-8">
  <title>Várakozás</title>
  <style>
    body {
      margin: 0;
      font-family: sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      background: #dddddd;
    }
    .container {
      text-align: center;
      background: #fff;
      padding: 20px 30px;
      border-radius: 8px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.15);
      max-width: 400px;
      width: 100%;
    }
    #message {
      margin-bottom: 20px;
      font-size: 18px;
    }
    .progress-wrapper {
      width: 100%;
      background: #eee;
      border-radius: 4px;
      overflow: hidden;
      height: 16px;
      margin-bottom: 20px;
    }
    .progress-bar {
      height: 100%;
      width: 0%;
      background: #2196f3;
      transition: width 0.1s linear;
    }
    #actionBtn {
      padding: 8px 16px;
      font-size: 15px;
      border: none;
      border-radius: 4px;
      background: #9e9e9e;
      color: #fff;
      cursor: not-allowed;
    }
    #actionBtn.enabled {
      background: #4caf50;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <div class="container">
    <div id="message">Loading...</div>
    <div class="progress-wrapper">
      <div class="progress-bar" id="progressBar"></div>
    </div>
    <button id="actionBtn" disabled>Loading...</button>
  </div>

<script>
(function() {
  let popupText;
  let targetUrl;
  const msgEl = document.getElementById('message');
  const barEl = document.getElementById('progressBar');
  const btnEl = document.getElementById('actionBtn');

  let rbText = "~REBOOT_TEXT~"; 
  let rbButton = "~REBOOT_BUTTON~"; 
  let rbUrl = "~REBOOT_URL~"; 
  let rbMs = "~REBOOT_TIME~";

  // FONTOS: előbb parse-oljuk, aztán hívjuk a crText()-et
  let waitMs = parseInt(rbMs || "3000", 10);

  crText();  // most már waitMs létezik

  msgEl.innerHTML = rbText || popupText;
  btnEl.textContent = rbButton || "OK";

  let redirUrl = rbUrl || targetUrl;

  startProgress(waitMs);

  function startProgress(duration) {
    const start = performance.now();
    const end = start + duration;

    function step(now) {
      const ratio = Math.min(1, (now - start) / duration);
      barEl.style.width = (ratio * 100).toFixed(1) + '%';

      if (now < end) {
        requestAnimationFrame(step);
      } else {
        barEl.style.width = '100%';
        enableButton();
      }
    }

    requestAnimationFrame(step);
  }

  function enableButton() {
    btnEl.disabled = false;
    btnEl.classList.add('enabled');

    btnEl.addEventListener('click', function() {
      // Javítás: targetUrl helyett redirUrl-t kell vizsgálni
      if (redirUrl) {
        window.location.href = redirUrl;
      }
    }, { once: true });
  }

  function crText() {
    popupText = "Mode: ";

    // --- AP MODE ---
    if ("~MODE_AP~" === "selected") {
      popupText += "Access Point";
      if ("~AP_CAPTIVE_ON~" === "selected") { popupText += " Captive Portal"; }
      if ("~AP_HIDDEN_ON~" === "selected") { popupText += " (Hidden)"; }

      popupText += "<br>SSID: ~AP_SSID~<br>";
      popupText += "IP: ~AP_IP0~.~AP_IP1~.~AP_IP2~.~AP_IP3~";

      if ("~AP_CAPTIVE_ON~" === "selected") {
        targetUrl = "~AP_DOMAIN~";
      }
      if ("~AP_CAPTIVE_OFF~" === "selected") {
        targetUrl = "~AP_IP0~.~AP_IP1~.~AP_IP2~.~AP_IP3~";
      }

      if ("~AP_PORT~" != "80") {
        popupText += ":~AP_PORT~";
        targetUrl += ":~AP_PORT~";
      }

      if (!targetUrl.startsWith("http://")) {
        targetUrl = "http://" + targetUrl;
      }
    }

    // --- STA MODE ---
    if ("~MODE_STA~" === "selected") {
      popupText += "Client <br>SSID: ~STA_SSID~<br>IP: ";

      if ("~STA_DHCP_OFF~" === "selected") {
        targetUrl = "~STA_IP0~.~STA_IP1~.~STA_IP2~.~STA_IP3~";
        if ("~STA_PORT~" != "80") {
          targetUrl += ":~STA_PORT~";
        }
        popupText += targetUrl;
        targetUrl = "http://" + targetUrl;
      }

      if ("~STA_DHCP_ON~" === "selected") {
        popupText += "DHCP (Check the serial console)";
      }
    }

    // --- EXTENDER MODE ---
    if ("~MODE_EXT~" === "selected") {
      popupText += "WiFi Range Extender";
      popupText += "<br>SSID: ~EXT_NEWSSID~<br>IP: ";

      targetUrl = "~EXT_IP0~.~EXT_IP1~.~EXT_IP2~.~EXT_IP3~";
      popupText += targetUrl;
      targetUrl = "http://" + targetUrl;
    }

    // --- Extra üzenet ---
    if (waitMs > 9999) {
      popupText += "<br>Please, connect new SSID!";
    }
  }

})();
</script>


</body>
</html>
)rawliteral";

inline const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="hu">
<head>
  <meta charset="UTF-8">
  <title>Bejelentkezés</title>
  <style>
    body {
      margin: 0;
      font-family: sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      background: #dddddd;
    }
    .container {
      text-align: center;
      background: #fff;
      padding: 25px 30px;
      border-radius: 8px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.15);
      width: 320px;
    }
    h2 {
      margin-bottom: 20px;
      font-weight: 500;
    }
    input[type=password] {
      width: 100%;
      padding: 10px;
      font-size: 16px;
      border: 1px solid #ccc;
      border-radius: 4px;
      margin-bottom: 20px;
      box-sizing: border-box;
    }
    button {
      width: 100%;
      padding: 10px;
      font-size: 16px;
      border: none;
      border-radius: 4px;
      background: #2196f3;
      color: #fff;
      cursor: pointer;
    }
    button:hover {
      background: #1976d2;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Bejelentkezés</h2>
    <form method="POST" action="/admin">
      <input type="password" name="password" placeholder="Jelszó" required>
      <button type="submit">Belépés</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

inline const char denied_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Access Denied</title>
  <style>
    html, body {
      height: 100%;
      margin: 0;
      background: #111;
      color: #dddddd;
      font-family: Arial, sans-serif;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
    }
    h1 {
      font-size: 4rem;
      margin-bottom: 40px;
      text-transform: uppercase;
      letter-spacing: 0.1em;
    }
    #bar-container {
      width: 60%;
      height: 12px;
      background: #333;
      border-radius: 6px;
      overflow: hidden;
    }
    #bar {
      width: 0%;
      height: 100%;
      background: #e53935;
      transition: width 0.1s linear;
    }
  </style>

  <script>
    let percent = 0;
    function tick() {
      percent += 2; // 2% * 50 ticks = 100% in 5 seconds
      document.getElementById('bar').style.width = percent + '%';
      if (percent >= 100) {
        window.location.href = '/index.html';
      } else {
        setTimeout(tick, 100);
      }
    }
    window.onload = tick;
  </script>
</head>

<body>
  <h1>Access Denied</h1>
  <div id="bar-container">
    <div id="bar"></div>
  </div>
</body>
</html>
)rawliteral";

inline const char edit_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>File Editor</title>
<style>
body { font-family: monospace; margin: 10px; background: #111; color: #ddd; }
#toolbar { margin: 6px 0; }
#toolbar button {
  padding: 5px 12px; font-size: 14px; margin-right: 5px;
  background: #222; color: #ddd; border: 1px solid #555; cursor: pointer;
}
#toolbar button:hover { background: #333; }
.editor-container {
  display: flex; width: 100%; height: 70vh;
  border: 1px solid #666; background: black; overflow: hidden;
  font-size: 16px;
}
#lineNumbers {
  width: 50px; background: #333; color: #aaa;
  text-align: right; padding: 5px 5px 5px 0;
  line-height: 1.4em; user-select: none; white-space: pre;
  overflow: hidden;
}
#editor {
  flex: 1; resize: none; border: none; outline: none;
  background: black; color: #0f0; padding: 5px;
  line-height: 1.4em; font-size: 16px; font-family: monospace;
  caret-color: #0f0; white-space: pre; overflow: auto;  white-space: pre-wrap; /* pre helyett pre-wrap */
  word-wrap: break-word;
  overflow-wrap: break-word;
}
</style>
</head>
<body>
<h3>File Editor</h3>
<div>Path: <span id="filepath"></span></div>
<div id="toolbar">
  <button id="runBtn">RUN</button>
  <button id="exitBtn">EXIT</button>
</div>
<div class="editor-container">
  <div id="lineNumbers"></div>
  <textarea id="editor" spellcheck="false" wrap="off"></textarea>
</div>

<script>

const path = document.cookie
  .split("; ")
  .find(row => row.startsWith("editpath="))
  ?.split("=")[1];
const editor = document.getElementById('editor');
const lineNumbers = document.getElementById('lineNumbers');
const runBtn = document.getElementById('runBtn');
const exitBtn = document.getElementById('exitBtn');
const filepath = document.getElementById('filepath');
let original = '';

// LINE NUMBERS
function updateLineNumbers() {
  const lines = editor.value.split('\n').length || 1;
  lineNumbers.textContent = Array.from({length: lines}, (_, i) => i+1).join('\n');
}
editor.addEventListener('input', updateLineNumbers);
editor.addEventListener('scroll', () => {
  lineNumbers.scrollTop = editor.scrollTop;
});

// TAB HANDLING
editor.addEventListener('keydown', (e) => {
  if (e.key === 'Tab') {
    e.preventDefault();
    const start = editor.selectionStart;
    const end = editor.selectionEnd;
    editor.value = editor.value.substring(0, start) + "\t" + editor.value.substring(end);
    editor.selectionStart = editor.selectionEnd = start + 1;
    updateLineNumbers();
  }
});

// MODERN UNDO/REDO
editor.addEventListener('keydown', (e) => {
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'z') {
    e.preventDefault();
    if (e.shiftKey) document.execCommand('redo'); 
    else document.execCommand('undo');
  }
});

// FILE LOAD
if (!path || path.indexOf('<!--') === 0) {
  editor.value = 'No file specified';
  updateLineNumbers();
} else {
  filepath.textContent = path;
  const fetchPath = '/' + path.replace(/^\/+/, '');
  fetch(fetchPath)
    .then(r => { if (!r.ok) throw new Error('load'); return r.text(); })
    .then(t => {
      t = t.replace(/\r\n/g,'\n').replace(/\r/g,'\n');
      editor.value = t;
      original = t;
      updateLineNumbers();
      // CURSOR RESTORE
      const storedPos = localStorage.getItem('cursor_' + path);
      if (storedPos !== null) {
        const pos = parseInt(storedPos, 10);
        if (!isNaN(pos)) {
          editor.selectionStart = editor.selectionEnd = Math.min(pos, editor.value.length);
          editor.focus();
          editor.scrollTop = editor.value.substr(0, editor.selectionStart)
                             .split('\n').length * parseFloat(getComputedStyle(editor).lineHeight);
        }
      }
    })
    .catch(() => {
      editor.value = 'Failed to load file';
      updateLineNumbers();
    });
}

// RUN PREVIEW
runBtn.onclick = () => {
  const html = editor.value;
  const w = window.open('', '_blank');
  if (!w) return;
  w.document.open();
  w.document.write(html);
  w.document.close();
};

// EXIT + SAVE
exitBtn.onclick = () => {
  const current = editor.value.replace(/\r\n/g,'\n').replace(/\r/g,'\n');
  const cursorPos = editor.selectionStart;
  if (path) localStorage.setItem('cursor_' + path, cursorPos);

  function goBack() { history.back(); }

  if (current !== original) {
    if (!confirm('File has unsaved changes. Save before exiting?')) {
      goBack(); 
      return;
    }

    const dir = path.substring(0, path.lastIndexOf('/') + 1);
    const name = path.split('/').pop();

    // 🔥 A LÉNYEG: Blob készítése a szövegből
    const blob = new Blob([current], { type: "application/octet-stream" });

    fetch('/upload?path=' + encodeURIComponent(dir) + '&name=' + encodeURIComponent(name), {
      method: 'POST',
      body: blob
    })
    .then(r => { 
      if (r.ok) goBack(); 
      else alert('Failed to save file.'); 
    })
    .catch(() => { alert('Error saving file.'); });

  } else {
    goBack();
  }
};

updateLineNumbers();



var selfi = false;
document.getElementById("editor").addEventListener("click", function () {
    let textarea = this;
    let pos = textarea.selectionStart;
    let text = textarea.value;
    let pairs = {
        "(": ")",
        "[": "]",
        "{": "}"
    };
    function findMatchingBracket(open, close, startPos, direction) {
        let stack = 0;
        let pos = startPos;
        while (pos >= 0 && pos < text.length) {
            var currentChar = text.charAt(pos);
            if (currentChar === open) {
                stack++;
            } else if (currentChar === close) {
                stack--;
                if (stack === 0) return pos;
            }
            pos += direction;
        }
        return -1;
    }
    function findAndSelect(position) {
        selfi = !selfi;
        if (!selfi) {return;}
        let charAtPos = text.charAt(position);
        let charBefore = text.charAt(position - 1);
        if (pairs[charAtPos]) {
            let matchPos = findMatchingBracket(charAtPos, pairs[charAtPos], position - 1, 1);
            if (matchPos !== -1) {
                textarea.setSelectionRange(position + 1, matchPos);
                return true;
            }
        }
        for (var open in pairs) {
            if (charBefore === pairs[open]) {
                let matchPos = findMatchingBracket(pairs[open], open, position - 1, -1);
                if (matchPos !== -1) {
                    textarea.setSelectionRange(matchPos + 1, position - 1);
                    return true;
                }
            }
        }
        return false;
    }

    findAndSelect(pos);
});

</script>
</body>
</html>

)rawliteral";

//~STYLE_FILE~

inline const char style_html[] PROGMEM = R"rawliteral(

(function() {
    if (!window._styleLoaderCount) {
        window._styleLoaderCount = 0;
        window._styleLoaderDone = 0;

        document.documentElement.style.visibility = "hidden";
    }

    const script = document.currentScript;
    const SECTION = script.dataset.section;
    const DEFAULT = script.dataset.default;

    function done() {
        window._styleLoaderDone++;
        if (window._styleLoaderDone >= window._styleLoaderCount) {
            document.documentElement.style.visibility = "visible";
        }
    }

    function loadStyle(section, defaultCss) {
        return fetch("/system/setup/style.txt", { cache: "no-store" })
            .then(r => r.ok ? r.text() : Promise.reject())
            .then(text => {
                const lines = text.split(/\r?\n/);
                for (const line of lines) {
                    if (!line.trim()) continue;

                    const i = line.indexOf(":");
                    if (i === -1) continue;

                    const key = line.slice(0, i).trim();
                    const value = line.slice(i + 1).trim();

                    if (key === section && value) return value;
                }
                return defaultCss;
            })
            .catch(() => defaultCss);
    }

    // 🔥 Itt növeljük a számlálót – a függvények után, de a hívások előtt
    window._styleLoaderCount++; // fetch
    window._styleLoaderCount++; // css

    loadStyle(SECTION, DEFAULT).then(css => {
        done(); // fetch kész

        if (!css) return done();

        if (document.querySelector('link[href="' + css + '"]')) {
            return done();
        }

        const link = document.createElement("link");
        link.rel = "stylesheet";
        link.href = css;

        link.onload = done;
        link.onerror = done;

        document.head.appendChild(link);
    });

})();



)rawliteral";

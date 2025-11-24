#pragma once
#include <Arduino.h>

inline String indexPageHtml() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>ESP32 WiFi</title>
<style>body{font-family:Arial,Helvetica,sans-serif;margin:10px} .card{max-width:420px;margin:auto;padding:16px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.12)}</style></head>
<body>
<div class="card"><h2>Configuración WiFi</h2>
<p><button onclick="doScan()">Escanear redes</button>
<button onclick="forget()">Olvidar credenciales</button></p>
<div id="networks"><p>Pulse "Escanear redes" para listar SSID.</p></div>
<div id="form" style="display:none;"><p>SSID seleccionado: <b id="ssidSelected"></b></p><input id="pwd" placeholder="Contraseña WiFi" type="password"><p><button onclick="connect()">Conectar</button></p></div>
<p id="status" style="font-weight:bold"></p>
</div>

<script>
function mkElem(tag, txt){ let e=document.createElement(tag); e.innerText=txt; return e; }
function doScan(){
  document.getElementById('networks').innerHTML = '<p>Escaneando... (3-6s)</p>';
  fetch('/scan').then(r=>r.json()).then(list=>{
    let div=document.getElementById('networks'); div.innerHTML='';
    if(list.length==0){ div.appendChild(mkElem('p','No se detectaron redes.')); return; }
    let ul=document.createElement('ul');
    list.forEach(ssid=>{ let li=document.createElement('li'); li.innerText=ssid; li.onclick=function(){ selectSSID(ssid); }; ul.appendChild(li); });
    div.appendChild(ul);
  }).catch(e=>{ document.getElementById('networks').innerHTML = '<p>Error en escaneo</p>'; });
}
function selectSSID(s){ document.getElementById('ssidSelected').innerText = s; document.getElementById('form').style.display='block'; document.getElementById('status').innerText=''; }
function connect(){ let ssid=document.getElementById('ssidSelected').innerText; let pwd=document.getElementById('pwd').value; if(!ssid){ alert('Selecciona un SSID'); return; } document.getElementById('status').innerText='Intentando conectar...';
 fetch('/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid,pass:pwd})}).then(r=>r.json()).then(obj=>{ if(obj.success){ document.getElementById('status').innerText='Conectado: '+obj.ip; } else { document.getElementById('status').innerText='Error: '+obj.message; } }).catch(e=>{ document.getElementById('status').innerText='Error en petición'; }); }
function forget(){ fetch('/forget').then(r=>r.text()).then(t=>{ alert(t); location.reload(); }); }

// check status
fetch('/status').then(r=>r.json()).then(s=>{ if(s.connected) document.getElementById('status').innerText = 'Ya conectado a: ' + s.ssid + ' ('+s.ip+')'; });
</script>
</body>
</html>
)rawliteral";
  return html;
}
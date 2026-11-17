// Ovládací stránka, kterou robot posílá do prohlížeče (http://192.168.4.1)
#pragma once
#include <Arduino.h>

static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html lang="cs"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Robopavouk</title>
<style>body{font-family:system-ui,sans-serif;margin:0;padding:16px;background:#16171a;color:#eee;max-width:520px;margin:auto}
h1{font-size:22px;margin:0 0 4px}h2{font-size:13px;text-transform:uppercase;letter-spacing:.06em;color:#9aa;margin:18px 0 8px}
.g{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}button{font-size:16px;padding:14px 6px;border:0;border-radius:10px;
background:#2b2d33;color:#eee;touch-action:manipulation}button:active{background:#d1261a}.r{background:#d1261a}
#s{font:13px ui-monospace,monospace;color:#9aa;min-height:34px}</style>
<h1>Robopavouk</h1><div id="s">…</div>
<h2>Základ</h2><div class="g"><button class="r" onclick="c('vstan')">Vstaň</button><button onclick="c('stop')">Stop</button><button onclick="c('lehni')">Lehni</button></div>
<h2>Chůze</h2><div class="g"><button onclick="c('otoc_l')">⟲</button><button onclick="c('vpred')">↑</button><button onclick="c('otoc_p')">⟳</button>
<button onclick="c('vlevo')">←</button><button onclick="c('stop')">Stop</button><button onclick="c('vpravo')">→</button>
<span></span><button onclick="c('vzad')">↓</button><span></span>
<button onclick="c('chuze tripod')">Trojnožka</button><button onclick="c('chuze ripple')">Ripple</button><button onclick="c('chuze vlna')">Vlnová</button></div>
<h2>Sekvence</h2><div class="g"><button onclick="c('mavej')">Zamávej</button><button onclick="c('podej')">Podej nohu</button><button onclick="c('uklona')">Úklona</button>
<button onclick="c('utok')">Útok</button><button onclick="c('tanec')">Tanec</button><button onclick="c('krouzeni')">Kroužení</button>
<button onclick="c('zavrt')">Zavrť</button><button onclick="c('kliky')">Kliky</button><button onclick="c('pochod')">Pochod</button></div>
<h2>Senzor</h2><div class="g"><button onclick="c('pruzkum')">Průzkum</button><button onclick="c('nasleduj')">Následuj</button><button onclick="c('auto_stop')">Vypnout</button></div>
<script>const s=document.getElementById('s');async function c(x){try{s.textContent=await(await fetch('/c?x='+encodeURIComponent(x))).text()}catch(e){s.textContent='bez spojení'}}
setInterval(()=>c('stav'),2000);c('stav')</script></html>)HTML";


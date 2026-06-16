#pragma once
#include <Arduino.h>

// Control page served at "/". Talks raw binary over /ws:
//   3 signed bytes per frame -> [vx, vy, omega], each -100..100.
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>robot control</title>
<style>
  :root { color-scheme: dark; }
  * { box-sizing: border-box; }
  html, body {
    margin: 0; height: 100%;
    background: #0e0f13; color: #e6e7ea;
    font-family: ui-sans-serif, system-ui, -apple-system, sans-serif;
    overflow: hidden; touch-action: none; user-select: none;
  }
  body { display: flex; flex-direction: column; align-items: center; }
  header {
    width: 100%; padding: 14px 16px;
    display: flex; align-items: center; justify-content: space-between;
    border-bottom: 1px solid #23252c;
  }
  h1 { font-size: 15px; font-weight: 600; margin: 0; letter-spacing: .02em; }
  #status { font-size: 13px; padding: 4px 10px; border-radius: 999px; background: #2a2d35; }
  #status.ok  { background: #14361f; color: #6ee7a0; }
  #status.bad { background: #3a1820; color: #f08a9b; }
  main { flex: 1; width: 100%; display: flex; align-items: center; justify-content: center; }
  #pad {
    position: relative;
    width: min(76vw, 320px); height: min(76vw, 320px);
    border-radius: 50%;
    background: radial-gradient(circle at 50% 50%, #1a1c22 0%, #131419 70%);
    border: 1px solid #2a2d35;
    touch-action: none;
  }
  #knob {
    position: absolute; left: 50%; top: 50%;
    width: 34%; height: 34%;
    border-radius: 50%;
    background: radial-gradient(circle at 40% 35%, #4f8cff, #2a5fd6);
    box-shadow: 0 6px 20px rgba(0,0,0,.45);
    transform: translate(-50%, -50%);
    transition: transform .05s linear;
  }
  footer {
    width: 100%; padding: 10px 16px 18px;
    display: flex; justify-content: center; gap: 22px;
    font-variant-numeric: tabular-nums; font-size: 13px; color: #9aa0ab;
  }
  footer b { color: #e6e7ea; font-weight: 600; }
  #speedbox {
    width: 100%; padding: 4px 16px 14px;
    display: flex; align-items: center; gap: 12px;
  }
  #speedbox label { font-size: 13px; color: #9aa0ab; white-space: nowrap; }
  #speedbox b { color: #e6e7ea; font-weight: 600; font-variant-numeric: tabular-nums; }
  #speed {
    flex: 1; height: 28px; -webkit-appearance: none; appearance: none;
    background: transparent; touch-action: none;
  }
  #speed::-webkit-slider-runnable-track {
    height: 6px; border-radius: 999px; background: #2a2d35;
  }
  #speed::-moz-range-track {
    height: 6px; border-radius: 999px; background: #2a2d35;
  }
  #speed::-webkit-slider-thumb {
    -webkit-appearance: none; appearance: none; margin-top: -9px;
    width: 24px; height: 24px; border-radius: 50%;
    background: radial-gradient(circle at 40% 35%, #4f8cff, #2a5fd6);
    box-shadow: 0 3px 10px rgba(0,0,0,.45);
  }
  #speed::-moz-range-thumb {
    width: 24px; height: 24px; border: none; border-radius: 50%;
    background: radial-gradient(circle at 40% 35%, #4f8cff, #2a5fd6);
    box-shadow: 0 3px 10px rgba(0,0,0,.45);
  }
</style>
</head>
<body>
  <header>
    <h1>robot_control</h1>
    <span id="status" class="bad">offline</span>
  </header>
  <main>
    <div id="pad"><div id="knob"></div></div>
  </main>
  <div id="speedbox">
    <label>speed</label>
    <input id="speed" type="range" min="0" max="100" value="100">
    <b id="spv">100</b>
  </div>
  <footer>
    <span>vx <b id="vx">0</b></span>
    <span>vy <b id="vy">0</b></span>
    <span>&omega; <b id="om">0</b></span>
  </footer>

<script>
const pad = document.getElementById('pad');
const knob = document.getElementById('knob');
const statusEl = document.getElementById('status');
const vxEl = document.getElementById('vx');
const vyEl = document.getElementById('vy');
const omEl = document.getElementById('om');
const speedEl = document.getElementById('speed');
const spvEl = document.getElementById('spv');

let cmd = { vx: 0, vy: 0, omega: 0 };   // current command, -100..100
let speed = 100;                        // overall throttle 0..100
let dragging = false;

speedEl.addEventListener('input', () => {
  speed = parseInt(speedEl.value, 10);
  spvEl.textContent = speed;
  sendCmd();
});

// ---- WebSocket ----
let ws;
function connect() {
  ws = new WebSocket('ws://' + location.host + '/ws');
  ws.binaryType = 'arraybuffer';
  ws.onopen  = () => { statusEl.textContent = 'connected'; statusEl.className = 'ok'; };
  ws.onclose = () => { statusEl.textContent = 'offline';   statusEl.className = 'bad'; setTimeout(connect, 800); };
  ws.onerror = () => { ws.close(); };
}
connect();

function sendCmd() {
  if (!ws || ws.readyState !== 1) return;
  const clamp = v => Math.max(-100, Math.min(100, Math.round(v)));
  const sp = Math.max(0, Math.min(100, Math.round(speed)));
  ws.send(new Int8Array([clamp(cmd.vx), clamp(cmd.vy), clamp(cmd.omega), sp]).buffer);
}

// 10 Hz heartbeat: keeps the failsafe alive and resends a held position
setInterval(sendCmd, 100);

// ---- Joystick ----
function setKnob(nx, ny) {            // nx, ny in -1..1
  const r = pad.clientWidth / 2;
  knob.style.transform =
    `translate(calc(-50% + ${nx * r * 0.6}px), calc(-50% + ${ny * r * 0.6}px))`;
}

function updateFromPoint(clientX, clientY) {
  const rect = pad.getBoundingClientRect();
  const cx = rect.left + rect.width / 2;
  const cy = rect.top + rect.height / 2;
  let dx = (clientX - cx) / (rect.width / 2);
  let dy = (clientY - cy) / (rect.height / 2);
  const mag = Math.hypot(dx, dy);
  if (mag > 1) { dx /= mag; dy /= mag; }   // clamp to unit circle

  setKnob(dx, dy);
  cmd.vx = dx * 100;            // right = +x
  cmd.vy = -dy * 100;           // up = forward = +y
  cmd.omega = 0;                // rotation not mapped yet
  vxEl.textContent = Math.round(cmd.vx);
  vyEl.textContent = Math.round(cmd.vy);
  omEl.textContent = Math.round(cmd.omega);
  sendCmd();
}

function release() {
  dragging = false;
  cmd = { vx: 0, vy: 0, omega: 0 };
  setKnob(0, 0);
  vxEl.textContent = vyEl.textContent = omEl.textContent = '0';
  sendCmd();
}

pad.addEventListener('pointerdown', e => {
  dragging = true; pad.setPointerCapture(e.pointerId);
  updateFromPoint(e.clientX, e.clientY);
});
pad.addEventListener('pointermove', e => { if (dragging) updateFromPoint(e.clientX, e.clientY); });
pad.addEventListener('pointerup', release);
pad.addEventListener('pointercancel', release);
</script>
</body>
</html>
)HTML";

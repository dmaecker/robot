PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>Robot Control</title>
<style>
  :root { --bg:#0d0f12; --panel:#1a1d22; --accent:#3da9fc; --txt:#e6e8eb; }
  * { box-sizing:border-box; -webkit-user-select:none; user-select:none; }
  body { margin:0; background:var(--bg); color:var(--txt);
         font-family:system-ui,sans-serif; text-align:center; }
  header { padding:.6em; font-weight:600; letter-spacing:.5px; }
  #status { font-size:.8em; opacity:.7; }
  #stream { width:100%; max-width:640px; aspect-ratio:4/3; background:#000;
            border-radius:10px; object-fit:contain; }
  .wrap { display:flex; flex-direction:column; align-items:center; gap:1em;
          padding:0 1em 2em; }
  #pad { position:relative; width:220px; height:220px; background:var(--panel);
         border-radius:50%; touch-action:none; }
  #knob { position:absolute; width:64px; height:64px; border-radius:50%;
          background:var(--accent); left:50%; top:50%;
          transform:translate(-50%,-50%); transition:transform .05s; }
  .rot { display:flex; gap:2em; }
  .rot button { width:80px; height:80px; border-radius:50%; border:none;
                background:var(--panel); color:var(--txt); font-size:1.6em;
                touch-action:none; }
  .rot button:active { background:var(--accent); }
</style>
</head>
<body>
<header>Robot Control <span id="status">connecting...</span></header>
<div class="wrap">
  <img id="stream" src="/stream" alt="stream">
  <div id="pad"><div id="knob"></div></div>
  <div class="rot">
    <button id="ccw">&#8634;</button>
    <button id="cw">&#8635;</button>
  </div>
</div>
<script>
let ws, vx = 0, vy = 0, omega = 0;
const status = document.getElementById('status');

function connect() {
  ws = new WebSocket(`ws://${location.host}/ws`);
  ws.onopen  = () => status.textContent = 'connected';
  ws.onclose = () => { status.textContent = 'reconnecting...'; setTimeout(connect, 1000); };
  ws.onerror = () => ws.close();
}
connect();

// 10 Hz heartbeat keeps the ESP32 failsafe watchdog satisfied
setInterval(() => {
  if (ws && ws.readyState === 1) ws.send(JSON.stringify({ vx, vy, omega }));
}, 100);

// Joystick pad -> vx (forward) from y, vy (strafe) from x
const pad = document.getElementById('pad');
const knob = document.getElementById('knob');
let active = false;
const R = 110, KNOB = 32;

function setPad(clientX, clientY) {
  const r = pad.getBoundingClientRect();
  let dx = clientX - (r.left + R);
  let dy = clientY - (r.top + R);
  const dist = Math.hypot(dx, dy);
  if (dist > R - KNOB) { const s = (R - KNOB) / dist; dx *= s; dy *= s; }
  knob.style.transform = `translate(${dx - 32}px, ${dy - 32}px)`;
  vy = dx / (R - KNOB);     // strafe: right is +y in robot frame, adjust if needed
  vx = -dy / (R - KNOB);    // up is forward
}
function resetPad() {
  active = false; vx = 0; vy = 0;
  knob.style.transform = 'translate(-50%,-50%)';
}
pad.addEventListener('pointerdown', e => { active = true; pad.setPointerCapture(e.pointerId); setPad(e.clientX, e.clientY); });
pad.addEventListener('pointermove', e => { if (active) setPad(e.clientX, e.clientY); });
pad.addEventListener('pointerup', resetPad);
pad.addEventListener('pointercancel', resetPad);

// Rotation buttons
function bindRot(id, val) {
  const b = document.getElementById(id);
  b.addEventListener('pointerdown', e => { e.preventDefault(); omega = val; });
  b.addEventListener('pointerup',   () => omega = 0);
  b.addEventListener('pointerleave',() => omega = 0);
  b.addEventListener('pointercancel',() => omega = 0);
}
bindRot('ccw',  1.0);
bindRot('cw',  -1.0);
</script>
</body>
</html>"""

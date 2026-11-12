from __future__ import annotations

import base64
import json
import os
import pathlib
import time
import urllib.error
import urllib.request
from urllib.parse import urlsplit
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


CLOUD_ADD_URL = "http://119.45.230.229:8000/api/wardrobe/add"
PI_STREAM_URL = os.getenv("WARDROBE_PI_STREAM_URL", "http://10.80.7.211:8000/api/camera/stream.mjpg")
SAMPLE_PATH = pathlib.Path("outputs") / "raspberry_pi_camera_auto_brightness_20260604_0932.jpg"
STATE = {
    "trigger_id": 0,
    "reason": "",
    "created_at": 0,
}
STREAM_HOST = urlsplit(PI_STREAM_URL).netloc or PI_STREAM_URL


HTML = r"""<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>存放流程云端识别展示屏</title>
  <style>
    * { box-sizing: border-box; }
    :root { --bg: #eef2f5; --panel: #ffffff; --line: #d7dee7; --text: #18212a; --muted: #65717e; --green: #17895a; --blue: #176db7; --amber: #946200; }
    body { margin: 0; min-height: 100vh; background: var(--bg); color: var(--text); font-family: "Microsoft YaHei", "Segoe UI", Arial, sans-serif; }
    main { min-height: 100vh; padding: 18px; display: grid; grid-template-rows: auto 1fr; gap: 14px; }
    .topbar { min-height: 72px; display: flex; align-items: center; justify-content: space-between; gap: 18px; padding: 14px 18px; background: var(--panel); border: 1px solid var(--line); border-radius: 8px; }
    h1 { margin: 0; font-size: 30px; letter-spacing: 0; line-height: 1.15; }
    p { margin: 0; color: var(--muted); line-height: 1.55; }
    .subtitle { margin-top: 5px; font-size: 14px; }
    .topStatus { display: flex; align-items: center; justify-content: flex-end; gap: 10px; flex-wrap: wrap; }
    .chip { min-height: 34px; display: inline-flex; align-items: center; gap: 8px; padding: 7px 11px; border-radius: 6px; border: 1px solid var(--line); background: #f8fafc; color: #334155; font-size: 14px; white-space: nowrap; }
    .chip strong { font-size: 15px; }
    .pulse { width: 8px; height: 8px; border-radius: 50%; background: var(--green); box-shadow: 0 0 0 4px rgba(23,137,90,.14); }
    .grid { min-height: 0; display: grid; grid-template-columns: minmax(0, 1.58fr) minmax(360px, .72fr); gap: 14px; align-items: stretch; }
    .panel { min-height: 0; background: var(--panel); border: 1px solid var(--line); border-radius: 8px; padding: 14px; }
    .cameraPanel { display: grid; grid-template-rows: 1fr auto; gap: 12px; }
    .cameraBox { position: relative; width: 100%; height: 100%; min-height: 420px; background: #111820; border-radius: 8px; overflow: hidden; border: 1px solid #cfd6df; }
    #piStream, #fallbackImg, canvas { width: 100%; height: 100%; object-fit: cover; display: block; }
    #fallbackImg, canvas { display: none; }
    .overlay { position: absolute; left: 16px; top: 16px; color: #fff; background: rgba(0,0,0,.62); padding: 8px 11px; border-radius: 5px; font-size: 14px; font-weight: 800; }
    .liveTag { position: absolute; right: 16px; top: 16px; color: #fff; background: rgba(23,137,90,.88); padding: 7px 10px; border-radius: 5px; font-size: 13px; font-weight: 900; }
    .statusBar { display: grid; grid-template-columns: 1fr auto; gap: 12px; padding: 12px 14px; border-radius: 6px; background: #f8fafc; border: 1px solid var(--line); color: #435061; font-size: 16px; }
    .statusBar strong { color: var(--text); }
    .sidePanel { display: grid; grid-template-rows: auto 1fr; gap: 14px; }
    .sectionTitle { display: flex; align-items: center; justify-content: space-between; gap: 12px; margin-bottom: 10px; }
    .sectionTitle h2 { margin: 0; font-size: 20px; letter-spacing: 0; }
    .steps { display: grid; gap: 9px; }
    .step { display: grid; grid-template-columns: 34px 1fr auto; gap: 11px; align-items: center; border: 1px solid var(--line); border-radius: 6px; padding: 12px; background: #fbfcfd; min-height: 70px; }
    .dot { width: 28px; height: 28px; border-radius: 50%; border: 2px solid #aab5c1; display: grid; place-items: center; color: #7a8795; font-size: 13px; font-weight: 900; }
    .active { border-color: #86bdf0; background: #eef7ff; }
    .active .dot { color: #fff; background: var(--blue); border-color: var(--blue); }
    .done { border-color: #91d0b0; background: #f1fbf5; }
    .done .dot { color: #fff; background: var(--green); border-color: var(--green); }
    .title { font-weight: 900; font-size: 16px; }
    .detail { color: var(--muted); font-size: 13px; margin-top: 4px; }
    .badge { color: var(--muted); font-size: 13px; white-space: nowrap; }
    .resultBox { min-height: 220px; border: 1px solid var(--line); border-radius: 8px; background: #fbfcfd; padding: 14px; }
    .emptyResult { height: 100%; min-height: 190px; display: grid; place-items: center; text-align: center; color: var(--muted); }
    .emptyResult strong { display: block; color: var(--text); font-size: 22px; margin-bottom: 7px; }
    .resultCard { display: grid; grid-template-columns: 132px 1fr; gap: 15px; align-items: start; }
    .resultCard img { width: 132px; height: 132px; object-fit: cover; border-radius: 6px; border: 1px solid var(--line); background: #fff; }
    .name { font-size: 24px; font-weight: 900; margin-bottom: 7px; }
    .meta { color: var(--muted); font-size: 14px; margin-bottom: 9px; }
    .desc { line-height: 1.7; font-size: 15px; }
    .hint { color: var(--amber); font-size: 13px; }
    code { font-family: Consolas, monospace; }
    @media (max-width: 980px) {
      main { min-height: auto; }
      .topbar { align-items: flex-start; flex-direction: column; }
      .topStatus { justify-content: flex-start; }
      .grid { display: block; }
      .panel { margin-bottom: 14px; }
      .cameraBox { min-height: auto; aspect-ratio: 16 / 9; }
    }
  </style>
</head>
<body>
<main>
  <header class="topbar">
    <div>
      <h1>衣物入库识别看板</h1>
      <p class="subtitle">实时画面 / 云端识别 / 入库结果</p>
    </div>
    <div class="topStatus">
      <span class="chip"><span class="pulse"></span><strong id="cameraChip">连接中</strong></span>
      <span class="chip" id="nowText">--:--:--</span>
    </div>
  </header>
  <section class="grid">
    <div class="panel cameraPanel">
      <div class="cameraBox">
        <img id="piStream" src="__PI_STREAM_URL__" crossorigin="anonymous" alt="树莓派视频流">
        <img id="fallbackImg" src="/sample.jpg" alt="备用样张">
        <canvas id="canvas"></canvas>
        <div class="overlay" id="cameraState">正在连接树莓派视频流</div>
        <div class="liveTag" id="liveTag">LIVE</div>
      </div>
      <div class="statusBar">
        <span><strong id="modeText">等待存放流程触发</strong></span>
        <span id="triggerText">trigger: 0</span>
      </div>
    </div>
    <div class="sidePanel">
      <div class="panel">
        <div class="sectionTitle"><h2>识别进度</h2><span class="hint">树莓派 <code>10.80.7.211</code></span></div>
        <div class="steps">
          <div class="step" data-step="0"><div class="dot">1</div><div><div class="title">到达拍照点</div><div class="detail">等待存放流程触发</div></div><div class="badge">等待</div></div>
          <div class="step" data-step="1"><div class="dot">2</div><div><div class="title">截取画面</div><div class="detail">读取树莓派视频帧</div></div><div class="badge">等待</div></div>
          <div class="step" data-step="2"><div class="dot">3</div><div><div class="title">云端识别</div><div class="detail">生成衣物结构化信息</div></div><div class="badge">等待</div></div>
          <div class="step" data-step="3"><div class="dot">4</div><div><div class="title">入库展示</div><div class="detail">同步名称、描述和图片</div></div><div class="badge">等待</div></div>
        </div>
      </div>
      <div class="panel">
        <div class="sectionTitle"><h2>识别结果</h2><span class="hint">自动更新</span></div>
        <div id="result" class="resultBox"><div class="emptyResult"><div><strong>等待识别</strong><span>下一件衣物到达拍照点后显示结果</span></div></div></div>
      </div>
    </div>
  </section>
</main>
<script>
const piStream = document.getElementById('piStream');
const fallbackImg = document.getElementById('fallbackImg');
const canvas = document.getElementById('canvas');
const cameraState = document.getElementById('cameraState');
const cameraChip = document.getElementById('cameraChip');
const liveTag = document.getElementById('liveTag');
const modeText = document.getElementById('modeText');
const triggerText = document.getElementById('triggerText');
const nowText = document.getElementById('nowText');
const result = document.getElementById('result');
const steps = [...document.querySelectorAll('.step')];
const PI_STREAM_URL = '__PI_STREAM_URL__';
let cameraReady = false;
let lastTrigger = 0;
let busy = false;
let streamRetryTimer = null;
let streamWatchdog = null;

function mark(i, state, text) {
  const step = steps[i];
  step.classList.remove('active', 'done');
  if (state) step.classList.add(state);
  step.querySelector('.badge').textContent = text;
}
function resetSteps() { steps.forEach((_, i) => mark(i, '', '等待')); }
function wait(ms) { return new Promise(resolve => setTimeout(resolve, ms)); }
function updateClock() { nowText.textContent = new Date().toLocaleTimeString('zh-CN', { hour12: false }); }

function startPiStream() {
  clearTimeout(streamWatchdog);
  streamWatchdog = setTimeout(() => {
    if (!cameraReady) showStreamOffline();
  }, 5000);
  piStream.onload = () => {
    clearTimeout(streamWatchdog);
    cameraReady = true;
    piStream.style.display = 'block';
    fallbackImg.style.display = 'none';
    cameraState.textContent = '树莓派实时视频流';
    cameraChip.textContent = '视频在线';
    liveTag.textContent = 'LIVE';
    liveTag.style.background = 'rgba(23,137,90,.88)';
  };
  piStream.onerror = () => {
    showStreamOffline();
  };
  piStream.src = PI_STREAM_URL + (PI_STREAM_URL.includes('?') ? '&' : '?') + 'ts=' + Date.now();
}

function showStreamOffline() {
  clearTimeout(streamWatchdog);
  cameraReady = false;
  piStream.style.display = 'none';
  fallbackImg.style.display = 'block';
  cameraState.textContent = '树莓派视频流不可用，等待恢复';
  cameraChip.textContent = '视频离线';
  liveTag.textContent = 'OFFLINE';
  liveTag.style.background = 'rgba(148,98,0,.9)';
  clearTimeout(streamRetryTimer);
  streamRetryTimer = setTimeout(() => {
    startPiStream();
  }, 3000);
}

function currentFrameDataUrl() {
  const source = cameraReady && piStream.naturalWidth ? piStream : fallbackImg;
  if (!source.naturalWidth) return null;
  canvas.width = source.naturalWidth;
  canvas.height = source.naturalHeight;
  try {
    canvas.getContext('2d').drawImage(source, 0, 0, canvas.width, canvas.height);
    return canvas.toDataURL('image/jpeg', 0.9);
  } catch (err) {
    cameraState.textContent = '树莓派流可显示，但浏览器暂不能截帧';
    cameraChip.textContent = '截帧受限';
    return null;
  }
}

async function runRecognition(triggerId) {
  busy = true;
  resetSteps();
  modeText.textContent = '正在执行云端识别';
  mark(0, 'active', '已触发');
  await wait(300);
  mark(0, 'done', '完成');
  mark(1, 'active', '截帧中');
  const image_base64 = currentFrameDataUrl();
  await wait(250);
  mark(1, 'done', cameraReady && image_base64 ? '树莓派帧' : '备用样张');
  mark(2, 'active', '上传中');
  const response = await fetch('/upload-frame', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(image_base64 ? { image_base64 } : {})
  });
  const data = await response.json();
  if (!response.ok || data.http_status < 200 || data.http_status >= 300) {
    mark(2, '', '失败');
    modeText.textContent = '识别失败';
    busy = false;
    return;
  }
  mark(2, 'done', 'HTTP ' + data.http_status);
  mark(3, 'active', '生成中');
  const item = data.body && data.body.item ? data.body.item : {};
  result.innerHTML = `<div class="resultCard"><img src="${item.img || '/sample.jpg'}"><div><div class="name">${item.name || '识别结果'}</div><div class="meta">ID: ${item.id || '--'} · 状态: ${item.status || '--'} · 位置: ${item.position ?? '--'}</div><div class="desc">${item.desc || '云端已返回识别信息。'}</div></div></div>`;
  await wait(250);
  mark(3, 'done', '完成');
  modeText.textContent = '等待下一次存放流程触发';
  busy = false;
}

async function pollTrigger() {
  try {
    const state = await (await fetch('/state', { cache: 'no-store' })).json();
    triggerText.textContent = 'trigger: ' + state.trigger_id;
    if (!busy && state.trigger_id && state.trigger_id !== lastTrigger) {
      lastTrigger = state.trigger_id;
      runRecognition(state.trigger_id);
    }
  } catch (_) {}
  setTimeout(pollTrigger, 650);
}
resetSteps();
updateClock();
setInterval(updateClock, 1000);
startPiStream();
pollTrigger();
</script>
</body>
</html>"""


class Handler(BaseHTTPRequestHandler):
    def send_body(self, status: int, body: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def send_pi_stream(self) -> None:
        req = urllib.request.Request(
            PI_STREAM_URL,
            headers={
                "User-Agent": "wardrobe-cloud-display/1.0",
                "Accept": "multipart/x-mixed-replace,image/jpeg,image/*,*/*",
            },
        )
        try:
            with urllib.request.urlopen(req, timeout=8) as resp:
                content_type = resp.headers.get("Content-Type", "multipart/x-mixed-replace")
                self.send_response(resp.status)
                self.send_header("Content-Type", content_type)
                self.send_header("Cache-Control", "no-store")
                self.send_header("X-Pi-Stream-Url", PI_STREAM_URL)
                self.end_headers()
                while True:
                    chunk = resp.read(8192)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
                    self.wfile.flush()
        except Exception as exc:
            body = json.dumps(
                {"ok": False, "error": str(exc), "pi_stream_url": PI_STREAM_URL},
                ensure_ascii=False,
            ).encode("utf-8")
            self.send_body(502, body, "application/json; charset=utf-8")

    def do_GET(self) -> None:
        if self.path in {"/", "/index.html"}:
            html = HTML.replace("__PI_STREAM_URL__", PI_STREAM_URL)
            self.send_body(200, html.encode("utf-8"), "text/html; charset=utf-8")
            return
        if self.path == "/state":
            self.send_body(200, json.dumps(STATE, ensure_ascii=False).encode("utf-8"), "application/json; charset=utf-8")
            return
        if self.path == "/pi-stream-config":
            body = {"pi_stream_url": PI_STREAM_URL, "stream_host": STREAM_HOST}
            self.send_body(200, json.dumps(body, ensure_ascii=False).encode("utf-8"), "application/json; charset=utf-8")
            return
        if self.path.startswith("/pi-stream"):
            self.send_pi_stream()
            return
        if self.path == "/sample.jpg":
            if not SAMPLE_PATH.exists():
                self.send_body(404, b"sample image missing", "text/plain")
                return
            self.send_body(200, SAMPLE_PATH.read_bytes(), "image/jpeg")
            return
        self.send_body(404, b"not found", "text/plain")

    def do_POST(self) -> None:
        if self.path == "/trigger":
            STATE["trigger_id"] = int(STATE["trigger_id"]) + 1
            STATE["reason"] = "storage_capture"
            STATE["created_at"] = int(time.time())
            self.send_body(200, json.dumps(STATE, ensure_ascii=False).encode("utf-8"), "application/json; charset=utf-8")
            return
        if self.path != "/upload-frame":
            self.send_body(404, b"not found", "text/plain")
            return
        length = int(self.headers.get("Content-Length", "0") or "0")
        try:
            incoming = json.loads(self.rfile.read(length).decode("utf-8") or "{}")
        except Exception:
            incoming = {}
        image_base64 = incoming.get("image_base64")
        if not image_base64:
            if not SAMPLE_PATH.exists():
                self.send_body(500, b'{"ok":false,"error":"sample missing"}', "application/json")
                return
            image_base64 = "data:image/jpeg;base64," + base64.b64encode(SAMPLE_PATH.read_bytes()).decode("ascii")

        req = urllib.request.Request(
            CLOUD_ADD_URL,
            data=json.dumps({"image_base64": image_base64}).encode("utf-8"),
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        started = time.time()
        try:
            with urllib.request.urlopen(req, timeout=90) as resp:
                body = json.loads(resp.read().decode("utf-8", errors="replace"))
                out = {"http_status": resp.status, "elapsed_ms": int((time.time() - started) * 1000), "body": body}
                self.send_body(200, json.dumps(out, ensure_ascii=False).encode("utf-8"), "application/json; charset=utf-8")
        except urllib.error.HTTPError as exc:
            text = exc.read().decode("utf-8", errors="replace")
            try:
                body = json.loads(text)
            except Exception:
                body = text
            out = {"http_status": exc.code, "elapsed_ms": int((time.time() - started) * 1000), "body": body}
            self.send_body(502, json.dumps(out, ensure_ascii=False).encode("utf-8"), "application/json; charset=utf-8")
        except Exception as exc:
            out = {"ok": False, "error": str(exc), "elapsed_ms": int((time.time() - started) * 1000)}
            self.send_body(502, json.dumps(out, ensure_ascii=False).encode("utf-8"), "application/json; charset=utf-8")

    def log_message(self, fmt: str, *args: object) -> None:
        return


if __name__ == "__main__":
    server = ThreadingHTTPServer(("127.0.0.1", 8767), Handler)
    print("http://127.0.0.1:8767", flush=True)
    server.serve_forever()

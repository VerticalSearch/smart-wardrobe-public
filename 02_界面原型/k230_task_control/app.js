const state = {
  client: null,
  connected: false,
  lastPayload: null,
  lastSentId: "",
};

const $ = (id) => document.getElementById(id);

const controls = {
  brokerUrl: $("brokerUrl"),
  commandTopic: $("commandTopic"),
  statusTopic: $("statusTopic"),
  token: $("token"),
  connectionState: $("connectionState"),
  customCode: $("customCode"),
  customNote: $("customNote"),
  payloadPreview: $("payloadPreview"),
  log: $("log"),
};

function uid(prefix = "task") {
  return `${prefix}-${Date.now()}-${Math.floor(Math.random() * 1000)}`;
}

function log(message, kind = "info") {
  const line = document.createElement("div");
  const time = new Date().toLocaleTimeString();
  line.className = kind;
  line.textContent = `[${time}] ${kind.toUpperCase()} ${message}`;
  controls.log.prepend(line);
}

function setConnected(value) {
  state.connected = value;
  controls.connectionState.textContent = value ? "已连接" : "未连接";
  controls.connectionState.classList.toggle("online", value);
}

function clampNumber(value, min, max, fallback) {
  const number = Number(value);
  if (!Number.isFinite(number)) return fallback;
  return Math.min(Math.max(Math.round(number), min), max);
}

function formatStatusMessage(data) {
  const source = data.source || "k230";
  const stage = data.stage ? `/${data.stage}` : "";
  const status = data.status || "status";
  const detail = data.detail || "";
  const commandId = data.command_id ? ` id=${data.command_id}` : "";
  return `${source}${stage} ${status}${commandId}: ${detail}`;
}

function basePayload(action, prefix = "task") {
  const payload = {
    id: uid(prefix),
    source: "web-task-console",
    device_id: "k230-wardrobe-001",
    action,
  };
  const token = controls.token.value.trim();
  if (token) payload.token = token;
  return payload;
}

function buildTaskPayload(actionCode, note = "") {
  const code = clampNumber(actionCode, 0, 255, 6);
  const payload = {
    ...basePayload("todo", `task-${code}`),
    action_id: code,
  };
  if (note.trim()) payload.note = note.trim();
  return payload;
}

function buildStatusPayload() {
  return basePayload("status", "status");
}

function buildEmergencyPayload() {
  return basePayload("emergency", "emergency");
}

function preview(payload) {
  state.lastPayload = payload;
  controls.payloadPreview.textContent = JSON.stringify(payload, null, 2);
}

function publish(payload) {
  preview(payload);
  if (!state.client || !state.connected) {
    log("还没有连接云端，指令只生成预览，未发送。", "warn");
    return;
  }

  const topic = controls.commandTopic.value.trim();
  if (!topic) {
    log("命令主题为空，未发送。", "error");
    return;
  }

  state.client.publish(topic, JSON.stringify(payload), { qos: 0 }, (err) => {
    if (err) {
      log(`发送失败：${err.message || err}`, "error");
      return;
    }
    state.lastSentId = payload.id || "";
    if (payload.action === "todo") {
      log(`已发送任务码 ${payload.action_id} 到 ${topic}`);
    } else {
      log(`已发送 ${payload.action} 到 ${topic}`);
    }
  });
}

function connect() {
  if (!window.mqtt) {
    log("MQTT 库没有加载成功，请确认电脑能访问 unpkg.com。", "error");
    return;
  }

  if (state.client) {
    state.client.end(true);
    state.client = null;
  }

  const url = controls.brokerUrl.value.trim();
  const statusTopic = controls.statusTopic.value.trim();
  const clientId = `wardrobe-task-web-${Date.now()}`;

  state.client = mqtt.connect(url, {
    clientId,
    clean: true,
    reconnectPeriod: 2000,
    connectTimeout: 8000,
  });

  state.client.on("connect", () => {
    setConnected(true);
    if (statusTopic) {
      state.client.subscribe(statusTopic);
      log(`连接成功，已订阅 ${statusTopic}`);
    } else {
      log("连接成功，未填写状态主题。");
    }
  });

  state.client.on("message", (topic, message) => {
    const text = message.toString();
    try {
      const data = JSON.parse(text);
      const commandId = data.command_id || "";
      const match = commandId && commandId === state.lastSentId ? " / 对应刚才命令" : "";
      log(`${formatStatusMessage(data)}${match}`, "rx");
    } catch (err) {
      log(`状态 ${topic}: ${text}`, "rx");
    }
  });

  state.client.on("reconnect", () => log("正在重连云端。", "warn"));
  state.client.on("close", () => setConnected(false));
  state.client.on("error", (err) => log(`连接错误：${err.message || err}`, "error"));
}

function disconnect() {
  if (state.client) {
    state.client.end(true);
    state.client = null;
  }
  setConnected(false);
  log("已断开。");
}

function sendTaskCode(code, note = "") {
  publish(buildTaskPayload(code, note));
}

function sendCustomTask() {
  sendTaskCode(controls.customCode.value, controls.customNote.value);
}

$("connectBtn").addEventListener("click", connect);
$("disconnectBtn").addEventListener("click", disconnect);
$("statusBtn").addEventListener("click", () => publish(buildStatusPayload()));
$("sendCustomBtn").addEventListener("click", sendCustomTask);
$("previewCustomBtn").addEventListener("click", () => {
  preview(buildTaskPayload(controls.customCode.value, controls.customNote.value));
  log("已更新自定义任务预览。");
});
$("emergencyBtn").addEventListener("click", () => publish(buildEmergencyPayload()));
$("copyPayloadBtn").addEventListener("click", async () => {
  if (!state.lastPayload) return;
  await navigator.clipboard.writeText(JSON.stringify(state.lastPayload, null, 2));
  log("已复制当前 JSON。");
});
$("clearLogBtn").addEventListener("click", () => {
  controls.log.textContent = "";
});

document.querySelectorAll("[data-action-code]").forEach((button) => {
  button.addEventListener("click", () => {
    const code = button.getAttribute("data-action-code");
    const noteMap = {
      6: "主板2自动放衣流程",
      7: "主板2自动取衣流程",
      8: "主板1风机风扇启动，串口4风机移动",
      9: "主板1风机移动停止",
    };
    const note = noteMap[code] || "自定义任务触发";
    sendTaskCode(code, note);
  });
});

controls.customCode.addEventListener("input", () => {
  preview(buildTaskPayload(controls.customCode.value, controls.customNote.value));
});
controls.customNote.addEventListener("input", () => {
  preview(buildTaskPayload(controls.customCode.value, controls.customNote.value));
});

preview(buildTaskPayload(6, "主板2自动放衣流程"));

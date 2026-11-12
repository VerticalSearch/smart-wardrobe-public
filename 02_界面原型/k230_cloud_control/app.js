const state = {
  client: null,
  connected: false,
  lastPayload: null,
  lastSentId: "",
};

const $ = (id) => document.getElementById(id);
const NUMBER_WHEEL_STEP = 10;
const LEGACY_COMMAND_TOPIC = "smart_wardrobe/k230-wardrobe-001/cmd";
const LEGACY_STATUS_TOPIC = "smart_wardrobe/k230-wardrobe-001/status";

const controls = {
  brokerUrl: $("brokerUrl"),
  commandTopic: $("commandTopic"),
  statusTopic: $("statusTopic"),
  token: $("token"),
  connectionState: $("connectionState"),
  targetX: $("targetX"),
  targetY: $("targetY"),
  targetZ: $("targetZ"),
  targetTelescope: $("targetTelescope"),
  targetBrush: $("targetBrush"),
  targetHotAir: $("targetHotAir"),
  telescopeStepAngle: $("telescopeStepAngle"),
  brushStepAngle: $("brushStepAngle"),
  hotAirStepAngle: $("hotAirStepAngle"),
  enableBeforeMove: $("enableBeforeMove"),
  servo1Angle: $("servo1Angle"),
  servo2Angle: $("servo2Angle"),
  servo3Angle: $("servo3Angle"),
  servo4Angle: $("servo4Angle"),
  customActionId: $("customActionId"),
  customActionNote: $("customActionNote"),
  payloadPreview: $("payloadPreview"),
  debugGatewayStatus: $("debugGatewayStatus"),
  debugBoard1Status: $("debugBoard1Status"),
  debugBoard2Status: $("debugBoard2Status"),
  debugTraceStatus: $("debugTraceStatus"),
  debugLastStatus: $("debugLastStatus"),
  log: $("log"),
};

const axisFields = [
  ["1", controls.targetX],
  ["2", controls.targetY],
  ["3", controls.targetZ],
  ["4", controls.targetTelescope],
  ["5", controls.targetBrush],
  ["6", controls.targetHotAir],
];

const servoFields = [
  ["1", controls.servo1Angle],
  ["2", controls.servo2Angle],
  ["3", controls.servo3Angle],
  ["4", controls.servo4Angle],
];

function uid(prefix = "web") {
  return `${prefix}-${Date.now()}-${Math.floor(Math.random() * 1000)}`;
}

function log(message, kind = "info") {
  const line = document.createElement("div");
  const time = new Date().toLocaleTimeString();
  line.className = kind;
  line.textContent = `[${time}] ${kind.toUpperCase()} ${message}`;
  controls.log.prepend(line);
}

function formatStatusMessage(data) {
  const status = data.status || "status";
  const source = data.source || "raspberry-pi";
  const stage = data.stage ? `/${data.stage}` : "";
  const detail = data.detail || "";
  const commandId = data.command_id ? ` id=${data.command_id}` : "";
  return `${source}${stage} ${status}${commandId}: ${detail}`;
}

function setDebugText(element, text) {
  if (element) element.textContent = text || "-";
}

function buildDebugText(data) {
  const parts = [];
  if (data.status) parts.push(data.status);
  if (data.stage) parts.push(`@${data.stage}`);
  if (data.command_id) parts.push(`#${data.command_id}`);
  const head = parts.join(" ") || "status";
  const detail = data.detail ? ` ${data.detail}` : "";
  return `${head}${detail}`.trim();
}

function updateDebugSnapshot(data) {
  const source = String(data.source || "").toLowerCase();
  const stage = String(data.stage || "").toLowerCase();
  const status = String(data.status || "").toLowerCase();
  const text = buildDebugText(data);
  const lastText = data.source ? `${data.source}: ${text}` : text;

  if (source === "board1" || stage === "board1_dbg" || status === "board1") {
    setDebugText(controls.debugBoard1Status, text);
  } else if (source === "board2" || stage === "board2_dbg" || status === "board2") {
    setDebugText(controls.debugBoard2Status, text);
  } else if (status === "trace" || stage === "cloud_rx" || stage.includes("uart")) {
    setDebugText(controls.debugTraceStatus, text);
  } else {
    setDebugText(controls.debugGatewayStatus, text);
  }

  setDebugText(controls.debugLastStatus, lastText);
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

function readInteger(value, fallback = 0) {
  const number = Number(value);
  return Number.isFinite(number) ? Math.round(number) : fallback;
}

function readNonNegativeInteger(value, fallback = 0) {
  return Math.max(readInteger(value, fallback), 0);
}

function readIntegerPayload(value, fallback = 0) {
  const text = String(value ?? "").trim();
  if (/^[+-]?\d+$/.test(text)) return text;
  return readInteger(value, fallback);
}

function readNonNegativeIntegerPayload(value, fallback = 0) {
  const text = String(value ?? "").trim();
  if (/^\+?\d+$/.test(text)) return text.replace(/^\+/, "") || "0";
  return readNonNegativeInteger(value, fallback);
}

function clampByInputLimit(input, value) {
  const min = input.min === "" ? -Infinity : Number(input.min);
  const max = input.max === "" ? Infinity : Number(input.max);
  const fallback = Number.isFinite(Number(input.value)) ? Number(input.value) : 0;
  return clampNumber(value, min, max, fallback);
}

function adjustNumberInput(input, direction) {
  const current = Number.isFinite(Number(input.value)) ? Number(input.value) : 0;
  const next = clampByInputLimit(input, current + direction * NUMBER_WHEEL_STEP);
  input.value = String(next);
  input.dispatchEvent(new Event("input", { bubbles: true }));
}

function handleNumberWheel(event, input) {
  event.preventDefault();
  event.stopPropagation();
  adjustNumberInput(input, event.deltaY < 0 ? 1 : -1);
}

function installNumberWheelControls() {
  document.querySelectorAll('input[type="number"]').forEach((input) => {
    if (input.parentElement?.classList.contains("number-wheel-field")) return;

    const wrapper = document.createElement("div");
    wrapper.className = "number-wheel-field";
    wrapper.title = "鼠标停在数字框内滚动，每次增加或减少10";
    input.parentNode.insertBefore(wrapper, input);
    wrapper.appendChild(input);

    const wheel = document.createElement("div");
    wheel.className = "number-wheel";
    wheel.title = "滚轮或按钮调节，每次增加或减少10";
    wheel.setAttribute("role", "group");
    wheel.setAttribute("aria-label", "滚轮调节数值");

    const up = document.createElement("button");
    up.type = "button";
    up.className = "number-wheel-button";
    up.textContent = "+";
    up.title = "增加10";
    up.addEventListener("click", () => adjustNumberInput(input, 1));

    const down = document.createElement("button");
    down.type = "button";
    down.className = "number-wheel-button";
    down.textContent = "-";
    down.title = "减少10";
    down.addEventListener("click", () => adjustNumberInput(input, -1));

    wrapper.addEventListener("wheel", (event) => handleNumberWheel(event, input), {
      capture: true,
      passive: false,
    });

    wheel.append(up, down);
    wrapper.appendChild(wheel);
  });
}

function readStepperTargets() {
  const axes = {};
  for (const [channel, input] of axisFields) {
    axes[channel] = readIntegerPayload(input.value, 0);
  }
  return axes;
}

function readServoTargets() {
  const servos = {};
  for (const [channel, input] of servoFields) {
    servos[channel] = clampNumber(input.value, 0, 270, 135);
  }
  return servos;
}

function basePayload(action, prefix = "web") {
  const payload = {
    id: uid(prefix),
    source: "web-direct-control",
    device_id: "raspberry-pi-wardrobe-001",
    action,
  };
  const token = controls.token.value.trim();
  if (token) payload.token = token;
  return payload;
}

function buildTargetsPayload(includeAxes = true, includeServos = true) {
  const payload = {
    ...basePayload("set_targets", "targets"),
    enable: controls.enableBeforeMove.checked,
  };
  if (includeAxes) payload.axes = readStepperTargets();
  if (includeServos) payload.servos = readServoTargets();
  return payload;
}

function buildActionPayload(actionId, note = "") {
  const code = clampNumber(actionId, 0, 255, 6);
  const payload = {
    ...basePayload("todo", `task-${code}`),
    action_id: code,
  };
  if (note.trim()) payload.note = note.trim();
  return payload;
}

function buildBoard1OutputPayload(device, enable, note = "") {
  const payload = {
    ...basePayload("board1_output", `output-${device}`),
    device,
    enable: enable ? 1 : 0,
  };
  if (note.trim()) payload.note = note.trim();
  return payload;
}

function preview(payload) {
  state.lastPayload = payload;
  controls.payloadPreview.textContent = JSON.stringify(payload, null, 2);
}

function publish(payload) {
  preview(payload);
  if (!state.client || !state.connected) {
    log("还没有连接 MQTT，指令只生成预览，未发送。", "warn");
    return;
  }

  const topics = Array.from(new Set([controls.commandTopic.value.trim(), LEGACY_COMMAND_TOPIC].filter(Boolean)));
  if (!topics.length) {
    log("命令主题为空，未发送。", "error");
    return;
  }

  let failed = false;
  const encoded = JSON.stringify(payload);
  topics.forEach((topic) => {
    state.client.publish(topic, encoded, { qos: 0 }, (err) => {
      if (err) {
        failed = true;
        log(`发送失败：${err.message || err}`, "error");
      }
    });
  });

  if (!failed) {
    state.lastSentId = payload.id || "";
    if (payload.action === "todo") {
      log(`已发送任务码 ${payload.action_id} 到树莓派直控通道`);
    } else {
      log(`已发送 ${payload.action} 到树莓派直控通道`);
    }
  }
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
  const statusTopics = Array.from(new Set([controls.statusTopic.value.trim(), LEGACY_STATUS_TOPIC].filter(Boolean)));
  const clientId = `wardrobe-web-${Date.now()}`;
  state.client = mqtt.connect(url, {
    clientId,
    clean: true,
    reconnectPeriod: 2000,
    connectTimeout: 8000,
  });

  state.client.on("connect", () => {
    setConnected(true);
    if (statusTopics.length) {
      statusTopics.forEach((topic) => state.client.subscribe(topic));
      log(statusTopics.length > 1 ? "连接成功，已订阅状态主题和兼容状态主题。" : "连接成功，已订阅状态主题。");
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
      updateDebugSnapshot(data);
      log(`${formatStatusMessage(data)}${match}`, "rx");
    } catch (err) {
      log(`状态 ${topic}: ${text}`, "rx");
    }
  });

  state.client.on("reconnect", () => log("正在重连 MQTT。", "warn"));
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

function stepperEnable(enable) {
  publish({
    ...basePayload("stepper_enable", "enable"),
    channels: [1, 2, 3, 4, 5, 6],
    enable,
  });
}

function stepperMoveRelative(channel, direction, angleInput, label) {
  publish({
    ...basePayload("stepper_move", `move-${channel}`),
    channel,
    mode: "relative",
    relative: true,
    dir: direction,
    angle: readNonNegativeIntegerPayload(angleInput.value, 0),
    enable: controls.enableBeforeMove.checked,
    note: label,
  });
}

function actionCommand(actionId, note = "") {
  publish(buildActionPayload(actionId, note));
}

function servoCommand(channel, input, note = "") {
  publish({
    ...basePayload("servo", `servo-${channel}`),
    channel,
    angle: clampNumber(input.value, 0, 270, 135),
    note,
  });
}

function board1Output(device, enable, note = "") {
  publish(buildBoard1OutputPayload(device, enable, note));
}

function emergency() {
  publish(basePayload("emergency", "emergency"));
}

function updatePreviewFromForm() {
  preview(buildTargetsPayload(true, true));
}

function updateCustomActionPreview() {
  preview(buildActionPayload(controls.customActionId.value, controls.customActionNote.value));
}

$("connectBtn").addEventListener("click", connect);
$("disconnectBtn").addEventListener("click", disconnect);
$("pingBtn").addEventListener("click", () => publish(basePayload("status", "status")));
$("sendAxisTargetsBtn").addEventListener("click", () => publish(buildTargetsPayload(true, false)));
$("sendServoTargetsBtn").addEventListener("click", () => publish(buildTargetsPayload(false, true)));
$("sendAllTargetsBtn").addEventListener("click", () => publish(buildTargetsPayload(true, true)));
$("zeroTargetsBtn").addEventListener("click", () => {
  for (const [, input] of axisFields) input.value = "0";
  updatePreviewFromForm();
});
$("enableBtn").addEventListener("click", () => stepperEnable(true));
$("disableBtn").addEventListener("click", () => stepperEnable(false));
$("centerServosBtn").addEventListener("click", () => {
  for (const [, input] of servoFields) input.value = "135";
  publish(buildTargetsPayload(false, true));
});
$("sendServo1Btn").addEventListener("click", () => servoCommand(1, controls.servo1Angle, "单独设置舵机1"));
$("sendServo2Btn").addEventListener("click", () => servoCommand(2, controls.servo2Angle, "单独设置舵机2"));
$("sendServo3Btn").addEventListener("click", () => servoCommand(3, controls.servo3Angle, "单独设置舵机3 PWME"));
$("sendServo4Btn").addEventListener("click", () => servoCommand(4, controls.servo4Angle, "单独设置舵机4 PWMF"));
$("telescopeBtn").addEventListener("click", () => actionCommand(5, "主板2伸缩轴单步"));
$("actionBtn").addEventListener("click", () => actionCommand(6, "主板2自动放衣流程"));
$("takeClothesBtn").addEventListener("click", () => actionCommand(7, "主板2自动取衣流程"));
$("fanMoveBtn").addEventListener("click", () => actionCommand(8, "主板1风机风扇启动，串口4风机移动"));
$("fanStopBtn").addEventListener("click", () => actionCommand(9, "主板1风机移动停止"));
$("brushCleanBtn").addEventListener("click", () => actionCommand(10, "主板2刷毛清理：舵机4到80度，刷毛电机到-18000，再回0，舵机4回150度"));
$("telescopeForwardBtn").addEventListener("click", () => stepperMoveRelative(4, 0, controls.telescopeStepAngle, "伸缩杆正向"));
$("telescopeReverseBtn").addEventListener("click", () => stepperMoveRelative(4, 1, controls.telescopeStepAngle, "伸缩杆反向"));
$("brushForwardBtn").addEventListener("click", () => stepperMoveRelative(5, 0, controls.brushStepAngle, "刷毛正向"));
$("brushReverseBtn").addEventListener("click", () => stepperMoveRelative(5, 1, controls.brushStepAngle, "刷毛反向"));
$("hotAirForwardBtn").addEventListener("click", () => stepperMoveRelative(6, 0, controls.hotAirStepAngle, "热风机正向"));
$("hotAirReverseBtn").addEventListener("click", () => stepperMoveRelative(6, 1, controls.hotAirStepAngle, "热风机反向"));
$("uvLampOnBtn").addEventListener("click", () => board1Output("uv_lamp", true, "主板1 DIRC 拉高，紫外灯亮"));
$("uvLampOffBtn").addEventListener("click", () => board1Output("uv_lamp", false, "主板1 DIRC 拉低，紫外灯关"));
$("ventFanOnBtn").addEventListener("click", () => board1Output("vent_fan", true, "主板1 PWMB 拉高，换气风扇转"));
$("ventFanOffBtn").addEventListener("click", () => board1Output("vent_fan", false, "主板1 PWMB 拉低，换气风扇停"));
$("taskStopBtn").addEventListener("click", emergency);
$("emergencyBtn").addEventListener("click", emergency);
$("sendCustomActionBtn").addEventListener("click", () => {
  actionCommand(controls.customActionId.value, controls.customActionNote.value);
});
$("previewCustomActionBtn").addEventListener("click", () => {
  updateCustomActionPreview();
  log("已更新自定义任务预览。");
});
$("copyPayloadBtn").addEventListener("click", async () => {
  if (!state.lastPayload) return;
  await navigator.clipboard.writeText(JSON.stringify(state.lastPayload, null, 2));
  log("已复制当前 JSON。");
});
$("clearLogBtn").addEventListener("click", () => {
  controls.log.textContent = "";
});

[...axisFields, ...servoFields].forEach(([, input]) => {
  input.addEventListener("input", updatePreviewFromForm);
});
controls.enableBeforeMove.addEventListener("change", updatePreviewFromForm);
controls.customActionId.addEventListener("input", updateCustomActionPreview);
controls.customActionNote.addEventListener("input", updateCustomActionPreview);

installNumberWheelControls();
updatePreviewFromForm();

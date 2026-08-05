"use strict";

const livePlayer = document.querySelector("#livePlayer");
const videoStage = document.querySelector("#videoStage");
const videoPlaceholder = document.querySelector("#videoPlaceholder");
const placeholderTitle = document.querySelector("#placeholderTitle");
const placeholderText = document.querySelector("#placeholderText");
const systemState = document.querySelector("#systemState");
const systemStateText = document.querySelector("#systemStateText");
const liveBadge = document.querySelector("#liveBadge");
const liveBadgeText = document.querySelector("#liveBadgeText");
const sourceForm = document.querySelector("#sourceForm");
const deviceIpInput = document.querySelector("#deviceIpInput");
const connectButton = document.querySelector("#connectButton");
const videoSpec = document.querySelector("#videoSpec");
const reconnectButton = document.querySelector("#reconnectButton");
const fullscreenButton = document.querySelector("#fullscreenButton");
const recordButton = document.querySelector("#recordButton");
const recordButtonTitle = document.querySelector("#recordButtonTitle");
const recordButtonSubtitle = document.querySelector("#recordButtonSubtitle");
const sessionTimer = document.querySelector("#sessionTimer");
const sessionHint = document.querySelector("#sessionHint");
const recordOverlay = document.querySelector("#recordOverlay");
const recordOverlayTime = document.querySelector("#recordOverlayTime");
const downloadPanel = document.querySelector("#downloadPanel");
const downloadButton = document.querySelector("#downloadButton");
const downloadMeta = document.querySelector("#downloadMeta");
const formatLabel = document.querySelector("#formatLabel");
const toast = document.querySelector("#toast");

const BRIDGE_SOURCE = "maixcam-recorder-bridge";
const HOST_SOURCE = "maixcam-recorder-host";
const DEVICE_IP_STORAGE_KEY = "maixcam.recorder.deviceIp";

let bridgeTargetOrigin = "";
let sourceUrl = "";
let webrtcPort = 8000;
let bridgeReady = false;
let videoOnline = false;
let recording = false;
let recordingStartedAt = 0;
let timerHandle = 0;
let pingHandle = 0;
let bridgeTimeoutHandle = 0;
let toastHandle = 0;
let downloadUrl = "";

function showToast(message) {
  window.clearTimeout(toastHandle);
  toast.textContent = message;
  toast.classList.add("is-visible");
  toastHandle = window.setTimeout(
    () => toast.classList.remove("is-visible"),
    2800,
  );
}

function formatDuration(milliseconds) {
  const totalSeconds = Math.max(0, Math.floor(milliseconds / 1000));
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  return [hours, minutes, seconds]
    .map((value) => String(value).padStart(2, "0"))
    .join(":");
}

function formatBytes(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 ** 2) return `${(bytes / 1024).toFixed(1)} KB`;
  if (bytes < 1024 ** 3) return `${(bytes / 1024 ** 2).toFixed(1)} MB`;
  return `${(bytes / 1024 ** 3).toFixed(2)} GB`;
}

function normalizePrivateIpv4(value) {
  const text = String(value || "").trim();
  const parts = text.split(".");
  if (
    parts.length !== 4 ||
    parts.some(
      (part) =>
        !/^\d{1,3}$/.test(part) || Number(part) < 0 || Number(part) > 255,
    )
  ) {
    throw new Error("请输入正确的 IPv4 地址");
  }

  const numbers = parts.map(Number);
  const isPrivate =
    numbers[0] === 10 ||
    (numbers[0] === 192 && numbers[1] === 168) ||
    (numbers[0] === 172 && numbers[1] >= 16 && numbers[1] <= 31);
  if (!isPrivate) throw new Error("请输入局域网私有 IP 地址");
  return numbers.join(".");
}

function connectToDevice(ip, { save = false, manual = false } = {}) {
  const normalizedIp = normalizePrivateIpv4(ip);
  sourceUrl = `http://${normalizedIp}:${webrtcPort}/`;
  bridgeTargetOrigin = new URL(sourceUrl).origin;
  deviceIpInput.value = normalizedIp;
  if (save) localStorage.setItem(DEVICE_IP_STORAGE_KEY, normalizedIp);
  loadNativeWebRtc(manual);
}

function setConnectionState(state, detail = "") {
  systemState.classList.remove("is-online", "is-error");
  liveBadge.classList.remove("is-online", "is-error");

  if (state === "online") {
    systemState.classList.add("is-online");
    liveBadge.classList.add("is-online");
    systemStateText.textContent = "视频在线";
    liveBadgeText.textContent = "LIVE";
    videoPlaceholder.classList.add("is-hidden");
    recordButton.disabled = recording;
    sessionHint.textContent = recording
      ? "正在记录本次测试"
      : "原生 WebRTC 已连接，可以开始本次记录";
    return;
  }

  if (state === "bridge-error") {
    systemState.classList.add("is-error");
    liveBadge.classList.add("is-error");
    systemStateText.textContent = "录像桥接未连接";
    liveBadgeText.textContent = "VIEW ONLY";
    videoPlaceholder.classList.add("is-hidden");
    recordButton.disabled = true;
    sessionHint.textContent = "画面可以查看；重新加载录像扩展后即可录制";
    return;
  }

  videoPlaceholder.classList.remove("is-hidden");
  recordButton.disabled = true;

  if (state === "error") {
    systemState.classList.add("is-error");
    liveBadge.classList.add("is-error");
    systemStateText.textContent = "连接异常";
    liveBadgeText.textContent = "OFFLINE";
    placeholderTitle.textContent = "未收到 MaixCAM 画面";
    placeholderText.textContent =
      detail || "请检查原生 WebRTC 程序、设备 IP 和浏览器扩展";
    sessionHint.textContent = "请先恢复视频连接";
    return;
  }

  systemStateText.textContent = "正在连接";
  liveBadgeText.textContent = "CONNECTING";
  placeholderTitle.textContent = "正在连接视频源";
  placeholderText.textContent =
    detail || "正在等待 MaixCAM 原生 WebRTC 页面和录像桥接";
  sessionHint.textContent = "画面连接后即可开始录制";
}

function sendBridge(command, extra = {}) {
  if (!livePlayer.contentWindow || !bridgeTargetOrigin) return;
  livePlayer.contentWindow.postMessage(
    {
      source: HOST_SOURCE,
      command,
      ...extra,
    },
    bridgeTargetOrigin,
  );
}

function updateTimer() {
  const text = formatDuration(Date.now() - recordingStartedAt);
  sessionTimer.textContent = text;
  recordOverlayTime.textContent = text;
}

function resetDownload() {
  if (downloadUrl) {
    URL.revokeObjectURL(downloadUrl);
    downloadUrl = "";
  }
  downloadPanel.hidden = true;
  downloadButton.removeAttribute("href");
}

function setRecordingUi(active, label = "") {
  recording = active;
  window.clearInterval(timerHandle);
  timerHandle = 0;

  if (active) {
    recordingStartedAt = Date.now();
    sessionTimer.textContent = "00:00:00";
    recordOverlayTime.textContent = "00:00:00";
    timerHandle = window.setInterval(updateTimer, 250);
    recordButton.disabled = false;
    recordButton.classList.add("is-recording");
    recordButtonTitle.textContent = "完成录制";
    recordButtonSubtitle.textContent = "STOP & SAVE";
    recordOverlay.classList.add("is-active");
    sessionHint.textContent = "正在记录本次测试，请勿刷新页面";
    deviceIpInput.disabled = true;
    connectButton.disabled = true;
    if (label) formatLabel.textContent = label;
    return;
  }

  recordButton.classList.remove("is-recording");
  recordButtonTitle.textContent = "开始录制";
  recordButtonSubtitle.textContent = "RECORD SESSION";
  recordOverlay.classList.remove("is-active");
  recordButton.disabled = !videoOnline;
  deviceIpInput.disabled = false;
  connectButton.disabled = false;
}

function handleBridgeMessage(event) {
  if (event.source !== livePlayer.contentWindow) return;
  if (bridgeTargetOrigin && event.origin !== bridgeTargetOrigin) return;

  const data = event.data;
  if (!data || data.source !== BRIDGE_SOURCE) return;

  if (data.type === "bridge-ready") {
    bridgeReady = true;
    window.clearTimeout(bridgeTimeoutHandle);
    setConnectionState(
      "connecting",
      "录像桥接已就绪，正在等待 WebRTC 视频首帧",
    );
    sendBridge("status");
    return;
  }

  if (data.type === "video-ready") {
    bridgeReady = true;
    videoOnline = true;
    window.clearTimeout(bridgeTimeoutHandle);
    videoSpec.textContent = `${data.width || "—"} × ${data.height || "—"}`;
    setConnectionState("online");
    return;
  }

  if (data.type === "video-waiting") {
    videoOnline = false;
    if (!recording) {
      videoSpec.textContent = "等待视频";
      setConnectionState(
        "connecting",
        data.message || "播放器已打开，正在等待 WebRTC 视频首帧",
      );
    }
    return;
  }

  if (data.type === "recording-started") {
    resetDownload();
    setRecordingUi(true, data.label || "浏览器录像");
    showToast("已开始录制");
    return;
  }

  if (data.type === "recording-finalizing") {
    recordButton.disabled = true;
    recordButtonTitle.textContent = "正在生成录像";
    recordButtonSubtitle.textContent = "FINALIZING";
    sessionHint.textContent = "正在封装录像文件，请稍候";
    return;
  }

  if (data.type === "recording-complete") {
    setRecordingUi(false, data.label || formatLabel.textContent);

    if (!(data.blob instanceof Blob) || data.blob.size === 0) {
      sessionHint.textContent = "录像文件为空，请重新录制";
      showToast("未收到有效录像数据");
      return;
    }

    downloadUrl = URL.createObjectURL(data.blob);
    downloadButton.href = downloadUrl;
    downloadButton.download =
      data.filename || `maixcam_recording.${data.extension || "webm"}`;
    downloadMeta.textContent = `${formatDuration(
      data.duration || Date.now() - recordingStartedAt,
    )} · ${formatBytes(data.blob.size)} · ${(
      data.extension || "webm"
    ).toUpperCase()}`;
    downloadPanel.hidden = false;
    sessionHint.textContent = "录像已生成，请点击下载保存";
    showToast("录像已生成");
    return;
  }

  if (data.type === "recording-error") {
    setRecordingUi(false);
    sessionHint.textContent = "录制失败，请检查提示后重试";
    showToast(data.message || "录制失败");
  }
}

function armBridgeTimeout() {
  window.clearTimeout(bridgeTimeoutHandle);
  bridgeTimeoutHandle = window.setTimeout(() => {
    if (bridgeReady) return;
    setConnectionState(
      "bridge-error",
      "画面页已打开，但录像桥接扩展尚未连接",
    );
  }, 10000);
}

function loadNativeWebRtc(manual = false) {
  if (!sourceUrl) return;
  bridgeReady = false;
  videoOnline = false;
  setConnectionState(
    "connecting",
    manual
      ? "正在重新加载 MaixCAM 原生 WebRTC"
      : "正在打开 MaixCAM 原生 WebRTC",
  );

  const joiner = sourceUrl.includes("?") ? "&" : "?";
  livePlayer.src = `${sourceUrl}${joiner}host_ts=${Date.now()}`;
  armBridgeTimeout();
}

async function fetchStatus() {
  try {
    const response = await fetch("/api/status", { cache: "no-store" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const status = await response.json();
    webrtcPort = Number(status.source.port) || 8000;
    const savedIp = localStorage.getItem(DEVICE_IP_STORAGE_KEY);
    let initialIp = status.source.ip;
    if (savedIp) {
      try {
        initialIp = normalizePrivateIpv4(savedIp);
      } catch {
        localStorage.removeItem(DEVICE_IP_STORAGE_KEY);
      }
    }
    connectToDevice(initialIp);
  } catch (error) {
    setConnectionState("error", `读取设备配置失败：${error.message}`);
  }
}

window.addEventListener("message", handleBridgeMessage);

livePlayer.addEventListener("load", () => {
  setConnectionState(
    "connecting",
    "原生 WebRTC 页面已加载，正在等待视频与录像桥接",
  );
  window.setTimeout(() => sendBridge("hello"), 250);
  window.setTimeout(() => sendBridge("status"), 900);
});

recordButton.addEventListener("click", () => {
  if (!bridgeReady || !videoOnline) {
    showToast("录像桥接或视频尚未就绪");
    return;
  }

  if (recording) {
    sendBridge("stop");
    recordButton.disabled = true;
    return;
  }

  resetDownload();
  recordButton.disabled = true;
  sessionHint.textContent = "正在启动录像";
  sendBridge("start");
});

sourceForm.addEventListener("submit", (event) => {
  event.preventDefault();
  if (recording) {
    showToast("请先完成当前录像");
    return;
  }

  try {
    connectToDevice(deviceIpInput.value, { save: true, manual: true });
    showToast(`正在连接 ${deviceIpInput.value.trim()}`);
  } catch (error) {
    showToast(error.message || "设备 IP 无效");
    deviceIpInput.focus();
    deviceIpInput.select();
  }
});

reconnectButton.addEventListener("click", () => {
  if (recording) {
    showToast("请先完成当前录像");
    return;
  }
  loadNativeWebRtc(true);
});

fullscreenButton.addEventListener("click", async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await videoStage.requestFullscreen();
  } catch {
    showToast("浏览器未允许进入全屏");
  }
});

window.addEventListener("beforeunload", (event) => {
  if (recording) {
    event.preventDefault();
    event.returnValue = "";
  }
});

window.addEventListener("pagehide", () => {
  window.clearInterval(timerHandle);
  window.clearInterval(pingHandle);
  window.clearTimeout(bridgeTimeoutHandle);
  if (downloadUrl) URL.revokeObjectURL(downloadUrl);
});

pingHandle = window.setInterval(() => {
  if (!bridgeReady) sendBridge("hello");
  else if (!recording) sendBridge("status");
}, 2500);

setConnectionState("connecting", "正在读取 MaixCAM WebRTC 配置");
fetchStatus();

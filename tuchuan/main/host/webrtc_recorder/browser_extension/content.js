(() => {
  "use strict";

  function isPrivateDevicePage() {
    const host = window.location.hostname;
    const isPrivateHost =
      host.startsWith("10.") ||
      host.startsWith("192.168.") ||
      /^172\.(1[6-9]|2\d|3[01])\./.test(host);
    return (
      window.location.protocol === "http:" &&
      window.location.port === "8000" &&
      isPrivateHost
    );
  }

  if (!isPrivateDevicePage()) return;
  if (window.top !== window) return;
  if (document.querySelector("#mx-recorder-console")) return;

  const state = {
    video: null,
    recorder: null,
    stream: null,
    chunks: [],
    startedAt: 0,
    timer: 0,
    downloadUrl: "",
    selected: null,
  };

  const consoleRoot = document.createElement("section");
  consoleRoot.id = "mx-recorder-console";
  consoleRoot.setAttribute("aria-label", "MaixCAM WebRTC 录像台");
  consoleRoot.innerHTML = `
    <div class="mx-console-head">
      <div class="mx-brand">
        <strong>MAIXCAM · 录像台</strong>
        <span>NATIVE WEBRTC / LOCAL RECORD</span>
      </div>
      <div class="mx-status" id="mxStatus"><i></i><span>寻找画面</span></div>
    </div>
    <div class="mx-console-body">
      <div class="mx-readout">
        <span>本次记录</span>
        <time id="mxTimer">00:00:00</time>
      </div>
      <button class="mx-record-button" id="mxRecordButton" type="button" disabled>
        <span class="mx-record-icon"><i></i></span>
        <span class="mx-record-copy">
          <b id="mxRecordTitle">开始录制</b>
          <small id="mxRecordSubtitle">RECORD SESSION</small>
        </span>
        <span class="mx-arrow">›</span>
      </button>
      <p class="mx-message" id="mxMessage">正在等待 MaixCAM WebRTC 画面。</p>
      <div class="mx-download" id="mxDownload">
        <div class="mx-download-copy">
          <strong>录像已生成</strong>
          <span id="mxDownloadMeta">准备下载</span>
        </div>
        <a id="mxDownloadButton" href="#" download>下载录像</a>
      </div>
    </div>
    <div class="mx-console-foot">
      <span id="mxVideoSpec">WAITING VIDEO</span>
      <span id="mxFormat">LOCAL ONLY</span>
    </div>
  `;

  const liveFlag = document.createElement("div");
  liveFlag.className = "mx-live-flag";
  liveFlag.innerHTML = "<i></i><span>REC</span>";

  document.body.append(consoleRoot, liveFlag);

  const status = consoleRoot.querySelector("#mxStatus");
  const statusText = status.querySelector("span");
  const timer = consoleRoot.querySelector("#mxTimer");
  const recordButton = consoleRoot.querySelector("#mxRecordButton");
  const recordTitle = consoleRoot.querySelector("#mxRecordTitle");
  const recordSubtitle = consoleRoot.querySelector("#mxRecordSubtitle");
  const message = consoleRoot.querySelector("#mxMessage");
  const downloadPanel = consoleRoot.querySelector("#mxDownload");
  const downloadMeta = consoleRoot.querySelector("#mxDownloadMeta");
  const downloadButton = consoleRoot.querySelector("#mxDownloadButton");
  const videoSpec = consoleRoot.querySelector("#mxVideoSpec");
  const format = consoleRoot.querySelector("#mxFormat");

  function setMessage(text, isError = false) {
    message.textContent = text;
    message.classList.toggle("is-error", isError);
  }

  function setStatus(kind, text) {
    status.classList.toggle("is-online", kind === "online");
    status.classList.toggle("is-error", kind === "error");
    statusText.textContent = text;
  }

  function formatDuration(milliseconds) {
    const total = Math.max(0, Math.floor(milliseconds / 1000));
    const hours = Math.floor(total / 3600);
    const minutes = Math.floor((total % 3600) / 60);
    const seconds = total % 60;
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

  function timestamp() {
    const date = new Date();
    const pad = (value) => String(value).padStart(2, "0");
    return [
      date.getFullYear(),
      pad(date.getMonth() + 1),
      pad(date.getDate()),
      "_",
      pad(date.getHours()),
      pad(date.getMinutes()),
      pad(date.getSeconds()),
    ].join("");
  }

  function chooseRecordingType() {
    const candidates = [
      { mime: "video/mp4;codecs=avc1.42E01E", ext: "mp4", label: "H.264 / MP4" },
      { mime: "video/mp4", ext: "mp4", label: "MP4" },
      { mime: "video/webm;codecs=vp9", ext: "webm", label: "VP9 / WebM" },
      { mime: "video/webm;codecs=vp8", ext: "webm", label: "VP8 / WebM" },
      { mime: "video/webm", ext: "webm", label: "WebM" },
    ];
    return (
      candidates.find((item) => MediaRecorder.isTypeSupported(item.mime)) || {
        mime: "",
        ext: "webm",
        label: "浏览器默认格式",
      }
    );
  }

  function findVideo() {
    const videos = [...document.querySelectorAll("video")];
    return (
      videos.find(
        (item) =>
          item.videoWidth > 0 &&
          item.videoHeight > 0 &&
          item.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA,
      ) ||
      videos.find((item) => item.srcObject) ||
      videos[0] ||
      null
    );
  }

  function updateVideoState() {
    if (state.recorder?.state === "recording") return;

    const nextVideo = findVideo();
    state.video = nextVideo;

    if (!nextVideo) {
      recordButton.disabled = true;
      videoSpec.textContent = "WAITING VIDEO";
      setStatus("waiting", "寻找画面");
      setMessage("未发现视频元素，请等待 WebRTC 页面加载。");
      return;
    }

    if (
      nextVideo.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA &&
      nextVideo.videoWidth > 0 &&
      nextVideo.videoHeight > 0
    ) {
      recordButton.disabled = false;
      videoSpec.textContent = `${nextVideo.videoWidth} × ${nextVideo.videoHeight}`;
      setStatus("online", "画面在线");
      setMessage("原生 WebRTC 已连接，可以开始本次测试录像。");
      return;
    }

    recordButton.disabled = true;
    setStatus("waiting", "正在连接");
    setMessage("已找到播放器，正在等待 MaixCAM 视频首帧。");
  }

  function createRecordingStream(video) {
    const source = video.srcObject;
    if (source && typeof source.getTracks === "function") {
      const tracks = source
        .getTracks()
        .filter((track) => track.readyState === "live")
        .map((track) => track.clone());
      if (tracks.some((track) => track.kind === "video")) {
        return new MediaStream(tracks);
      }
    }

    const capture = video.captureStream || video.mozCaptureStream;
    if (!capture) {
      throw new Error("当前浏览器不支持视频录制，请使用最新版 Edge 或 Chrome。");
    }
    const stream = capture.call(video);
    if (!stream.getVideoTracks().length) {
      throw new Error("未获得有效视频轨，请确认画面正在播放。");
    }
    return stream;
  }

  function resetDownload() {
    if (state.downloadUrl) {
      URL.revokeObjectURL(state.downloadUrl);
      state.downloadUrl = "";
    }
    downloadPanel.classList.remove("is-visible");
    downloadButton.removeAttribute("href");
  }

  function updateTimer() {
    timer.textContent = formatDuration(Date.now() - state.startedAt);
  }

  function resetRecordingUi() {
    window.clearInterval(state.timer);
    state.timer = 0;
    recordButton.classList.remove("is-recording");
    recordTitle.textContent = "开始录制";
    recordSubtitle.textContent = "RECORD SESSION";
    liveFlag.classList.remove("is-visible");
  }

  function startRecording() {
    if (!state.video || state.video.readyState < HTMLMediaElement.HAVE_CURRENT_DATA) {
      setMessage("画面尚未就绪，请稍后再试。", true);
      return;
    }

    if (typeof MediaRecorder === "undefined") {
      setMessage("当前浏览器缺少 MediaRecorder，请升级 Edge 或 Chrome。", true);
      return;
    }

    resetDownload();

    try {
      state.stream = createRecordingStream(state.video);
      state.selected = chooseRecordingType();
      const options = state.selected.mime
        ? { mimeType: state.selected.mime, videoBitsPerSecond: 5_000_000 }
        : { videoBitsPerSecond: 5_000_000 };

      state.recorder = new MediaRecorder(state.stream, options);
      state.chunks = [];
      state.startedAt = Date.now();

      state.recorder.addEventListener("dataavailable", (event) => {
        if (event.data?.size) state.chunks.push(event.data);
      });

      state.recorder.addEventListener("error", (event) => {
        setMessage(`录制失败：${event.error?.message || "未知错误"}`, true);
        state.stream?.getTracks().forEach((track) => track.stop());
        state.stream = null;
        state.recorder = null;
        resetRecordingUi();
      });

      state.recorder.addEventListener("stop", () => {
        const elapsed = Date.now() - state.startedAt;
        const finalMime =
          state.recorder?.mimeType || state.selected.mime || "video/webm";
        const extension = finalMime.includes("mp4") ? "mp4" : state.selected.ext;
        const blob = new Blob(state.chunks, { type: finalMime });
        const filename = `maixcam_${timestamp()}.${extension}`;

        state.downloadUrl = URL.createObjectURL(blob);
        downloadButton.href = state.downloadUrl;
        downloadButton.download = filename;
        downloadMeta.textContent = `${formatDuration(elapsed)} · ${formatBytes(blob.size)} · ${extension.toUpperCase()}`;
        downloadPanel.classList.add("is-visible");

        state.stream?.getTracks().forEach((track) => track.stop());
        state.stream = null;
        state.chunks = [];
        state.recorder = null;
        resetRecordingUi();
        format.textContent = state.selected.label;
        setMessage("录像已生成，点击“下载录像”保存到电脑。");
      });

      state.recorder.start(1000);
      timer.textContent = "00:00:00";
      state.timer = window.setInterval(updateTimer, 250);
      recordButton.classList.add("is-recording");
      recordTitle.textContent = "完成录制";
      recordSubtitle.textContent = "STOP & PREPARE DOWNLOAD";
      liveFlag.classList.add("is-visible");
      format.textContent = state.selected.label;
      setMessage("正在记录本次测试；完成后再次点击红色按钮。");
    } catch (error) {
      state.stream?.getTracks().forEach((track) => track.stop());
      state.stream = null;
      state.recorder = null;
      setMessage(error.message || "无法开始录制。", true);
    }
  }

  function stopRecording() {
    if (!state.recorder || state.recorder.state === "inactive") return;
    recordButton.disabled = true;
    recordTitle.textContent = "正在生成录像";
    recordSubtitle.textContent = "FINALIZING";
    setMessage("正在封装录像文件，请稍候。");
    state.recorder.stop();
    window.setTimeout(updateVideoState, 250);
  }

  recordButton.addEventListener("click", () => {
    if (state.recorder?.state === "recording") stopRecording();
    else startRecording();
  });

  window.addEventListener("beforeunload", (event) => {
    if (state.recorder?.state === "recording") {
      event.preventDefault();
      event.returnValue = "";
    }
  });

  window.addEventListener("pagehide", () => {
    window.clearInterval(state.timer);
    state.stream?.getTracks().forEach((track) => track.stop());
    if (state.downloadUrl) URL.revokeObjectURL(state.downloadUrl);
  });

  updateVideoState();
  window.setInterval(updateVideoState, 1000);
})();

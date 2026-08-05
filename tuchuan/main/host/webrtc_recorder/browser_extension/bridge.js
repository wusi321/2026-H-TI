(() => {
  "use strict";

  if (window.top === window) return;

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

  const BRIDGE_SOURCE = "maixcam-recorder-bridge";
  const HOST_SOURCE = "maixcam-recorder-host";

  let trustedParentOrigin = "";
  let recorder = null;
  let recordingStream = null;
  let chunks = [];
  let startedAt = 0;
  let selected = null;
  let lastVideoReady = false;

  function send(type, extra = {}) {
    window.parent.postMessage(
      {
        source: BRIDGE_SOURCE,
        type,
        ...extra,
      },
      trustedParentOrigin || "*",
    );
  }

  function acceptParentOrigin(origin) {
    try {
      const url = new URL(origin);
      const host = url.hostname;
      const isPrivateHost =
        host === "localhost" ||
        host === "127.0.0.1" ||
        host.startsWith("10.") ||
        host.startsWith("192.168.") ||
        /^172\.(1[6-9]|2\d|3[01])\./.test(host);
      return (
        url.protocol === "http:" &&
        url.port === "18765" &&
        isPrivateHost
      );
    } catch {
      return false;
    }
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

  function reportStatus(force = false) {
    if (recorder?.state === "recording") return;

    const video = findVideo();
    const ready = Boolean(
      video &&
        video.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA &&
        video.videoWidth > 0 &&
        video.videoHeight > 0,
    );

    if (ready) {
      if (force || !lastVideoReady) {
        send("video-ready", {
          width: video.videoWidth,
          height: video.videoHeight,
        });
      }
      lastVideoReady = true;
      return;
    }

    if (force || lastVideoReady) {
      send("video-waiting", {
        message: video
          ? "已找到播放器，正在等待 WebRTC 视频首帧"
          : "正在等待 MaixCAM WebRTC 播放器",
      });
    }
    lastVideoReady = false;
  }

  function chooseRecordingType() {
    const candidates = [
      {
        mime: "video/mp4;codecs=avc1.42E01E",
        ext: "mp4",
        label: "H.264 / MP4",
      },
      { mime: "video/mp4", ext: "mp4", label: "MP4" },
      {
        mime: "video/webm;codecs=vp9",
        ext: "webm",
        label: "VP9 / WebM",
      },
      {
        mime: "video/webm;codecs=vp8",
        ext: "webm",
        label: "VP8 / WebM",
      },
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
      throw new Error("当前浏览器不支持视频轨录制，请升级 Edge 或 Chrome");
    }
    const stream = capture.call(video);
    if (!stream.getVideoTracks().length) {
      throw new Error("未获取到有效视频轨，请等待画面出现后重试");
    }
    return stream;
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

  function cleanupStream() {
    recordingStream?.getTracks().forEach((track) => track.stop());
    recordingStream = null;
  }

  function fail(message) {
    cleanupStream();
    recorder = null;
    chunks = [];
    send("recording-error", { message });
  }

  function startRecording() {
    if (recorder?.state === "recording") return;
    if (typeof MediaRecorder === "undefined") {
      fail("当前浏览器缺少 MediaRecorder，请升级 Edge 或 Chrome");
      return;
    }

    const video = findVideo();
    if (
      !video ||
      video.readyState < HTMLMediaElement.HAVE_CURRENT_DATA ||
      !video.videoWidth
    ) {
      fail("WebRTC 画面尚未就绪，请稍后重试");
      return;
    }

    try {
      recordingStream = createRecordingStream(video);
      selected = chooseRecordingType();
      const options = selected.mime
        ? { mimeType: selected.mime, videoBitsPerSecond: 5_000_000 }
        : { videoBitsPerSecond: 5_000_000 };

      recorder = new MediaRecorder(recordingStream, options);
      chunks = [];
      startedAt = Date.now();

      recorder.addEventListener("dataavailable", (event) => {
        if (event.data?.size) chunks.push(event.data);
      });

      recorder.addEventListener("error", (event) => {
        fail(event.error?.message || "浏览器录像器发生错误");
      });

      recorder.addEventListener("stop", () => {
        const duration = Date.now() - startedAt;
        const finalMime =
          recorder?.mimeType || selected?.mime || "video/webm";
        const extension = finalMime.includes("mp4")
          ? "mp4"
          : selected?.ext || "webm";
        const blob = new Blob(chunks, { type: finalMime });

        cleanupStream();
        recorder = null;
        chunks = [];

        if (!blob.size) {
          send("recording-error", { message: "录像数据为空，请重新录制" });
          return;
        }

        send("recording-complete", {
          blob,
          duration,
          extension,
          label: selected?.label || "浏览器录像",
          filename: `maixcam_${timestamp()}.${extension}`,
        });
        reportStatus(true);
      });

      recorder.start(1000);
      send("recording-started", { label: selected.label });
    } catch (error) {
      fail(error.message || "无法开始录制");
    }
  }

  function stopRecording() {
    if (!recorder || recorder.state === "inactive") return;
    send("recording-finalizing");
    recorder.stop();
  }

  window.addEventListener("message", (event) => {
    if (event.source !== window.parent) return;
    const data = event.data;
    if (!data || data.source !== HOST_SOURCE) return;
    if (!acceptParentOrigin(event.origin)) return;

    trustedParentOrigin = event.origin;

    if (data.command === "hello") {
      send("bridge-ready");
      reportStatus(true);
    } else if (data.command === "status") {
      reportStatus(true);
    } else if (data.command === "start") {
      startRecording();
    } else if (data.command === "stop") {
      stopRecording();
    }
  });

  send("bridge-ready");
  reportStatus(true);
  window.setInterval(() => reportStatus(false), 1000);
})();

"use strict";

const http = require("http");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { spawn } = require("child_process");

const rootDir = __dirname;
const webDir = path.join(rootDir, "web");
const configPath = path.join(rootDir, "device.json");

function readConfig() {
  const value = JSON.parse(fs.readFileSync(configPath, "utf8"));
  const deviceIp = String(value.deviceIp || "").trim();
  const webrtcPort = Number(value.webrtcPort || 8000);
  const webPort = Number(value.webPort);

  if (!/^[a-zA-Z0-9.-]+$/.test(deviceIp)) {
    throw new Error("device.json: invalid deviceIp");
  }
  if (!Number.isInteger(webrtcPort) || webrtcPort < 1 || webrtcPort > 65535) {
    throw new Error("device.json: invalid webrtcPort");
  }
  if (!Number.isInteger(webPort) || webPort < 1 || webPort > 65535) {
    throw new Error("device.json: invalid webPort");
  }

  return { deviceIp, webrtcPort, webPort };
}

function localIpv4Addresses() {
  const addresses = new Set();
  for (const group of Object.values(os.networkInterfaces())) {
    for (const item of group || []) {
      if (
        item.family === "IPv4" &&
        !item.internal &&
        !item.address.startsWith("169.254.")
      ) {
        addresses.add(item.address);
      }
    }
  }
  return [...addresses];
}

function contentType(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  return (
    {
      ".html": "text/html; charset=utf-8",
      ".css": "text/css; charset=utf-8",
      ".js": "text/javascript; charset=utf-8",
      ".json": "application/json; charset=utf-8",
      ".svg": "image/svg+xml",
      ".png": "image/png",
    }[ext] || "application/octet-stream"
  );
}

function sendJson(res, statusCode, value) {
  const body = Buffer.from(JSON.stringify(value));
  res.writeHead(statusCode, {
    "Content-Type": "application/json; charset=utf-8",
    "Content-Length": body.length,
    "Cache-Control": "no-store",
  });
  res.end(body);
}

function serveStatic(req, res, pathname) {
  let relative;
  try {
    relative =
      pathname === "/"
        ? "index.html"
        : decodeURIComponent(pathname).replace(/^\/+/, "");
  } catch {
    sendJson(res, 400, { ok: false, error: "Bad path" });
    return;
  }

  const resolvedWebDir = path.resolve(webDir);
  const candidate = path.resolve(webDir, relative);
  if (
    candidate !== path.join(resolvedWebDir, "index.html") &&
    !candidate.startsWith(`${resolvedWebDir}${path.sep}`)
  ) {
    sendJson(res, 403, { ok: false, error: "Forbidden" });
    return;
  }

  fs.stat(candidate, (error, stat) => {
    if (error || !stat.isFile()) {
      sendJson(res, 404, { ok: false, error: "Not found" });
      return;
    }

    res.writeHead(200, {
      "Content-Type": contentType(candidate),
      "Content-Length": stat.size,
      "Cache-Control": "no-store",
      "X-Content-Type-Options": "nosniff",
      "Referrer-Policy": "no-referrer",
    });
    if (req.method === "HEAD") {
      res.end();
      return;
    }
    fs.createReadStream(candidate).pipe(res);
  });
}

const config = readConfig();
let shuttingDown = false;

const server = http.createServer((req, res) => {
  const url = new URL(req.url, "http://localhost");

  if (req.method !== "GET" && req.method !== "HEAD") {
    sendJson(res, 405, { ok: false, error: "Method not allowed" });
    return;
  }

  if (url.pathname === "/api/status") {
    sendJson(res, 200, {
      ok: true,
      source: {
        ip: config.deviceIp,
        port: config.webrtcPort,
        url: `http://${config.deviceIp}:${config.webrtcPort}/`,
      },
      transport: "MaixCAM Native WebRTC",
      bridgeRequired: true,
    });
    return;
  }

  serveStatic(req, res, url.pathname);
});

function openBrowser(url) {
  if (
    process.env.MAIXCAM_NO_BROWSER === "1" ||
    process.argv.includes("--no-browser")
  ) {
    return;
  }

  const child = spawn("cmd.exe", ["/d", "/c", "start", "", url], {
    detached: true,
    windowsHide: true,
    stdio: "ignore",
  });
  child.unref();
}

function shutdown(signal) {
  if (shuttingDown) return;
  shuttingDown = true;
  console.log(`\nReceived ${signal}. Stopping web recorder...`);
  server.close(() => process.exit(0));
  setTimeout(() => process.exit(0), 1000).unref();
}

process.on("SIGINT", () => shutdown("Ctrl+C"));
process.on("SIGTERM", () => shutdown("termination signal"));

server.listen(config.webPort, "0.0.0.0", () => {
  const localUrl = `http://127.0.0.1:${config.webPort}`;
  console.log("============================================================");
  console.log(" MaixCAM Native WebRTC recorder started");
  console.log(` Local: ${localUrl}`);
  for (const address of localIpv4Addresses()) {
    console.log(` LAN:   http://${address}:${config.webPort}`);
  }
  console.log(
    ` Source: http://${config.deviceIp}:${config.webrtcPort}/ (Native WebRTC)`,
  );
  console.log("============================================================");
  setTimeout(() => openBrowser(localUrl), 700);
});

server.on("error", (error) => {
  console.error(`\n[ERROR] Web service failed: ${error.message}`);
  if (error.code === "EADDRINUSE") {
    console.error(`Port ${config.webPort} is already in use.`);
  }
  process.exitCode = 1;
  shutdown("server error");
});

const statusText = document.getElementById('statusText');
const portsSelect = document.getElementById('ports');
const refreshBtn = document.getElementById('refresh');
const openBtn = document.getElementById('openBtn');
const closeBtn = document.getElementById('closeBtn');
const sendBtn = document.getElementById('sendBtn');
const sendData = document.getElementById('sendData');
const sendFormat = document.getElementById('sendFormat');
const displayFormat = document.getElementById('displayFormat');
const log = document.getElementById('log');
const clearBtn = document.getElementById('clearBtn');

let ws;
let connectedPort = null;
let isOpen = false;

function setStatus(text) {
  statusText.textContent = text;
}

function appendLog(line) {
  log.textContent += line + '\n';
  log.scrollTop = log.scrollHeight;
}

function formatData(bytes) {
  if (displayFormat.value === 'hex') {
    return bytes.map(b => b.toString(16).padStart(2, '0')).join(' ');
  }
  return bytes.map(b => String.fromCharCode(b)).join('');
}

function toBytes(str, format) {
  if (format === 'hex') {
    const clean = str.replace(/[^0-9a-fA-F]/g, '');
    const bytes = [];
    for (let i = 0; i < clean.length; i += 2) {
      bytes.push(parseInt(clean.substr(i, 2), 16));
    }
    return bytes;
  }
  return Array.from(new TextEncoder().encode(str));
}

async function api(path, method = 'GET', body) {
  const opts = { method };
  if (body) {
    opts.headers = { 'Content-Type': 'application/json' };
    opts.body = JSON.stringify(body);
  }
  const res = await fetch(path, opts);
  return res.json();
}

async function refreshPorts() {
  const data = await api('/api/ports');
  portsSelect.innerHTML = '';
  (data.ports || []).forEach(port => {
    const opt = document.createElement('option');
    opt.value = port;
    opt.textContent = port;
    portsSelect.appendChild(opt);
  });
}

function connectWs() {
  if (ws) {
    ws.close();
  }
  const url = (location.protocol === 'https:' ? 'wss' : 'ws') + '://' + location.host + '/';
  ws = new WebSocket(url);
  ws.onopen = () => {
    setStatus('ws connected');
  };
  ws.onmessage = (event) => {
    try {
      const msg = JSON.parse(event.data);
      if (msg.port && msg.hex) {
        const format = displayFormat.value;
        const text = format === 'ascii' ? msg.ascii : msg.hex;
        appendLog(`[${msg.port}] ${text}`);
      } else {
        appendLog(`[ws] ${event.data}`);
      }
    } catch (e) {
      appendLog(`[ws] ${event.data}`);
    }
  };
  ws.onclose = () => {
    setStatus('ws disconnected');
  };
}

async function openPort() {
  const port = portsSelect.value;
  const body = {
    port,
    baud: Number(document.getElementById('baud').value),
    databits: Number(document.getElementById('databits').value),
    stopbits: Number(document.getElementById('stopbits').value),
    parity: document.getElementById('parity').value,
  };
  const resp = await api('/api/port/open', 'POST', body);
  if (resp.code === 0) {
    isOpen = true;
    connectedPort = port;
    openBtn.disabled = true;
    closeBtn.disabled = false;
    setStatus(`open ${port}`);
  } else {
    appendLog('[error] open failed');
  }
}

async function closePort() {
  const port = connectedPort || portsSelect.value;
  const resp = await api('/api/port/close', 'POST', { port });
  if (resp.code === 0) {
    isOpen = false;
    connectedPort = null;
    openBtn.disabled = false;
    closeBtn.disabled = true;
    setStatus('closed');
  } else {
    appendLog('[error] close failed');
  }
}

async function send() {
  if (!isOpen) {
    appendLog('[warn] port not open');
    return;
  }
  const port = connectedPort;
  const data = sendData.value;
  const format = sendFormat.value;
  const resp = await api('/api/port/write', 'POST', { port, data, format });
  if (resp.code === 0) {
    appendLog(`[send] ${data}`);
  } else {
    appendLog('[error] send failed');
  }
}

refreshBtn.addEventListener('click', refreshPorts);
openBtn.addEventListener('click', openPort);
closeBtn.addEventListener('click', closePort);
sendBtn.addEventListener('click', send);
clearBtn.addEventListener('click', () => (log.textContent = ''));

window.addEventListener('load', async () => {
  await refreshPorts();
  connectWs();
  setStatus('ready');
});

// ── DOM refs ──────────────────────────────────────────

const statusLed = document.getElementById('statusLed');
const ledEl = statusLed.querySelector('.led');
const statusText = document.getElementById('statusText');
const portsSelect = document.getElementById('ports');
const refreshBtn = document.getElementById('refresh');
const openBtn = document.getElementById('openBtn');
const closeBtn = document.getElementById('closeBtn');
const sendBtn = document.getElementById('sendBtn');
const sendData = document.getElementById('sendData');
const sendFormat = document.getElementById('sendFormat');
const displayFormat = document.getElementById('displayFormat');
const logEl = document.getElementById('log');
const clearBtn = document.getElementById('clearBtn');
const scrollLockBtn = document.getElementById('scrollLockBtn');

let ws;
let connectedPort = null;
let isOpen = false;
let autoScroll = true;

// ── Status ───────────────────────────────────────────

function setStatus(text, state) {
  statusText.textContent = text;
  statusLed.title = text;
  ledEl.className = 'led';
  if (state === 'online') ledEl.classList.add('online');
  else if (state === 'error') ledEl.classList.add('error');
}

// ── Timestamp ────────────────────────────────────────

function timestamp() {
  const d = new Date();
  const hh = String(d.getHours()).padStart(2, '0');
  const mm = String(d.getMinutes()).padStart(2, '0');
  const ss = String(d.getSeconds()).padStart(2, '0');
  const ms = String(d.getMilliseconds()).padStart(3, '0');
  return `${hh}:${mm}:${ss}.${ms}`;
}

// ── Log ──────────────────────────────────────────────

function appendLog(dir, text, port) {
  const line = document.createElement('div');
  line.className = 'log-line';

  const timeEl = document.createElement('span');
  timeEl.className = 'log-time';
  timeEl.textContent = timestamp();

  const dirEl = document.createElement('span');
  dirEl.className = `log-dir ${dir}`;
  dirEl.textContent = dir.toUpperCase();

  const portEl = document.createElement('span');
  portEl.className = 'log-port';
  portEl.textContent = port || '';

  const dataEl = document.createElement('span');
  dataEl.className = `log-data ${dir}`;
  dataEl.textContent = text;

  line.appendChild(timeEl);
  line.appendChild(dirEl);
  line.appendChild(portEl);
  line.appendChild(dataEl);

  logEl.appendChild(line);

  if (autoScroll) {
    logEl.scrollTop = logEl.scrollHeight;
  }
}

// ── Data formatting ──────────────────────────────────

function formatReceivedData(msg) {
  if (displayFormat.value === 'hex') {
    return msg.hex || '';
  }
  return msg.ascii || '';
}

// ── API helper ───────────────────────────────────────

async function api(path, method = 'GET', body) {
  const opts = { method };
  if (body) {
    opts.headers = { 'Content-Type': 'application/json' };
    opts.body = JSON.stringify(body);
  }
  const res = await fetch(path, opts);
  return res.json();
}

// ── Port management ──────────────────────────────────

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

// ── WebSocket ────────────────────────────────────────

function connectWs() {
  if (ws) ws.close();

  const proto = location.protocol === 'https:' ? 'wss' : 'ws';
  ws = new WebSocket(`${proto}://${location.host}/`);

  ws.onopen = () => {
    setStatus('WS CONNECTED', 'online');
  };

  ws.onmessage = (event) => {
    try {
      const msg = JSON.parse(event.data);
      if (msg.port && msg.hex) {
        const text = formatReceivedData(msg);
        appendLog('rx', text, msg.port);
      } else {
        appendLog('info', event.data);
      }
    } catch {
      appendLog('info', event.data);
    }
  };

  ws.onclose = () => {
    setStatus('WS DISCONNECTED', 'error');
  };

  ws.onerror = () => {
    setStatus('WS ERROR', 'error');
  };
}

// ── Port open/close ──────────────────────────────────

async function openPort() {
  const port = portsSelect.value;
  if (!port) {
    appendLog('err', '未选择端口');
    return;
  }

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
    setStatus(`OPEN ${port}`, 'online');
    appendLog('info', `端口已打开: ${port}`, port);
  } else {
    appendLog('err', `打开失败: ${resp.msg || '未知错误'}`);
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
    setStatus('PORT CLOSED', '');
    appendLog('info', `端口已关闭: ${port}`, port);
  } else {
    appendLog('err', `关闭失败: ${resp.msg || '未知错误'}`);
  }
}

// ── Send ─────────────────────────────────────────────

async function send() {
  if (!isOpen) {
    appendLog('err', '端口未打开');
    return;
  }

  const data = sendData.value;
  if (!data) return;

  const format = sendFormat.value;
  const resp = await api('/api/port/write', 'POST', {
    port: connectedPort,
    data,
    format,
  });

  if (resp.code === 0) {
    appendLog('tx', data, connectedPort);
  } else {
    appendLog('err', `发送失败: ${resp.msg || '未知错误'}`);
  }
}

// ── Event listeners ──────────────────────────────────

refreshBtn.addEventListener('click', refreshPorts);
openBtn.addEventListener('click', openPort);
closeBtn.addEventListener('click', closePort);
sendBtn.addEventListener('click', send);

sendData.addEventListener('keydown', (e) => {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    send();
  }
});

clearBtn.addEventListener('click', () => {
  logEl.textContent = '';
});

scrollLockBtn.addEventListener('click', () => {
  autoScroll = !autoScroll;
  scrollLockBtn.classList.toggle('active', autoScroll);
});

displayFormat.addEventListener('change', () => {
  // Display format only affects new incoming data
});

// ── Mode tabs ────────────────────────────────────────

const modeTabs = document.querySelectorAll('.mode-tab');
const modbusPanel = document.getElementById('modbusPanel');
const serialEls = document.querySelectorAll('[data-mode="serial"]');

function switchMode(mode) {
  modeTabs.forEach(tab => {
    tab.classList.toggle('active', tab.dataset.mode === mode);
  });

  serialEls.forEach(el => {
    el.style.display = mode === 'serial' ? '' : 'none';
  });

  modbusPanel.style.display = mode === 'modbus' ? '' : 'none';
}

modeTabs.forEach(tab => {
  tab.addEventListener('click', () => switchMode(tab.dataset.mode));
});

// ── Modbus ───────────────────────────────────────────

const modbusSlave = document.getElementById('modbusSlave');
const modbusFunction = document.getElementById('modbusFunction');
const modbusAddress = document.getElementById('modbusAddress');
const modbusQuantity = document.getElementById('modbusQuantity');
const modbusValue = document.getElementById('modbusValue');
const modbusTimeout = document.getElementById('modbusTimeout');
const modbusPoll = document.getElementById('modbusPoll');
const modbusSend = document.getElementById('modbusSend');
const modbusPollBtn = document.getElementById('modbusPollBtn');
const modbusQuantityField = document.getElementById('modbusQuantityField');
const modbusValueField = document.getElementById('modbusValueField');
const modbusTableBody = document.getElementById('modbusTableBody');
const modbusDataFormat = document.getElementById('modbusDataFormat');
const modbusRawTx = document.getElementById('modbusRawTx');
const modbusRawRx = document.getElementById('modbusRawRx');

let pollTimer = null;

function modbusFunctionName(code) {
  const names = {
    1: 'Read Coils',
    2: 'Read Discrete Inputs',
    3: 'Read Holding Registers',
    4: 'Read Input Registers',
    5: 'Write Single Coil',
    6: 'Write Single Register',
    15: 'Write Multiple Coils',
    16: 'Write Multiple Registers',
  };
  return names[code] || `Function ${code}`;
}

function bytesToFloat32(high, low) {
  const buf = new ArrayBuffer(4);
  const view = new DataView(buf);
  view.setUint16(0, high);
  view.setUint16(2, low);
  return view.getFloat32(0);
}

function bytesToInt32(high, low) {
  const buf = new ArrayBuffer(4);
  const view = new DataView(buf);
  view.setUint16(0, high);
  view.setUint16(2, low);
  return view.getInt32(0);
}

function updateModbusFields() {
  const fn = Number(modbusFunction.value);
  const isRead = fn >= 1 && fn <= 4;
  const isWriteSingle = fn === 5 || fn === 6;
  const isWriteMultiple = fn === 15 || fn === 16;

  modbusQuantityField.style.display = (isRead || isWriteMultiple) ? '' : 'none';
  modbusValueField.style.display = (isWriteSingle || isWriteMultiple) ? '' : 'none';

  if (isWriteSingle) {
    modbusValue.placeholder = fn === 5 ? '0 or 1' : '0-65535';
  } else if (isWriteMultiple) {
    modbusValue.placeholder = 'comma separated: 1,2,3,4';
  }

  modbusPollBtn.disabled = !isRead || Number(modbusPoll.value) <= 0;
}

modbusFunction.addEventListener('change', updateModbusFields);

modbusPoll.addEventListener('input', () => {
  const fn = Number(modbusFunction.value);
  const isRead = fn >= 1 && fn <= 4;
  modbusPollBtn.disabled = !isRead || Number(modbusPoll.value) <= 0;
});

function fillModbusTable(values, format) {
  modbusTableBody.innerHTML = '';
  if (!values || !values.length) return;

  const baseAddr = Number(modbusAddress.value) || 0;

  if (format === 'float32' || format === 'int32') {
    const step = 2;
    for (let i = 0; i + 1 < values.length; i += step) {
      const high = values[i];
      const low = values[i + 1];
      const addr = baseAddr + i;

      let dec;
      if (format === 'float32') {
        dec = bytesToFloat32(high, low);
        dec = Number.isFinite(dec) ? dec.toFixed(4) : 'NaN';
      } else {
        dec = bytesToInt32(high, low);
      }

      const tr = document.createElement('tr');
      tr.innerHTML =
        `<td>${addr}-${addr + 1}</td>` +
        `<td>0x${high.toString(16).padStart(4, '0')} ${low.toString(16).padStart(4, '0')}</td>` +
        `<td>${dec}</td>`;
      modbusTableBody.appendChild(tr);
    }
  } else {
    for (let i = 0; i < values.length; i++) {
      const v = values[i];
      const addr = baseAddr + i;

      let hex, dec;
      if (format === 'int16') {
        const signed = v > 0x7FFF ? v - 0x10000 : v;
        hex = '0x' + (v & 0xFFFF).toString(16).padStart(4, '0');
        dec = signed;
      } else if (format === 'hex') {
        hex = '0x' + (v & 0xFFFF).toString(16).padStart(4, '0');
        dec = hex;
      } else {
        hex = '0x' + (v & 0xFFFF).toString(16).padStart(4, '0');
        dec = v;
      }

      const tr = document.createElement('tr');
      tr.innerHTML = `<td>${addr}</td><td>${hex}</td><td>${dec}</td>`;
      modbusTableBody.appendChild(tr);
    }
  }
}

async function modbusRequest() {
  const fn = Number(modbusFunction.value);
  const slave = Number(modbusSlave.value);
  const address = Number(modbusAddress.value);
  const timeout = Number(modbusTimeout.value);

  if (!connectedPort) {
    modbusRawRx.textContent = 'Error: 请先打开串口';
    return;
  }

  let resp;
  try {
    const port = connectedPort;
    if (fn >= 1 && fn <= 4) {
      const quantity = Number(modbusQuantity.value);
      resp = await api('/api/modbus/read', 'POST', {
        port, slave, function: fn, address, quantity, timeout,
      });
    } else if (fn === 5 || fn === 6) {
      const value = Number(modbusValue.value);
      resp = await api('/api/modbus/write', 'POST', {
        port, slave, function: fn, address, value, timeout,
      });
    } else {
      const quantity = Number(modbusQuantity.value);
      const values = modbusValue.value.split(',').map(s => Number(s.trim()));
      resp = await api('/api/modbus/write', 'POST', {
        port, slave, function: fn, address, quantity, values, timeout,
      });
    }
  } catch (e) {
    modbusRawTx.textContent = '-';
    modbusRawRx.textContent = `Error: ${e.message}`;
    return;
  }

  if (resp.code !== 0) {
    modbusRawRx.textContent = `Error: ${resp.msg || 'unknown'}`;
    return;
  }

  const data = resp.data || {};
  modbusRawTx.textContent = data.raw_tx || '-';
  modbusRawRx.textContent = data.raw_rx || '-';

  if (data.values) {
    fillModbusTable(data.values, modbusDataFormat.value);
  }
}

modbusSend.addEventListener('click', modbusRequest);

modbusDataFormat.addEventListener('change', () => {
  const fn = Number(modbusFunction.value);
  if (fn >= 1 && fn <= 4) {
    modbusRequest();
  }
});

function startPoll() {
  const interval = Number(modbusPoll.value);
  if (interval <= 0) return;

  pollTimer = setInterval(modbusRequest, interval);
  modbusPollBtn.classList.add('active');
  modbusPollBtn.textContent = 'STOP';
  modbusSend.disabled = true;
}

function stopPoll() {
  if (pollTimer) {
    clearInterval(pollTimer);
    pollTimer = null;
  }
  modbusPollBtn.classList.remove('active');
  modbusPollBtn.textContent = 'POLL';
  modbusSend.disabled = false;
}

modbusPollBtn.addEventListener('click', () => {
  if (pollTimer) {
    stopPoll();
  } else {
    startPoll();
  }
});

// ── Init ─────────────────────────────────────────────

window.addEventListener('load', async () => {
  await refreshPorts();
  connectWs();
  setStatus('READY', '');
});

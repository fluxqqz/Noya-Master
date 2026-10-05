#pragma once
#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Noya Master</title>
  <style>
    :root {
      color-scheme: light dark;

      --space-1: 4px;
      --space-2: 8px;
      --space-3: 12px;
      --space-4: 16px;
      --space-5: 24px;
      --space-6: 32px;
      --space-7: 48px;

      --font-sans: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      --font-mono: ui-monospace, "SF Mono", Menlo, Consolas, monospace;

      --text-xs: 12px;
      --text-sm: 14px;
      --text-base: 15px;
      --text-lg: 20px;

      --radius-sm: 6px;
      --radius-md: 8px;

      --control-h: 40px;

      --bg: #f9fafb;
      --surface: #ffffff;
      --text: #111827;
      --text-muted: #6b7280;
      --border: #e5e7eb;
      --border-subtle: #f3f4f6;
      --accent: #0f766e;
      --accent-hover: #115e59;
      --accent-contrast: #ffffff;
      --accent-subtle: #f0fdfa;
      --status-ok: #15803d;
      --status-warn: #b45309;
      --status-off: #9ca3af;
      --danger: #b91c1c;
    }

    @media (prefers-color-scheme: dark) {
      :root {
        --bg: #0f1115;
        --surface: #17191e;
        --text: #f3f4f6;
        --text-muted: #9ca3af;
        --border: #262930;
        --border-subtle: #1e2026;
        --accent: #14b8a6;
        --accent-hover: #2dd4bf;
        --accent-contrast: #0f1115;
        --accent-subtle: #132b28;
        --status-ok: #22c55e;
        --status-warn: #f59e0b;
        --status-off: #4b5563;
        --danger: #ef4444;
      }
    }

    /* Touch devices get 44px targets */
    @media (pointer: coarse) {
      :root { --control-h: 44px; }
    }

    *, *::before, *::after {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }

    body {
      font-family: var(--font-sans);
      font-size: var(--text-base);
      line-height: 1.5;
      color: var(--text);
      background-color: var(--bg);
      min-height: 100vh;
      -webkit-font-smoothing: antialiased;
    }

    .container {
      max-width: 980px;
      margin: 0 auto;
      padding: var(--space-5) var(--space-4) var(--space-7);
    }

    /* Header */
    header {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      padding-bottom: var(--space-4);
      border-bottom: 1px solid var(--border);
      margin-bottom: var(--space-6);
      flex-wrap: wrap;
      gap: var(--space-3);
    }

    .brand-title {
      display: inline;
      font-size: var(--text-lg);
      font-weight: 600;
      letter-spacing: -0.01em;
    }

    .brand-sub {
      font-size: var(--text-xs);
      color: var(--text-muted);
      margin-left: var(--space-2);
      font-family: var(--font-mono);
    }

    .uptime {
      font-family: var(--font-mono);
      font-size: var(--text-xs);
      color: var(--text-muted);
      min-width: 7ch;
    }

    .header-meta {
      display: flex;
      align-items: center;
      gap: var(--space-4);
      font-size: var(--text-sm);
    }

    .conn-status {
      display: inline-flex;
      align-items: center;
      gap: var(--space-2);
      color: var(--text-muted);
    }

    .conn-dot {
      width: 6px;
      height: 6px;
      border-radius: 50%;
      background-color: var(--status-off);
    }

    .conn-dot.online {
      background-color: var(--status-ok);
    }

    .link-subtle {
      color: var(--text-muted);
      text-decoration: none;
      transition: color 150ms ease-out;
    }

    .link-subtle:hover {
      color: var(--text);
      text-decoration: underline;
    }

    .link-subtle:focus-visible {
      outline: 2px solid var(--accent);
      outline-offset: 2px;
      border-radius: var(--radius-sm);
    }

    /* Offline banner */
    .banner {
      display: flex;
      align-items: baseline;
      gap: var(--space-2);
      padding: var(--space-3) var(--space-4);
      margin-bottom: var(--space-5);
      background-color: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius-md);
      font-size: var(--text-sm);
    }

    .banner[hidden] {
      display: none;
    }

    .banner strong {
      font-weight: 600;
      color: var(--danger);
    }

    /* Controls group: disabled as one unit while offline */
    fieldset {
      border: 0;
      min-width: 0;
    }

    fieldset:disabled {
      opacity: 0.55;
    }

    fieldset:disabled button:disabled {
      opacity: 1;
    }

    /* Section Structure */
    section {
      margin-bottom: var(--space-6);
    }

    .section-label {
      font-size: var(--text-xs);
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: var(--text-muted);
      margin-bottom: var(--space-3);
    }

    .section-hint {
      font-size: var(--text-xs);
      color: var(--text-muted);
    }

    .section-label + .section-hint {
      margin: calc(-1 * var(--space-2)) 0 var(--space-3);
    }

    .section-hint[hidden] {
      display: none;
    }

    /* Routine View: Focal Point */
    .routine-header {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      gap: var(--space-3);
      margin-bottom: var(--space-2);
    }

    .routine-step-title {
      font-size: var(--text-lg);
      font-weight: 600;
    }

    .routine-badge {
      font-family: var(--font-mono);
      font-size: var(--text-xs);
      font-weight: 600;
      letter-spacing: 0.04em;
      color: var(--text-muted);
    }

    .routine-badge[data-state="RUNNING"] {
      color: var(--status-ok);
    }

    .routine-badge[data-state="PAUSED"] {
      color: var(--status-warn);
    }

    .progress-bar {
      width: 100%;
      height: 3px;
      background-color: var(--border);
      border-radius: var(--radius-sm);
      overflow: hidden;
      margin-bottom: var(--space-4);
    }

    .progress-fill {
      height: 100%;
      width: 0%;
      background-color: var(--accent);
      transition: width 200ms ease-out;
    }

    /* Buttons */
    .button-row {
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      gap: var(--space-3) var(--space-5);
      margin-bottom: var(--space-4);
    }

    .button-group {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: var(--space-2);
    }

    button {
      font-family: var(--font-sans);
      font-size: var(--text-sm);
      font-weight: 600;
      line-height: 1;
      padding: var(--space-3) var(--space-4);
      min-height: var(--control-h);
      border-radius: var(--radius-sm);
      border: 1px solid transparent;
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      transition: background-color 150ms ease-out, border-color 150ms ease-out, color 150ms ease-out;
    }

    button:focus-visible {
      outline: 2px solid var(--accent);
      outline-offset: 2px;
    }

    button:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }

    .btn-primary {
      background-color: var(--accent);
      color: var(--accent-contrast);
    }

    .btn-primary:hover:not(:disabled) {
      background-color: var(--accent-hover);
    }

    .btn-secondary {
      background-color: var(--surface);
      color: var(--text);
      border-color: var(--border);
    }

    .btn-secondary:hover:not(:disabled) {
      border-color: var(--text-muted);
    }

    .btn-danger-ghost {
      background-color: transparent;
      color: var(--danger);
    }

    .btn-danger-ghost:hover:not(:disabled) {
      background-color: var(--border-subtle);
    }

    /* Steps Strip */
    .steps-strip {
      display: flex;
      gap: var(--space-2);
      overflow-x: auto;
      padding-bottom: var(--space-2);
      border-bottom: 1px solid var(--border);
      scrollbar-width: thin;
      scrollbar-color: var(--border) transparent;
    }

    .steps-strip::-webkit-scrollbar {
      height: 6px;
    }

    .steps-strip::-webkit-scrollbar-track {
      background: transparent;
    }

    .steps-strip::-webkit-scrollbar-thumb {
      background: var(--border);
      border-radius: 3px;
    }

    .step-item {
      flex: 0 0 auto;
      flex-direction: column;
      align-items: flex-start;
      justify-content: center;
      gap: var(--space-1);
      min-width: 110px;
      padding: var(--space-2) var(--space-3);
      line-height: 1.4;
      text-align: left;
      background: transparent;
      font-weight: inherit;
    }

    .step-item:hover {
      background-color: var(--border-subtle);
    }

    .step-item:focus-visible {
      outline-offset: -2px;
    }

    .step-item.active {
      background-color: var(--accent-subtle);
      border-color: var(--accent);
    }

    .step-item-num {
      font-family: var(--font-mono);
      font-size: var(--text-xs);
      color: var(--text-muted);
    }

    .step-item.active .step-item-num {
      color: var(--accent);
      font-weight: 600;
    }

    .step-item-desc {
      font-size: var(--text-xs);
      font-weight: 600;
      color: var(--text);
      white-space: nowrap;
    }

    /* Actuators: Flat Proximity Layout */
    .plants-layout {
      display: grid;
      grid-template-columns: 1fr;
      gap: var(--space-5);
    }

    .plant-block {
      border-bottom: 1px solid var(--border);
      padding-bottom: var(--space-5);
    }

    .plant-block:last-child {
      border-bottom: none;
      padding-bottom: 0;
    }

    .plant-header {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      flex-wrap: wrap;
      gap: var(--space-1) var(--space-3);
      margin-bottom: var(--space-3);
    }

    .plant-name {
      font-size: var(--text-base);
      font-weight: 600;
    }

    .plant-meta {
      font-family: var(--font-mono);
      font-size: var(--text-xs);
      color: var(--text-muted);
    }

    .peer-ok {
      color: var(--status-ok);
    }

    .peer-fail {
      color: var(--danger);
    }

    .actuator-row {
      display: grid;
      grid-template-columns: 120px 1fr 1fr 1fr 88px;
      align-items: center;
      gap: var(--space-4);
      padding: var(--space-2) 0;
    }

    .actuator-action button {
      width: 100%;
    }

    @media (max-width: 800px) {
      .actuator-row {
        grid-template-columns: 1fr auto;
        gap: var(--space-2) var(--space-4);
        padding: var(--space-3) 0;
      }

      .actuator-row + .actuator-row {
        border-top: 1px solid var(--border-subtle);
      }

      .actuator-id {
        grid-column: 1;
        grid-row: 1;
      }

      .actuator-action {
        grid-column: 2;
        grid-row: 1;
      }

      .actuator-action button {
        width: auto;
      }

      .actuator-row .field-group {
        grid-column: 1 / -1;
      }
    }

    .actuator-label {
      font-size: var(--text-sm);
      font-weight: 600;
      color: var(--text);
    }

    .actuator-sub {
      font-size: var(--text-xs);
      color: var(--text-muted);
      font-family: var(--font-mono);
    }

    .field-group {
      display: flex;
      flex-direction: column;
      gap: var(--space-1);
      min-width: 0;
    }

    .field-label {
      font-size: var(--text-xs);
      color: var(--text-muted);
    }

    .slider-row {
      display: flex;
      align-items: center;
      gap: var(--space-2);
    }

    .unit {
      min-width: 2ch;
      font-family: var(--font-mono);
      font-size: var(--text-xs);
      color: var(--text-muted);
    }

    /* Range: custom track and a larger thumb for easier grabbing */
    input[type="range"] {
      -webkit-appearance: none;
      appearance: none;
      flex: 1;
      min-width: 0;
      height: var(--control-h);
      margin: 0;
      background: transparent;
      cursor: pointer;
    }

    input[type="range"]::-webkit-slider-runnable-track {
      height: 4px;
      border-radius: 2px;
      background: var(--border);
    }

    input[type="range"]::-moz-range-track {
      height: 4px;
      border-radius: 2px;
      background: var(--border);
    }

    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 18px;
      height: 18px;
      margin-top: -7px;
      border: 0;
      border-radius: 50%;
      background: var(--accent);
    }

    input[type="range"]::-moz-range-thumb {
      width: 18px;
      height: 18px;
      border: 0;
      border-radius: 50%;
      background: var(--accent);
    }

    input[type="range"]:focus-visible {
      outline: none;
    }

    input[type="range"]:focus-visible::-webkit-slider-thumb {
      outline: 2px solid var(--accent);
      outline-offset: 2px;
    }

    input[type="range"]:focus-visible::-moz-range-thumb {
      outline: 2px solid var(--accent);
      outline-offset: 2px;
    }

    input[type="number"] {
      width: 64px;
      min-height: var(--control-h);
      font-family: var(--font-mono);
      font-size: var(--text-sm);
      color: var(--text);
      background-color: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius-sm);
      padding: var(--space-1) var(--space-2);
      text-align: right;
    }

    input[type="number"]:focus-visible {
      outline: 2px solid var(--accent);
      outline-offset: 1px;
    }

    input:disabled {
      cursor: not-allowed;
    }

    /* Activity Table */
    .table-wrap {
      overflow-x: auto;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      font-size: var(--text-sm);
    }

    th {
      text-align: left;
      font-size: var(--text-xs);
      font-weight: 600;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.05em;
      padding: var(--space-2) var(--space-5) var(--space-2) 0;
      border-bottom: 1px solid var(--border);
      white-space: nowrap;
    }

    td {
      padding: var(--space-3) var(--space-5) var(--space-3) 0;
      border-bottom: 1px solid var(--border-subtle);
      font-family: var(--font-mono);
      font-size: var(--text-xs);
      white-space: nowrap;
    }

    th:last-child,
    td:last-child {
      padding-right: 0;
      text-align: right;
    }

    td.empty {
      padding: var(--space-4) 0;
      text-align: left;
      white-space: normal;
      font-family: var(--font-sans);
      font-size: var(--text-sm);
      color: var(--text-muted);
    }

    .status-badge {
      font-weight: 600;
    }

    .status-badge.ok {
      color: var(--status-ok);
    }

    .status-badge.fail {
      color: var(--danger);
    }

    /* Toast for failed commands */
    .toast {
      position: fixed;
      left: 50%;
      bottom: var(--space-5);
      transform: translateX(-50%);
      max-width: calc(100% - 2 * var(--space-4));
      padding: var(--space-3) var(--space-4);
      background-color: var(--text);
      color: var(--bg);
      border-radius: var(--radius-md);
      font-size: var(--text-sm);
      opacity: 0;
      pointer-events: none;
      transition: opacity 150ms ease-out;
    }

    .toast.show {
      opacity: 1;
    }

    @media (prefers-reduced-motion: reduce) {
      *, *::before, *::after {
        animation-duration: 0.01ms !important;
        transition-duration: 0.01ms !important;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div>
        <h1 class="brand-title">Noya Master</h1>
        <span id="channel-label" class="brand-sub">Channel 6</span>
      </div>
      <div class="header-meta">
        <span id="uptime" class="uptime"></span>
        <div class="conn-status">
          <span id="conn-dot" class="conn-dot"></span>
          <span id="conn-text">Connecting</span>
        </div>
        <a href="/update" class="link-subtle" target="_blank" rel="noopener">OTA Update &nearr;</a>
      </div>
    </header>

    <div id="offline-banner" class="banner" role="status" hidden>
      <strong>Master unreachable.</strong>
      <span>Controls are disabled until it reconnects.</span>
    </div>

    <fieldset id="controls" disabled>
      <!-- Focal Point: Show Routine -->
      <section>
        <h2 class="section-label">Show Routine</h2>
        <div class="routine-header">
          <div id="routine-title" class="routine-step-title">Connecting&hellip;</div>
          <div id="routine-state" class="routine-badge">&mdash;</div>
        </div>
        <div id="progress-bar" class="progress-bar" role="progressbar" aria-label="Show progress" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0">
          <div id="progress-fill" class="progress-fill"></div>
        </div>

        <div class="button-row">
          <div class="button-group">
            <button type="button" id="btn-main" class="btn-primary" onclick="togglePlay()">Start Show</button>
            <button type="button" id="btn-prev" class="btn-secondary" onclick="routineAction('prev')">Previous</button>
            <button type="button" id="btn-next" class="btn-secondary" onclick="routineAction('next')">Next</button>
            <button type="button" id="btn-stop" class="btn-danger-ghost" onclick="routineAction('stop')">Stop</button>
          </div>
          <div class="button-group">
            <button type="button" class="btn-secondary" onclick="quickAction('all_talk')">Chorus All</button>
            <button type="button" class="btn-secondary" onclick="quickAction('rest_all')">Rest All</button>
          </div>
        </div>

        <div class="steps-strip" id="steps-strip">
          <!-- Rendered via JS -->
        </div>
      </section>

      <!-- Manual Actuators -->
      <section>
        <h2 class="section-label">Manual Actuation</h2>
        <p id="manual-hint" class="section-hint" hidden>Testing is locked while the show runs &mdash; pause or stop the routine to test mouths manually.</p>
        <div class="plants-layout" id="plants-layout">
          <!-- Rendered via JS -->
        </div>
      </section>
    </fieldset>

    <!-- Activity Log -->
    <section>
      <h2 class="section-label">Recent Activity</h2>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th scope="col">Time</th>
              <th scope="col">Cmd</th>
              <th scope="col">Target</th>
              <th scope="col">Mouth</th>
              <th scope="col">Angles</th>
              <th scope="col">Duration</th>
              <th scope="col">Status</th>
            </tr>
          </thead>
          <tbody id="history-tbody">
            <tr>
              <td colspan="7" class="empty">No transmissions yet.</td>
            </tr>
          </tbody>
        </table>
      </div>
    </section>
  </div>

  <div id="toast" class="toast" role="status" aria-live="polite"></div>

  <script>
    const PLANTS = [
      { id: 1, name: "Plant 1", mac: "02:02:00:00:00:02" },
      { id: 2, name: "Plant 2", mac: "02:02:00:00:00:03" },
      { id: 3, name: "Plant 3", mac: "02:02:00:00:00:04" },
      { id: 4, name: "Plant 4", mac: "02:02:00:00:00:05" }
    ];

    const MOUTHS = [
      { label: "Mouth 1", gpio: "GPIO 5" },
      { label: "Mouth 2", gpio: "GPIO 1" }
    ];

    let currentState = 'STOPPED';
    let failCount = 0;
    let stepsSig = '';
    let activeStep = null;
    let toastTimer = null;

    function actuatorRow(p, pIdx, m, mIdx) {
      const mouthName = `${p.name} ${m.label.toLowerCase()}`;
      return `
        <div class="actuator-row">
          <div class="actuator-id">
            <div class="actuator-label">${m.label}</div>
            <div class="actuator-sub">${m.gpio}</div>
          </div>

          <div class="field-group">
            <label class="field-label" for="open_num_${pIdx}_${mIdx}">Open (Rest)</label>
            <div class="slider-row">
              <input type="range" id="open_${pIdx}_${mIdx}" min="0" max="180" value="30" aria-label="Open angle slider, ${mouthName}" oninput="sync(${pIdx}, ${mIdx}, 'open', this.value)">
              <input type="number" id="open_num_${pIdx}_${mIdx}" min="0" max="180" value="30" inputmode="numeric" aria-label="Open angle in degrees, ${mouthName}" oninput="sync(${pIdx}, ${mIdx}, 'open', this.value)" onchange="clampNum(this)">
              <span class="unit" aria-hidden="true">&deg;</span>
            </div>
          </div>

          <div class="field-group">
            <label class="field-label" for="close_num_${pIdx}_${mIdx}">Close</label>
            <div class="slider-row">
              <input type="range" id="close_${pIdx}_${mIdx}" min="0" max="180" value="85" aria-label="Close angle slider, ${mouthName}" oninput="sync(${pIdx}, ${mIdx}, 'close', this.value)">
              <input type="number" id="close_num_${pIdx}_${mIdx}" min="0" max="180" value="85" inputmode="numeric" aria-label="Close angle in degrees, ${mouthName}" oninput="sync(${pIdx}, ${mIdx}, 'close', this.value)" onchange="clampNum(this)">
              <span class="unit" aria-hidden="true">&deg;</span>
            </div>
          </div>

          <div class="field-group">
            <label class="field-label" for="dur_num_${pIdx}_${mIdx}">Duration</label>
            <div class="slider-row">
              <input type="range" id="dur_${pIdx}_${mIdx}" min="500" max="15000" step="100" value="3000" aria-label="Duration slider, ${mouthName}" oninput="sync(${pIdx}, ${mIdx}, 'dur', this.value)">
              <input type="number" id="dur_num_${pIdx}_${mIdx}" min="500" max="15000" step="100" value="3000" inputmode="numeric" aria-label="Duration in milliseconds, ${mouthName}" oninput="sync(${pIdx}, ${mIdx}, 'dur', this.value)" onchange="clampNum(this)">
              <span class="unit" aria-hidden="true">ms</span>
            </div>
          </div>

          <div class="actuator-action">
            <button type="button" class="btn-secondary" aria-label="Test ${mouthName}" onclick="animateMouth(${pIdx}, ${mIdx})">Test</button>
          </div>
        </div>`;
    }

    function buildActuators() {
      const container = document.getElementById('plants-layout');
      container.innerHTML = PLANTS.map((p, pIdx) => `
        <div class="plant-block">
          <div class="plant-header">
            <h3 class="plant-name">${p.name}</h3>
            <span class="plant-meta">${p.mac} &middot; <span id="peer-status-${pIdx}">Ready</span></span>
          </div>
          ${MOUTHS.map((m, mIdx) => actuatorRow(p, pIdx, m, mIdx)).join('')}
        </div>
      `).join('');
    }

    function sync(p, m, type, val) {
      document.getElementById(`${type}_${p}_${m}`).value = val;
      document.getElementById(`${type}_num_${p}_${m}`).value = val;
    }

    // On blur, snap the typed value into range and mirror it to the slider,
    // so what's displayed is always what a Test would send.
    function clampNum(el) {
      const min = Number(el.min), max = Number(el.max);
      const v = Number(el.value);
      el.value = Math.min(max, Math.max(min, isNaN(v) ? min : v));
      const slider = document.getElementById(el.id.replace('_num_', '_'));
      if (slider) slider.value = el.value;
    }

    function inputVal(id) {
      const el = document.getElementById(id);
      const min = Number(el.min), max = Number(el.max);
      const v = Number(el.value);
      return Math.round(Math.min(max, Math.max(min, isNaN(v) ? min : v)));
    }

    function fmtDur(s) {
      if (s < 60) return s + 's';
      if (s < 3600) return Math.floor(s / 60) + 'm';
      const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60);
      return m ? h + 'h ' + m + 'm' : h + 'h';
    }

    function notify(msg) {
      const el = document.getElementById('toast');
      el.textContent = msg;
      el.classList.add('show');
      clearTimeout(toastTimer);
      toastTimer = setTimeout(() => el.classList.remove('show'), 3500);
    }

    function setLink(online) {
      document.getElementById('conn-dot').className = online ? 'conn-dot online' : 'conn-dot';
      document.getElementById('conn-text').textContent = online ? 'Connected' : 'Offline';
      document.getElementById('controls').disabled = !online;
      document.getElementById('offline-banner').hidden = online;
    }

    function togglePlay() {
      if (currentState === 'RUNNING') {
        routineAction('pause');
      } else if (currentState === 'PAUSED') {
        routineAction('resume');
      } else {
        routineAction('start');
      }
    }

    async function routineAction(action, step) {
      try {
        let body = `action=${action}`;
        if (step !== undefined) body += `&step=${step}`;
        const res = await fetch('/api/routine', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body
        });
        if (!res.ok) throw new Error(res.status);
        updateStatus();
      } catch (e) {
        console.error("Action error", e);
        notify("Command not sent. The master didn't respond.");
      }
    }

    async function quickAction(action) {
      try {
        const res = await fetch('/api/quick', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: `action=${action}`
        });
        if (!res.ok) throw new Error(res.status);
        updateStatus();
      } catch (e) {
        console.error("Quick error", e);
        notify("Command not sent. The master didn't respond.");
      }
    }

    async function animateMouth(slave, mouth) {
      // Read the number fields (what the user sees) and clamp, so sent == shown
      const open = inputVal(`open_num_${slave}_${mouth}`);
      const close = inputVal(`close_num_${slave}_${mouth}`);
      const duration = inputVal(`dur_num_${slave}_${mouth}`);

      try {
        const res = await fetch('/api/animate', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: `slave=${slave}&mouth=${mouth}&open=${open}&close=${close}&duration=${duration}`
        });
        if (!res.ok) throw new Error(res.status);
        saveManual();
        updateStatus();
      } catch (e) {
        console.error("Animate error", e);
        notify("Command not sent. The master didn't respond.");
      }
    }

    // Rebuild the strip only when the step list changes; otherwise just move the highlight
    function renderSteps(steps, currentStep, state) {
      const strip = document.getElementById('steps-strip');
      if (!steps || steps.length === 0) return;

      // Merge runs of identical steps (e.g. the chorus fan-out) into one tile
      const groups = [];
      for (const s of steps) {
        const last = groups[groups.length - 1];
        if (last && last.desc === s.desc) {
          last.end = s.idx;
          last.dur += s.dur;
        } else {
          groups.push({ start: s.idx, end: s.idx, dur: s.dur, desc: s.desc });
        }
      }

      const sig = groups.map(g => `${g.start}-${g.end}:${g.dur}:${g.desc}`).join('|');
      if (sig !== stepsSig) {
        stepsSig = sig;
        activeStep = undefined;
        strip.innerHTML = groups.map(g => {
          const num = g.start === g.end
            ? String(g.start).padStart(2, '0')
            : `${String(g.start).padStart(2, '0')}&ndash;${String(g.end).padStart(2, '0')}`;
          const dur = g.dur > 0 ? (g.dur / 1000).toFixed(1) + 's' : 'Rest';
          return `
            <button type="button" class="step-item" data-start="${g.start}" data-end="${g.end}" onclick="routineAction('jump', ${g.start})" title="Jump to step ${g.start}">
              <span class="step-item-num">${num} &middot; ${dur}</span>
              <span class="step-item-desc">${g.desc}</span>
            </button>`;
        }).join('');
      }

      const running = (state === 'RUNNING' || state === 'PAUSED');
      const nextActive = running ? currentStep : null;
      if (nextActive === activeStep) return;
      activeStep = nextActive;

      const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
      strip.querySelectorAll('.step-item').forEach(el => {
        const start = Number(el.dataset.start);
        const end = Number(el.dataset.end);
        const on = nextActive !== null && nextActive >= start && nextActive <= end;
        el.classList.toggle('active', on);
        // Jumping implies starting the show; only offer it while a routine is live
        el.disabled = !running;
        if (on) {
          el.setAttribute('aria-current', 'step');
          strip.scrollTo({
            left: el.offsetLeft - strip.offsetLeft - (strip.clientWidth - el.offsetWidth) / 2,
            behavior: reduceMotion ? 'auto' : 'smooth'
          });
        } else {
          el.removeAttribute('aria-current');
        }
      });
    }

    async function updateStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) throw new Error("Status error");
        const data = await res.json();

        // Header connectivity
        failCount = 0;
        setLink(true);

        if (data.uptime_sec !== undefined) {
          document.getElementById('uptime').textContent = 'up ' + fmtDur(data.uptime_sec);
        }
        if (data.channel !== undefined) {
          document.getElementById('channel-label').textContent = 'Channel ' + data.channel;
        }

        // Routine
        currentState = data.routine.state;
        const currentStep = data.routine.current_step;
        const totalSteps = data.routine.total_steps;

        const stateEl = document.getElementById('routine-state');
        stateEl.textContent = currentState;
        stateEl.dataset.state = currentState;

        const mainBtn = document.getElementById('btn-main');
        if (currentState === 'RUNNING') {
          mainBtn.textContent = 'Pause';
          mainBtn.className = 'btn-secondary';
        } else if (currentState === 'PAUSED') {
          mainBtn.textContent = 'Resume';
          mainBtn.className = 'btn-primary';
        } else {
          mainBtn.textContent = 'Start Show';
          mainBtn.className = 'btn-primary';
        }

        document.getElementById('btn-stop').disabled = (currentState === 'STOPPED');
        document.getElementById('btn-prev').disabled = (currentState === 'STOPPED');
        document.getElementById('btn-next').disabled = (currentState === 'STOPPED');

        // Manual tests would fight the running choreography for the servos
        const manualLocked = (currentState === 'RUNNING');
        document.querySelectorAll('.actuator-action button').forEach(b => { b.disabled = manualLocked; });
        document.getElementById('manual-hint').hidden = !manualLocked;

        let pct = 0;
        if (currentState === 'STOPPED') {
          document.getElementById('routine-title').textContent = 'Routine stopped';
        } else {
          document.getElementById('routine-title').textContent = `Step ${currentStep} of ${totalSteps}: ${data.routine.step_desc}`;
          pct = Math.round((currentStep / totalSteps) * 100);
        }
        document.getElementById('progress-fill').style.width = `${pct}%`;
        document.getElementById('progress-bar').setAttribute('aria-valuenow', pct);

        renderSteps(data.steps, currentStep, currentState);

        // Peers
        if (data.peers) {
          data.peers.forEach((p, idx) => {
            const el = document.getElementById(`peer-status-${idx}`);
            if (!el) return;
            el.classList.remove('peer-ok', 'peer-fail');
            if (p.sent && !p.ack) {
              el.textContent = 'No response';
              el.classList.add('peer-fail');
            } else if (p.ack) {
              el.textContent = 'Delivered';
              el.classList.add('peer-ok');
            } else {
              el.textContent = 'Ready';
            }
          });
        }

        // History
        const tbody = document.getElementById('history-tbody');
        if (data.history && data.history.length > 0) {
          tbody.innerHTML = data.history.map(item => `
            <tr>
              <td>${fmtDur(item.time_ago)} ago</td>
              <td>#${item.cmd_id}</td>
              <td>Plant ${item.slave_idx + 1}</td>
              <td>Mouth ${item.servo_idx + 1}</td>
              <td>${item.open_deg}&deg; &rarr; ${item.close_deg}&deg;</td>
              <td>${item.duration_ms > 0 ? item.duration_ms + 'ms' : '&mdash;'}</td>
              <td><span class="status-badge ${item.ack ? 'ok' : 'fail'}">${item.ack ? 'Delivered' : 'Failed'}</span></td>
            </tr>
          `).join('');
        }
      } catch (e) {
        // Two misses in a row before flagging offline, so one slow reply doesn't flicker the UI
        failCount++;
        if (failCount >= 2) setLink(false);
      }
    }

    // Remember the last-sent manual values across reloads (tuning sessions)
    const MANUAL_KEY = 'noya-manual-v1';

    function saveManual() {
      const vals = {};
      document.querySelectorAll('#plants-layout input[type="number"]').forEach(el => { vals[el.id] = el.value; });
      try { localStorage.setItem(MANUAL_KEY, JSON.stringify(vals)); } catch (e) { /* private mode */ }
    }

    function loadManual() {
      let vals = null;
      try { vals = JSON.parse(localStorage.getItem(MANUAL_KEY) || 'null'); } catch (e) { /* corrupt entry */ }
      if (!vals) return;
      Object.keys(vals).forEach(id => {
        const num = document.getElementById(id);
        if (!num) return;
        num.value = vals[id];
        const slider = document.getElementById(id.replace('_num_', '_'));
        if (slider) slider.value = vals[id];
      });
    }

    // Initialize
    buildActuators();
    loadManual();
    updateStatus();
    setInterval(updateStatus, 800);
  </script>
</body>
</html>
)rawliteral";
// ===== Mini Linux Web Shell - script.js =====
// Developed by: Bhaumik Negi, Divyanshi Kaushik, Harshita Pant, Ansh Karki
// University: Graphic Era Hill University
// Project: Operating Systems PBL - Mini Linux Shell (Web Version)

const commandInput = document.getElementById("commandInput");
const output = document.getElementById("output");
const clearBtn = document.getElementById("clearBtn");
const downloadBtn = document.getElementById("downloadBtn");
const themeToggle = document.getElementById("themeToggle");

let commandHistory = JSON.parse(localStorage.getItem("commandHistory") || "[]");
let historyIndex = commandHistory.length;
let logBuffer = "";

// ===== EVENT LISTENERS =====
commandInput.addEventListener("keydown", async (e) => {
  if (e.key === "Enter") {
    const command = commandInput.value.trim();
    if (command) {
      commandHistory.push(command);
      localStorage.setItem("commandHistory", JSON.stringify(commandHistory));
      historyIndex = commandHistory.length;

      displayCommand(command);
      await executeCommand(command);
      commandInput.value = "";
    }
  } else if (e.key === "ArrowUp") {
    e.preventDefault();
    if (historyIndex > 0) {
      historyIndex--;
      commandInput.value = commandHistory[historyIndex];
    }
  } else if (e.key === "ArrowDown") {
    e.preventDefault();
    if (historyIndex < commandHistory.length - 1) {
      historyIndex++;
      commandInput.value = commandHistory[historyIndex];
    } else {
      historyIndex = commandHistory.length;
      commandInput.value = "";
    }
  } else if (e.key === "l" && e.ctrlKey) {
    e.preventDefault();
    clearTerminal();
  }
});

// ===== DISPLAY COMMANDS & OUTPUT =====
function displayCommand(command) {
  const commandLine = document.createElement("div");
  commandLine.className = "command-line";
  commandLine.innerHTML = `<span class="prompt">$ </span><span class="command">${escapeHtml(
    command
  )}</span>`;
  output.appendChild(commandLine);
  logBuffer += `$ ${command}\n`;
  scrollToBottom();
}

function displayOutput(text, isError = false) {
  const outputLine = document.createElement("div");
  outputLine.className = isError ? "output error" : "output";
  outputLine.textContent = text;
  output.appendChild(outputLine);
  logBuffer += text + "\n";
  scrollToBottom();
}

// ===== COMMAND EXECUTION =====
async function executeCommand(command) {
  // Local-only commands (not sent to backend)
  if (command === "clear" || command === "cls") {
    clearTerminal();
    return;
  }

  // Send command to backend
  try {
    const response = await fetch("/execute", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `command=${encodeURIComponent(command)}`,
    });

    if (!response.ok) {
      displayOutput("Error: Failed to execute command", true);
      return;
    }

    const data = await response.json();
    if (data.output) displayOutput(data.output.trim());
  } catch (error) {
    displayOutput(`Error: ${error.message}`, true);
  }
}

// ===== UTILITIES =====
function clearTerminal() {
  output.innerHTML =
    "<div class=\"welcome\">🧹 Terminal cleared. Type 'help' for available commands.</div>";
  logBuffer = "";
}

function scrollToBottom() {
  output.scrollTop = output.scrollHeight;
}

function escapeHtml(text) {
  const map = {
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#039;",
  };
  return text.replace(/[&<>"']/g, (m) => map[m]);
}

// ===== BUTTON ACTIONS =====
clearBtn.addEventListener("click", clearTerminal);

downloadBtn.addEventListener("click", () => {
  const blob = new Blob([logBuffer], { type: "text/plain" });
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = "webshell_log.txt";
  a.click();
});

themeToggle.addEventListener("click", () => {
  document.body.classList.toggle("light");
  const isLight = document.body.classList.contains("light");
  themeToggle.textContent = isLight ? "🌞" : "🌙";
});

document.addEventListener("click", () => commandInput.focus());
commandInput.focus();

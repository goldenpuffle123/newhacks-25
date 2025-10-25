const optionsButton = document.getElementById('options-button');
const lockInButton = document.getElementById('lock-in-button');
let lockInButtonState = true;
optionsButton.addEventListener('click', () => {
    chrome.runtime.openOptionsPage();
});

lockInButton.addEventListener('click', () => {

    lockInButtonState = !lockInButtonState;
    if (lockInButtonState) {
        // Locked in state
        lockInButton.textContent = 'Unlock';
        lockInButton.style.backgroundColor = '#ff6b6b'; // reddish
    } else {
        lockInButton.textContent = 'Lock in';
        lockInButton.style.backgroundColor = '#4ecdc4'; // greenish
    }
    chrome.storage.local.set({ lockIn: lockInButtonState });
    console.log("Lock in button state:", lockInButtonState);
    sendCommand("led_blink");
});

// Load state from chrome.storage when popup opens
chrome.storage.local.get(['lockIn'], (result) => {
    if (typeof result.lockIn === 'boolean') {
        lockInButtonState = result.lockIn;
        if (lockInButtonState) {
            lockInButton.textContent = 'Unlock';
            lockInButton.style.backgroundColor = '#ff6b6b';
        } else {
            lockInButton.textContent = 'Lock in';
            lockInButton.style.backgroundColor = '#4ecdc4';
            // Perform actions when unlocking (if any)
        }
    } else {
        // Default state if not set
        lockInButton.textContent = 'Lock in';
        lockInButton.style.backgroundColor = '#ff6b6b';
    }
});

//ESP32 test

const ESP_IP = "http://10.0.0.20";

async function sendCommand(cmd) {
  const payload = { cmd }; // or include args, timestamp, seq
  const res = await fetch(`${ESP_IP}/command`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  if (!res.ok) throw new Error("HTTP error " + res.status);
  const json = await res.json();
  console.log("ESP response:", json);
}
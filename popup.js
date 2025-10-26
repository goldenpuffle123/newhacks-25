const optionsButton = document.getElementById('options-button');
const lockInButton = document.getElementById('lock-in-button');
const timerDisplay = document.getElementById('timer-display');
let timerInterval = null;

const timerInput = document.getElementById('timer-input');

optionsButton.addEventListener('click', () => {
    chrome.runtime.openOptionsPage();
});

lockInButton.addEventListener('click', () => {
    chrome.storage.local.get(['lockIn'], (result) => {
        const isLocked = result.lockIn || false;
        const newState = !isLocked;

        if (newState) {
            // Lock in - red theme
            lockInButton.textContent = 'Locked in';
            lockInButton.classList.add('locked');
            chrome.runtime.sendMessage({ type: 'ESP_COMMAND', cmd: "cmd_locked" });
            // Get minutes from input, default to 60 if invalid
            // Allow decimal values (e.g., 0.5 minutes = 30 seconds)
            let minutes = parseFloat(timerInput.value);
            if (isNaN(minutes) || minutes <= 0) minutes = 60;
            startTimer(Math.round(minutes * 60));
        } else {
            // Unlock - green theme
            lockInButton.textContent = 'Unlocked';
            lockInButton.classList.remove('locked');
            chrome.runtime.sendMessage({ type: 'ESP_COMMAND', cmd: "cmd_unlocked" });
            stopTimer();
        }

        chrome.storage.local.set({ lockIn: newState });
    });
});

function startTimer(seconds) {
    // Start timer in background
    chrome.runtime.sendMessage({ type: 'START_TIMER', duration: seconds });

    // Immediately update display to avoid showing 0:00
    updateTimerDisplay(seconds);

    // Clear any existing interval
    if (timerInterval) {
        clearTimeout(timerInterval);
        timerInterval = null;
    }

    function pollTimer() {
        chrome.runtime.sendMessage({ type: 'GET_TIMER' }, (response) => {
            if (response && response.active) {
                updateTimerDisplay(response.remaining);
                if (response.remaining <= 0) {
                    // Timer expired - unlock
                    lockInButton.textContent = 'Unlocked';
                    lockInButton.classList.remove('locked');
                    chrome.runtime.sendMessage({ type: 'ESP_COMMAND', cmd: "cmd_unlocked" });
                    chrome.storage.local.set({ lockIn: false });
                    stopTimer();
                } else {
                    // Use setTimeout for more accurate polling
                    timerInterval = setTimeout(pollTimer, 1000);
                }
            }
        });
    }
    pollTimer();
}

function stopTimer() {
    if (timerInterval) {
        clearTimeout(timerInterval);
        timerInterval = null;
    }
    chrome.runtime.sendMessage({ type: 'STOP_TIMER' });
    updateTimerDisplay(0);
}

function updateTimerDisplay(seconds) {
    if (timerDisplay) {
        const minutes = Math.floor(seconds / 60);
        const secs = seconds % 60;
        timerDisplay.textContent = `${minutes}:${secs.toString().padStart(2, '0')}`;
    }
}

// Initialize on load
chrome.storage.local.get(['lockIn'], (result) => {
    const isLocked = result.lockIn || false;
    
    if (isLocked) {
        lockInButton.textContent = 'Locked in';
        lockInButton.classList.add('locked');
        
        // Check if timer is still running
        chrome.runtime.sendMessage({ type: 'GET_TIMER' }, (response) => {
            if (response && response.active && response.remaining > 0) {
                // Immediately show the remaining time before starting interval
                updateTimerDisplay(response.remaining);
                startTimer(response.remaining);
            } else {
                // Timer expired while popup was closed
                lockInButton.textContent = 'Unlocked';
                lockInButton.classList.remove('locked');
                chrome.storage.local.set({ lockIn: false });
                updateTimerDisplay(0);
            }
        });
    } else {
        lockInButton.textContent = 'Unlocked';
        lockInButton.classList.remove('locked');
        updateTimerDisplay(0);
    }
});
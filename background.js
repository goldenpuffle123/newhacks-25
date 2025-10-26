// ESP32 communication handler for popup.js requests
const ESP_IP = "http://10.0.0.20";

async function sendCommand(cmd) {
    console.log(`[SEND] Sending command to ESP32: ${cmd}`);
    try {
        const payload = { cmd };
        const res = await fetch(`${ESP_IP}/command`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload),
        });
        if (!res.ok) throw new Error("HTTP error " + res.status);
        const json = await res.json();
        console.log(`[SUCCESS] Response to ${cmd}:`, json);
        return json;
    } catch (error) {
        console.error(`[ERROR] Error sending ${cmd}:`, error.message);
        return { error: error.message };
    }
}

let currentLinkId = null;
let browsingStartTime = null;
let timerStartTime = null;
let timerDuration = 0;

// Listen for messages from popup
chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
    console.log(`[MESSAGE] Received:`, request.type, request.cmd || '');
    
    if (request.type === 'ESP_COMMAND') {
        console.log(`[ESP] Processing command: ${request.cmd}`);
        // Properly await the command
        sendCommand(request.cmd).then(result => {
            console.log(`[ESP] Command completed:`, result);
        }).catch(err => {
            console.error(`[ESP] Command failed:`, err);
        });
        sendResponse({ success: true });
        return true;
    }
    else if (request.type === 'START_TIMER') {
        timerStartTime = Date.now();
        timerDuration = request.duration;
        console.log(`[TIMER] Started: ${timerDuration} seconds`);
        sendResponse({ success: true });
        return true;
    } 
    else if (request.type === 'STOP_TIMER') {
        timerStartTime = null;
        timerDuration = 0;
        console.log('[TIMER] Stopped');
        sendResponse({ success: true });
        return true;
    } 
    else if (request.type === 'GET_TIMER') {
        if (timerStartTime) {
            const elapsedSeconds = Math.floor((Date.now() - timerStartTime) / 1000);
            const remainingSeconds = Math.max(0, timerDuration - elapsedSeconds);
            console.log(`[TIMER] Check: ${remainingSeconds}s remaining`);
            sendResponse({ active: true, remaining: remainingSeconds });
        } else {
            sendResponse({ active: false, remaining: 0 });
        }
        return true;
    }
});

// Listen for tab updates to check if user is browsing a saved link
chrome.tabs.onUpdated.addListener((tabId, changeInfo, tab) => {
    if (changeInfo.status === 'complete') {
		checkIfBrowsingLink(tab.url);
    }
});

// Listen for active tab changes
chrome.tabs.onActivated.addListener((activeInfo) => {
    chrome.tabs.get(activeInfo.tabId, (tab) => {
		checkIfBrowsingLink(tab.url);
    });
});

// Listen for tab removal to save time spent
chrome.tabs.onRemoved.addListener((tabId) => {
	saveBrowsingTime();
});

// Check if current URL matches any saved link
function checkIfBrowsingLink(currentUrl) {
    if (!currentUrl) return;

    chrome.storage.sync.get(['links'], (result) => {
        const links = result.links || {};

        // Find which saved link (if any) matches the current URL
        let matchedLinkId = null;
        for (const [id, link] of Object.entries(links)) {
            const savedLinkUrl = typeof link === 'object' ? link.url : link;
            try {
                const currentUrlObj = new URL(currentUrl);
                const savedUrlObj = new URL(savedLinkUrl);
                
                if (currentUrlObj.hostname !== savedUrlObj.hostname) continue;
                
                let currentPath = currentUrlObj.pathname;
                let savedPath = savedUrlObj.pathname.replace(/\/$/, '');
                
                // Match if saved path is root, or current path starts with saved path
                if (savedPath === '' || currentPath === savedPath || currentPath.startsWith(savedPath + '/')) {
                    matchedLinkId = id;
                    // SEND SIGNAL!!
                    sendCommand("cmd_link_detected");
                    break;
                }
            } catch {
                continue;
            }
        }

        console.log(`URL: ${currentUrl}, Matched: ${matchedLinkId}, Current: ${currentLinkId}`);

        // Switching to a different saved link - save time for previous link
        if (currentLinkId && matchedLinkId !== currentLinkId) {
            console.log(`Switching from ${currentLinkId} to ${matchedLinkId}`);
            saveBrowsingTime();
        }

        // Update current link and start time
        if (matchedLinkId) {
            if (currentLinkId !== matchedLinkId) {
                browsingStartTime = Date.now();
                console.log(`Started tracking link ${matchedLinkId}`);
            }
            currentLinkId = matchedLinkId;
        } else {
            // Left a saved link
            if (currentLinkId) {
                console.log(`Left saved link ${currentLinkId}`);
                saveBrowsingTime();
            }
            currentLinkId = null;
        }

        // Store status for popup
        chrome.storage.local.set({ 
            isBrowsingLink: matchedLinkId !== null, 
            currentUrl 
        });
    });
}

// Save browsing time for the current link
function saveBrowsingTime() {
    if (!currentLinkId || !browsingStartTime) return;

    const timeSpentInSeconds = Math.floor((Date.now() - browsingStartTime) / 1000);
    const linkId = currentLinkId;
    
    // Reset tracking
    currentLinkId = null;
    browsingStartTime = null;

    chrome.storage.sync.get(['links'], (result) => {
        const links = result.links || {};
        
        if (links[linkId]) {
            // Update timeSpent in the link object
            links[linkId].timeSpent = (links[linkId].timeSpent || 0) + timeSpentInSeconds;
            
            chrome.storage.sync.set({ links }, () => {
                console.log(`Saved ${timeSpentInSeconds}s for link ${linkId}. Total: ${links[linkId].timeSpent}s`);
            });
        }
    });
}

// Normalize URLs for better comparison
function normalizeUrl(url) {
    try {
        const urlObj = new URL(url);
        // Remove protocol and trailing slash for comparison
        return urlObj.hostname + urlObj.pathname.replace(/\/$/, '');
    } catch {
        return url;
    }
}
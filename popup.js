const optionsButton = document.getElementById('options-button');

optionsButton.addEventListener('click', () => {
    chrome.runtime.openOptionsPage();
});
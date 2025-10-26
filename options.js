const linksContainer = document.getElementById('links-container');
const addLinkBtn = document.getElementById('add-link-btn');
const saveLinksBtn = document.getElementById('save-links-btn');

// Load links from Chrome Storage on page load
document.addEventListener('DOMContentLoaded', () => {
    loadLinks();
});

// Add new link field
addLinkBtn.addEventListener('click', () => {
    addLinkField();
});

// Save links to Chrome Storage
saveLinksBtn.addEventListener('click', () => {
    saveLinks();
});

// Load links from storage and render them
function loadLinks() {
    chrome.storage.sync.get(['links'], (result) => {
        const links = result.links || {};
        linksContainer.innerHTML = '';
        
        // Hide container if no links
        if (Object.keys(links).length === 0) {
            linksContainer.style.display = 'none';
        } else {
            linksContainer.style.display = 'block';
        }
        
        // Display each link
        Object.keys(links).forEach((key) => {
            renderLinkField(key, links[key]);
        });
    });
}

// Render a single link field
function renderLinkField(id, linkData) {
    const linkField = document.createElement('div');
    linkField.className = 'link-field';
    linkField.id = `link-${id}`;

    // Handle both old (string) and new (object) format
    const url = typeof linkData === 'object' ? linkData.url : linkData;
    const name = typeof linkData === 'object' ? linkData.name : '';

    linkField.innerHTML = `
        <input type="text" class="link-name" value="${name}" placeholder="Link name (optional)">
        <input type="text" class="link-url" value="${url}" placeholder="Enter link URL">
        <button class="remove-btn" data-id="${id}">Remove</button>
    `;

    linksContainer.appendChild(linkField);
    linksContainer.style.display = 'block'; // Show container when adding links
    
    // Add remove button listener
    linkField.querySelector('.remove-btn').addEventListener('click', () => {
        removeLinkField(id);
    });
}

// Add a new link field
function addLinkField() {
    const newId = Date.now().toString(); // Use timestamp as unique ID
    renderLinkField(newId, { name: '', url: '', timeSpent: 0 });
}

// Remove a link field
function removeLinkField(id) {
    const fieldElement = document.getElementById(`link-${id}`);
    if (fieldElement) {
        fieldElement.remove();
        
        // Hide container if no more links
        const remainingFields = document.querySelectorAll('.link-field');
        if (remainingFields.length === 0) {
            linksContainer.style.display = 'none';
        }
    }
}

// Save all links to Chrome Storage
function saveLinks() {
    chrome.storage.sync.get(['links'], (result) => {
        const links = result.links || {};
        const linkFields = document.querySelectorAll('.link-field');
        
        // Get current links from DOM
        const updatedLinks = {};
        linkFields.forEach((field) => {
            const id = field.id.replace('link-', '');
            const name = field.querySelector('.link-name').value.trim();
            const url = field.querySelector('.link-url').value.trim();
            
            if (url) { // Only save non-empty links
                updatedLinks[id] = {
                    name: name || new URL(url).hostname, // Use hostname as default name
                    url: url,
                    timeSpent: links[id]?.timeSpent || 0 // Preserve existing time
                };
            }
        });
        
        chrome.storage.sync.set({ links: updatedLinks }, () => {
            alert('Links saved successfully!');
            console.log('Saved links:', updatedLinks);
        });
    });
}

/**
 * Command Palette functionality for CoMPhy Lab website
 * This file contains all the functionality for the command palette
 */

// Make the command palette opening function globally available
window.openCommandPalette = function() {
  const palette = document.getElementById('simple-command-palette');
  if (palette) {
    palette.style.display = 'block';
    const input = document.getElementById('command-palette-input');
    if (input) {
      input.value = '';
      input.focus();
      if (typeof renderCommandResults === 'function') {
        renderCommandResults('');
      }
    }
  }
};

// Function to render command results based on search
function renderCommandResults(query) {
  const resultsContainer = document.getElementById('command-palette-results');
  if (!resultsContainer) return;
  
  // Clear results
  resultsContainer.innerHTML = '';
  
  // Get commands
  const commands = window.commandData || [];
  
  // Filter commands based on query
  const filteredCommands = query ? 
    commands.filter(cmd => 
      cmd.title.toLowerCase().includes(query.toLowerCase()) ||
      (cmd.section && cmd.section.toLowerCase().includes(query.toLowerCase()))
    ) : 
    commands;
  
  // Group by section
  const sections = {};
  filteredCommands.forEach(cmd => {
    if (!sections[cmd.section]) {
      sections[cmd.section] = [];
    }
    sections[cmd.section].push(cmd);
  });
  
  // If query is at least 3 characters, search the database as well
  if (query && query.length >= 3 && typeof window.searchDatabaseForCommandPalette === 'function') {
    // We'll use a promise to handle the async search
    window.searchDatabaseForCommandPalette(query).then(searchResults => {
      if (searchResults && searchResults.length > 0) {
        // Add search results to sections
        sections['Search Results'] = searchResults;
        
        // Re-render the UI with search results
        renderSections(sections, resultsContainer);
      }
    }).catch(err => {
      console.error('Error searching database:', err);
    });
  }
  
  // Render the sections we have now (this will be called immediately, and again if search results come in)
  renderSections(sections, resultsContainer);
  
  // Show message if no results
  if (Object.keys(sections).length === 0) {
    const noResults = document.createElement('div');
    noResults.className = 'command-palette-no-results';
    noResults.textContent = 'No commands found';
    resultsContainer.appendChild(noResults);
  }
}

// Helper function to render sections
function renderSections(sections, container) {
  // Clear container first
  container.innerHTML = '';
  
  // Create DOM elements for results
  Object.keys(sections).forEach(section => {
    const sectionEl = document.createElement('div');
    sectionEl.className = 'command-palette-section';
    
    const sectionTitle = document.createElement('div');
    sectionTitle.className = 'command-palette-section-title';
    sectionTitle.textContent = section;
    sectionEl.appendChild(sectionTitle);
    
    const commandsList = document.createElement('div');
    commandsList.className = 'command-palette-commands';
    
    sections[section].forEach(cmd => {
      const cmdEl = document.createElement('div');
      cmdEl.className = 'command-palette-command';
      
      let cmdContent = `
        <div class="command-palette-icon">${cmd.icon || ''}</div>
        <div class="command-palette-title">${cmd.title}</div>
      `;
      
      // Add excerpt for search results if available
      if (cmd.excerpt) {
        cmdContent += `<div class="command-palette-excerpt">${cmd.excerpt.substring(0, 120)}${cmd.excerpt.length > 120 ? '...' : ''}</div>`;
      }
      
      cmdEl.innerHTML = cmdContent;
      
      cmdEl.addEventListener('click', function(e) {
        if (typeof cmd.handler === 'function') {
          document.getElementById('simple-command-palette').style.display = 'none';
          cmd.handler();
        }
      });
      
      commandsList.appendChild(cmdEl);
    });
    
    sectionEl.appendChild(commandsList);
    container.appendChild(sectionEl);
  });
}

// Initialization function to set up command palette when DOM is loaded
function initCommandPalette() {
  // Ensure search database is preloaded for command palette search functionality
  // Try to prefetch the search database if it exists
  fetch('/assets/js/search_db.json').then(response => {
    if (response.ok) {
      return response.json();
    }
    throw new Error('Search database not found');
  }).then(data => {
    console.log('Search database prefetched for command palette');
    window.searchData = data;
    
    // Initialize Fuse.js with weighted keys
    window.searchFuse = new Fuse(data, {
      keys: [
        { name: 'title', weight: 0.7 },
        { name: 'content', weight: 0.2 },
        { name: 'tags', weight: 0.1 },
        { name: 'categories', weight: 0.1 }
      ],
      includeScore: true,
      threshold: 0.4
    });
  }).catch(err => {
    console.warn('Could not prefetch search database for command palette:', err.message);
  });

  // Set up backdrop click to close
  const backdrop = document.querySelector('.simple-command-palette-backdrop');
  if (backdrop) {
    backdrop.addEventListener('click', function() {
      document.getElementById('simple-command-palette').style.display = 'none';
    });
  }
  
  // Set up input handler
  const input = document.getElementById('command-palette-input');
  if (input) {
    input.addEventListener('input', function() {
      renderCommandResults(this.value);
    });
    
    input.addEventListener('keydown', function(e) {
      if (e.key === 'Escape') {
        document.getElementById('simple-command-palette').style.display = 'none';
      } else if (e.key === 'Enter') {
        const selectedCommand = document.querySelector('.command-palette-command.selected');
        if (selectedCommand) {
          selectedCommand.click();
        } else {
          const firstCommand = document.querySelector('.command-palette-command');
          if (firstCommand) {
            firstCommand.click();
          }
        }
      } else if (e.key === 'ArrowDown' || e.key === 'ArrowUp') {
        e.preventDefault();
        
        const commands = Array.from(document.querySelectorAll('.command-palette-command'));
        if (commands.length === 0) return;
        
        const currentSelected = document.querySelector('.command-palette-command.selected');
        let nextIndex = 0;
        
        if (currentSelected) {
          const currentIndex = commands.indexOf(currentSelected);
          currentSelected.classList.remove('selected');
          
          if (e.key === 'ArrowDown') {
            nextIndex = (currentIndex + 1) % commands.length;
          } else {
            nextIndex = (currentIndex - 1 + commands.length) % commands.length;
          }
        } else {
          nextIndex = e.key === 'ArrowDown' ? 0 : commands.length - 1;
        }
        
        commands[nextIndex].classList.add('selected');
        
        // Ensure the selected element is visible in the scroll view
        commands[nextIndex].scrollIntoView({
          behavior: 'smooth',
          block: 'nearest'
        });
      }
    });
  }
  
  // Register command palette keyboard shortcut
  document.addEventListener('keydown', function(e) {
    if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
      e.preventDefault();
      window.openCommandPalette();
    }
  });
  
  // Make command palette button work
  const commandPaletteBtn = document.getElementById('command-palette-btn');
  if (commandPaletteBtn) {
    commandPaletteBtn.addEventListener('click', function(e) {
      e.preventDefault();
      window.openCommandPalette();
    });
  }
}

// Run initialization when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
  // Initialize the command palette
  initCommandPalette();
  
  // Show appropriate shortcut text based on platform
  const isMac = navigator.platform.toUpperCase().indexOf('MAC') >= 0;
  document.querySelectorAll('.mac-theme-text').forEach(el => {
    el.style.display = isMac ? 'inline' : 'none';
  });
  document.querySelectorAll('.default-theme-text').forEach(el => {
    el.style.display = isMac ? 'none' : 'inline';
  });
  
  // Set the appropriate shortcut hint based on platform
  const shortcutHint = document.getElementById('command-palette-shortcut');
  if (shortcutHint) {
    shortcutHint.textContent = isMac ? '⌘K' : 'Ctrl+K';
  }
  
  // Ensure command palette button works correctly
  const commandPaletteBtn = document.getElementById('command-palette-btn');
  if (commandPaletteBtn) {
    console.log('Command palette button initialized with new styling');
    
    // Make sure the button retains focus styles
    commandPaletteBtn.addEventListener('focus', function() {
      this.classList.add('focused');
    });
    
    commandPaletteBtn.addEventListener('blur', function() {
      this.classList.remove('focused');
    });
  }
});

// Make functions available globally
window.renderCommandResults = renderCommandResults;
window.renderSections = renderSections;

// Function to search the database with priority sorting
window.searchDatabaseForCommandPalette = async function(query) {
  if (!window.searchFuse) {
    return [];
  }
  
  try {
    const results = window.searchFuse.search(query);
    
    // Sort results by priority first, then by Fuse.js score
    // Lower priority number = higher priority (1 is highest, 5 is lowest)
    const sortedResults = results.sort((a, b) => {
      // First compare by priority
      const priorityA = a.item.priority || 5; // Default to lowest priority if not specified
      const priorityB = b.item.priority || 5;
      
      if (priorityA !== priorityB) {
        return priorityA - priorityB; // Lower priority number comes first
      }
      
      // If priorities are equal, use Fuse.js score (lower score = better match)
      return a.score - b.score;
    });
    
    // Return at most 5 results
    return sortedResults.slice(0, 5).map(result => ({
      id: `search-result-${result.refIndex}`,
      title: result.item.title || 'Untitled',
      handler: () => { 
        if (result.item.url) {
          window.location.href = result.item.url; 
        }
      },
      section: "Search Results",
      icon: '<i class="fa-solid fa-file-lines"></i>',
      excerpt: result.item.excerpt || (result.item.content && result.item.content.substring(0, 100) + '...') || ''
    }));
  } catch (e) {
    console.error('Error searching database:', e);
    return [];
  }
};

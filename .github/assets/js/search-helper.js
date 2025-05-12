/**
 * Shared search helper functions for website command palette
 * This file contains the shared search functionality used by both command-data.js and command-palette.js
 */

// Initialize the window.searchHelper namespace if it doesn't exist
window.searchHelper = window.searchHelper || {};

// Shared search database function for command palette
window.searchHelper.searchDatabaseForCommandPalette = async function(query) {
  // Only perform search if query is at least 3 characters long
  if (!query || query.length < 3) {
    return [];
  }
  
  console.log('Searching database for:', query);
  
  try {
    // Check if we have a searchIndex already loaded in window
    if (!window.searchFuse && window.searchData) {
      // If we already have search data but no Fuse object
      try {
        window.searchFuse = new Fuse(window.searchData, {
          keys: [
            { name: 'title', weight: 0.7 },
            { name: 'content', weight: 0.2 },
            { name: 'tags', weight: 0.1 },
            { name: 'categories', weight: 0.1 }
          ],
          includeScore: true,
          threshold: 0.4
        });
      } catch (e) {
        console.error('Error creating Fuse instance:', e);
        return [];
      }
    } else if (!window.searchFuse) {
      // Try to fetch search database if it doesn't exist yet
      try {
        // Get base URL from meta tag to support GitHub Pages subfolders
        const baseUrlMeta = document.querySelector('meta[name="base-url"]');
        const baseUrl = baseUrlMeta ? baseUrlMeta.getAttribute('content') : '';
        const searchDbUrl = baseUrl ? `${baseUrl}/assets/js/search_db.json` : '/assets/js/search_db.json';
        
        const response = await fetch(searchDbUrl);
        if (response.ok) {
          try {
            const searchData = await response.json();
            if (!searchData || !Array.isArray(searchData)) {
              console.warn('Search database has invalid format');
              return [];
            }
            window.searchData = searchData;
            window.searchFuse = new Fuse(searchData, {
              keys: [
                { name: 'title', weight: 0.7 },
                { name: 'content', weight: 0.2 },
                { name: 'tags', weight: 0.1 },
                { name: 'categories', weight: 0.1 }
              ],
              includeScore: true,
              threshold: 0.4
            });
          } catch (e) {
            console.error('Error parsing search database JSON:', e);
            return [];
          }
        } else {
          console.warn(`No search database found (${response.status})`);
          return [];
        }
      } catch (e) {
        console.error('Error loading search database:', e);
        return [];
      }
    }
    
    // Perform the search
    if (window.searchFuse) {
      try {
        const results = window.searchFuse.search(query);
        
        // Sort results by priority first, then by Fuse.js score
        // Lower priority number = higher priority (1 is highest, 5 is lowest)
        const sortedResults = results.sort((a, b) => {
          // First compare by priority
          const priorityA = a.item.priority || a.item.priority === 0 ? a.item.priority : 5; // Default to lowest priority if not specified
          const priorityB = b.item.priority || b.item.priority === 0 ? b.item.priority : 5;
          
          if (priorityA !== priorityB) {
            return priorityA - priorityB; // Lower priority number comes first
          }
          
          // If priorities are equal, use Fuse.js score (lower score = better match)
          return a.score - b.score;
        });
        
        // Return at most 5 results to avoid cluttering the command palette
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
        console.error('Error performing search with Fuse:', e);
        return [];
      }
    }
  } catch (e) {
    console.error('Error searching database:', e);
  }
  
  return [];
};
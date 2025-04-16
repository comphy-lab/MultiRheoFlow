document.addEventListener('DOMContentLoaded', function() {
  // Detect if the user is on a Mac
  const isMac = navigator.platform.toUpperCase().indexOf('MAC') >= 0;
  
  // Update the displayed shortcut text
  const defaultThemeElements = document.querySelectorAll('.default-theme-text');
  const macThemeElements = document.querySelectorAll('.mac-theme-text');
  
  defaultThemeElements.forEach(function(element) {
    element.style.display = isMac ? 'none' : 'inline';
  });
  
  macThemeElements.forEach(function(element) {
    element.style.display = isMac ? 'inline' : 'none';
  });
}); 
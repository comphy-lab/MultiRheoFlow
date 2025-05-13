#!/usr/bin/env python3
"""
Test script to verify consistent HTML cleaning behavior between different scripts.
"""

import os
import sys
import tempfile
import shutil

# Enable importing from the current directory
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Import the shared patterns
from html_cleaning_patterns import apply_empty_anchor_cleanup as shared_cleanup
from html_cleaning_patterns import EMPTY_ANCHOR_PATTERNS

# Import the fix_empty_anchors implementation if available
try:
    from fix_empty_anchors import apply_empty_anchor_cleanup as fix_cleanup
    FIX_AVAILABLE = True
except ImportError:
    FIX_AVAILABLE = False
    print("Warning: fix_empty_anchors.py implementation not available")

# Test if clean_html.py is available and has BeautifulSoup
try:
    from clean_html import sanitize_html, clean_html_file, HAS_BEAUTIFULSOUP
    CLEAN_AVAILABLE = True
except ImportError:
    CLEAN_AVAILABLE = False
    print("Warning: clean_html.py implementation not available")

print("=== HTML Cleaning Tools Availability ===")
print(f"- Shared patterns module: Available")
print(f"- fix_empty_anchors.py implementation: {'Available' if FIX_AVAILABLE else 'Not Available'}")
print(f"- clean_html.py implementation: {'Available' if CLEAN_AVAILABLE else 'Not Available'}")
if CLEAN_AVAILABLE:
    print(f"- BeautifulSoup availability: {'Available' if HAS_BEAUTIFULSOUP else 'Not Available'}")

# Test HTML with various types of empty anchor tags
TEST_HTML = """<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
</head>
<body>
    <h1>Test HTML Cleaning</h1>
    <p>Regular paragraph with <b>bold text</b>.</p>
    
    <!-- Empty anchor with id and href -->
    <a id="test1" href="#"><!-- empty --></a>
    
    <!-- Empty anchor with href only -->
    <a href="#"><!-- empty --></a>
    
    <!-- Empty anchor with id only -->
    <a id="test2"><!-- empty --></a>
    
    <!-- Empty anchor with attributes in different order -->
    <a href="#" id="test3"><!-- empty --></a>
    
    <!-- Empty anchor with newlines -->
    <a id="test4" href="#">
    </a>
    
    <!-- Script tag containing empty anchors -->
    <script>
        document.write('<a id="test5" href="#"></a>');
        // Another one: <a href="#">empty</a>
    </script>
    
    <!-- Script tag that shouldn't be removed but sanitized -->
    <script>
        console.log("test");
    </script>
</body>
</html>"""

def test_html_cleanup_consistency():
    """Test consistency between different HTML cleanup implementations"""
    print("\n=== Testing HTML Cleanup Consistency ===")
    
    # Step 1: Test shared implementation
    shared_result = shared_cleanup(TEST_HTML)
    print(f"Shared cleanup removed {len(TEST_HTML) - len(shared_result)} characters")
    
    # Step 2: Test fix_empty_anchors implementation if available
    if FIX_AVAILABLE:
        fix_result = fix_cleanup(TEST_HTML)
        is_consistent = fix_result == shared_result
        print(f"fix_empty_anchors cleanup matches shared: {'Yes' if is_consistent else 'No'}")
        if not is_consistent:
            print("  - Differences detected between shared and fix_empty_anchors implementations")
    
    # Step 3: Create test file with sanitization for clean_html.py
    if CLEAN_AVAILABLE:
        with tempfile.NamedTemporaryFile(suffix='.html', delete=False) as tmp:
            tmp.write(TEST_HTML.encode('utf-8'))
            tmp_path = tmp.name
        
        try:
            # Run clean_html_file on the test file
            modified, count = clean_html_file(tmp_path)
            
            # Read the modified file
            with open(tmp_path, 'r', encoding='utf-8') as f:
                clean_result = f.read()
            
            # Check for script tags
            has_script_open = "<script" in clean_result
            has_script_escaped = "&lt;script" in clean_result
            
            print(f"clean_html.py removed approximately {count} tags")
            print(f"clean_html.py properly handled script tags: {'Yes' if not has_script_open or has_script_escaped else 'No'}")
            
        finally:
            # Clean up the temporary file
            if os.path.exists(tmp_path):
                os.unlink(tmp_path)
    
    print("\nTest completed.")

if __name__ == "__main__":
    test_html_cleanup_consistency()
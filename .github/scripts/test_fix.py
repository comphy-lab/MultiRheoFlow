#!/usr/bin/env python3
"""
Test script to verify the HTML double-encoding fix
"""

import sys
import os
import importlib.util

# Get the directory where this script is located
script_dir = os.path.dirname(os.path.abspath(__file__))

# Import the original sanitize_html function
sys.path.insert(0, script_dir)
original_path = os.path.join(script_dir, "clean_html.py.bak")
spec_original = importlib.util.spec_from_file_location("clean_html_original", original_path)
clean_html_original = importlib.util.module_from_spec(spec_original)
spec_original.loader.exec_module(clean_html_original)

# Import the fixed sanitize_html function
fixed_path = os.path.join(script_dir, "clean_html.py")
spec_fixed = importlib.util.spec_from_file_location("clean_html_fixed", fixed_path)
clean_html_fixed = importlib.util.module_from_spec(spec_fixed)
spec_fixed.loader.exec_module(clean_html_fixed)

# Read test HTML
test_html_path = os.path.join(script_dir, "test_html.html")
with open(test_html_path, "r") as f:
    test_html = f.read()

# Process with original function
original_result = clean_html_original.sanitize_html(test_html)

# Process with fixed function
fixed_result = clean_html_fixed.sanitize_html(test_html)

# Check for double-encoding
print("=== Original sanitize_html output ===")
print(original_result[:500])  # Show first 500 chars
print("\n=== Fixed sanitize_html output ===")
print(fixed_result[:500])  # Show first 500 chars

# Check specifically for &amp;lt; which would indicate double-encoding
if "&amp;lt;" in original_result:
    print("\nOriginal function produces double-encoding: &amp;lt; found")
else:
    print("\nOriginal function does NOT produce double-encoding")

if "&amp;lt;" in fixed_result:
    print("Fixed function produces double-encoding: &amp;lt; found")
else:
    print("Fixed function does NOT produce double-encoding")

# Check that script tags are still properly escaped
if "<script" in fixed_result:
    print("\nWARNING: Fixed function didn't escape script tags!")
elif "&lt;script" in fixed_result:
    print("\nSuccess: Fixed function properly escaped script tags")
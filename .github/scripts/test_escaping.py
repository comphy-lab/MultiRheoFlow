#!/usr/bin/env python3
"""
Test HTML escaping with and without the fix
"""

import re
import html

def sanitize_html_original(content):
    """Original version with double-encoding issue"""
    if not content:
        return ""

    # Remove potentially harmful script and iframe tags
    sanitized_content = re.sub(r'<\s*script', '&lt;script', content, flags=re.IGNORECASE)
    sanitized_content = re.sub(r'<\s*\/\s*script', '&lt;/script', sanitized_content, flags=re.IGNORECASE)
    sanitized_content = re.sub(r'<\s*iframe', '&lt;iframe', sanitized_content, flags=re.IGNORECASE)
    sanitized_content = re.sub(r'<\s*\/\s*iframe', '&lt;/iframe', sanitized_content, flags=re.IGNORECASE)

    # Remove on* event handlers (e.g., onclick, onload)
    sanitized_content = re.sub(r'on\w+\s*=\s*["\'][^"\']*["\']', '', sanitized_content, flags=re.IGNORECASE)

    # Remove javascript: URLs
    sanitized_content = re.sub(r'javascript\s*:', 'disabled-javascript:', sanitized_content, flags=re.IGNORECASE)

    # Finally, escape any remaining HTML special characters
    sanitized_content = html.escape(sanitized_content, quote=False)  # Don't escape quotes again

    return sanitized_content

def sanitize_html_fixed(content):
    """Fixed version that prevents double-encoding"""
    if not content:
        return ""

    # Remove potentially harmful script and iframe tags
    sanitized_content = re.sub(r'<\s*script', '&lt;script', content, flags=re.IGNORECASE)
    sanitized_content = re.sub(r'<\s*\/\s*script', '&lt;/script', sanitized_content, flags=re.IGNORECASE)
    sanitized_content = re.sub(r'<\s*iframe', '&lt;iframe', sanitized_content, flags=re.IGNORECASE)
    sanitized_content = re.sub(r'<\s*\/\s*iframe', '&lt;/iframe', sanitized_content, flags=re.IGNORECASE)

    # Remove on* event handlers (e.g., onclick, onload)
    sanitized_content = re.sub(r'on\w+\s*=\s*["\'][^"\']*["\']', '', sanitized_content, flags=re.IGNORECASE)

    # Remove javascript: URLs
    sanitized_content = re.sub(r'javascript\s*:', 'disabled-javascript:', sanitized_content, flags=re.IGNORECASE)

    # Unescape first to prevent double-encoding from previous regex replacements
    sanitized_content = html.unescape(sanitized_content)
    
    # Finally, escape any remaining HTML special characters
    sanitized_content = html.escape(sanitized_content, quote=False)  # Don't escape quotes again

    return sanitized_content

test_html = """<!DOCTYPE html>
<html>
<head>
    <title>Test HTML for Escaping</title>
</head>
<body>
    <h1>Test HTML</h1>
    <p>This is a test HTML file for checking the escaping mechanism.</p>
    
    <!-- This script tag should be escaped but not double-escaped -->
    <script>
        console.log("This is a script tag that should be escaped");
    </script>
    
    <!-- This iframe tag should be escaped but not double-escaped -->
    <iframe src="https://example.com"></iframe>
    
    <!-- This event handler should be removed -->
    <button onclick="alert('click')">Click me</button>
    
    <!-- This javascript: URL should be disabled -->
    <a href="javascript:alert('test')">Link</a>
</body>
</html>"""

# Process with original function
original_result = sanitize_html_original(test_html)

# Process with fixed function
fixed_result = sanitize_html_fixed(test_html)

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
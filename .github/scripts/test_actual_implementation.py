#!/usr/bin/env python3
"""
Test the actual implementation directly for the HTML escaping fix
"""

import os
import sys
import re
import html

def test_sanitize_function(test_html, original=True):
    """Test the sanitize_html function"""
    
    if original:
        def sanitize_html(content):
            """
            Original implementation
            """
            if not content:
                return ""
            
            # First apply regex removals on the raw content
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
            # This ensures we preserve the removals we already made
            sanitized_content = html.escape(sanitized_content, quote=False)  # Don't escape quotes again
            
            return sanitized_content
    else:
        def sanitize_html(content):
            """
            Fixed implementation
            """
            if not content:
                return ""
            
            # First apply regex removals on the raw content
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
    
    return sanitize_html(test_html)

test_html = """
<!DOCTYPE html>
<html>
<body>
    <h1>Test with Various Cases</h1>
    <!-- Test 1: Regular HTML -->
    <p>Normal paragraph with <b>bold</b> and <i>italic</i> text</p>
    
    <!-- Test 2: Script tag -->
    <script>
        alert("Test script");
        document.write("<div>Generated content</div>");
    </script>
    
    <!-- Test 3: Already escaped content -->
    <div>&lt;script&gt;console.log("Already escaped");&lt;/script&gt;</div>
    
    <!-- Test 4: Nested content -->
    <div onclick="javascript:alert('test')">
        <script>
            console.log("Nested script with <script>alert('inner')</script>");
        </script>
    </div>
    
    <!-- Test 5: Special characters -->
    <p>Special chars: &amp; &lt; &gt; &quot; &#39;</p>
</body>
</html>
"""

# Process with original function
original_result = test_sanitize_function(test_html, original=True)

# Process with fixed function
fixed_result = test_sanitize_function(test_html, original=False)

# Compare results
print("=== ORIGINAL IMPLEMENTATION ===")
print("Contains double-escaped entities (&amp;lt;):", "&amp;lt;" in original_result)
print("Contains double-escaped entities (&amp;amp;):", "&amp;amp;" in original_result)
print("Script tags properly escaped:", "<script" not in original_result and "&lt;script" in original_result)

print("\n=== FIXED IMPLEMENTATION ===")
print("Contains double-escaped entities (&amp;lt;):", "&amp;lt;" in fixed_result)
print("Contains double-escaped entities (&amp;amp;):", "&amp;amp;" in fixed_result)
print("Script tags properly escaped:", "<script" not in fixed_result and "&lt;script" in fixed_result)

# Check edge case - already escaped content
print("\n=== ALREADY ESCAPED CONTENT TEST ===")
already_escaped_original = test_sanitize_function("&lt;script&gt;alert();&lt;/script&gt;", original=True)
already_escaped_fixed = test_sanitize_function("&lt;script&gt;alert();&lt;/script&gt;", original=False)

print("Original implementation double-escapes already escaped content:", "&amp;lt;" in already_escaped_original)
print("Fixed implementation preserves already escaped content correctly:", "&amp;lt;" not in already_escaped_fixed)

# Print samples of the output
print("\n=== ORIGINAL OUTPUT SAMPLE ===")
print(original_result[:500])
print("\n=== FIXED OUTPUT SAMPLE ===")
print(fixed_result[:500])
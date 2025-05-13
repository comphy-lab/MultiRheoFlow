#!/usr/bin/env python3
"""
Test HTML escaping with more complex test cases
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

test_complex_html = """<!DOCTYPE html>
<html>
<head>
    <title>Complex HTML Test</title>
</head>
<body>
    <!-- Nested tags with entities -->
    <div class="container">
        <p>Regular paragraph with &amp; ampersand and &lt; less than</p>
        <script>
            // Nested script with HTML
            document.write("<p>Generated paragraph</p>");
            document.write("<script>alert('nested!')</script>");
        </script>
        <iframe src="data:text/html,<script>alert('iframe')</script>"></iframe>
    </div>
    
    <!-- Already escaped content -->
    <div>
        &lt;script&gt;console.log("Already escaped");&lt;/script&gt;
    </div>
    
    <!-- Mixed content -->
    <button onclick="javascript:alert('mix &lt;script&gt; tags')">
        Click &amp; Test
    </button>
</body>
</html>"""

# Process with original function
original_result = sanitize_html_original(test_complex_html)

# Process with fixed function
fixed_result = sanitize_html_fixed(test_complex_html)

print("=== ORIGINAL IMPLEMENTATION RESULTS ===")
print("Double encoded &amp;lt;? " + ("Yes" if "&amp;lt;" in original_result else "No"))
print("Double encoded &amp;amp;? " + ("Yes" if "&amp;amp;" in original_result else "No"))
print("Contains unescaped <script>? " + ("Yes" if "<script>" in original_result else "No"))

print("\n=== FIXED IMPLEMENTATION RESULTS ===")
print("Double encoded &amp;lt;? " + ("Yes" if "&amp;lt;" in fixed_result else "No"))
print("Double encoded &amp;amp;? " + ("Yes" if "&amp;amp;" in fixed_result else "No"))
print("Contains unescaped <script>? " + ("Yes" if "<script>" in fixed_result else "No"))

# Check for particular problematic sequences
print("\n=== SPECIFIC TESTS ===")
# Check if already-escaped content is double-escaped in original
already_escaped_orig = "'&amp;lt;script&gt;console.log(\"Already escaped\");&amp;lt;/script&gt;'" in original_result
print("Original double-escapes already escaped content: " + ("Yes" if already_escaped_orig else "No"))

# Check if already-escaped content is properly preserved in fixed version
already_escaped_fixed = "'&lt;script&gt;console.log(\"Already escaped\");&lt;/script&gt;'" in fixed_result
print("Fixed correctly handles already escaped content: " + ("Yes" if already_escaped_fixed else "No"))

# Output snippets for comparison
print("\n=== ORIGINAL OUTPUT SNIPPET ===")
snippet_pos = original_result.find("&amp;lt;script")
if snippet_pos > -1:
    print(original_result[max(0, snippet_pos-20):min(len(original_result), snippet_pos+100)])

print("\n=== FIXED OUTPUT SNIPPET ===")
snippet_pos = fixed_result.find("&lt;script")
if snippet_pos > -1:
    print(fixed_result[max(0, snippet_pos-20):min(len(fixed_result), snippet_pos+100)])
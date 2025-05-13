#!/usr/bin/env python3
"""
Test edge cases for the HTML escaping fix
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

# Test 1: Edge case with mixed content (already escaped + script tags)
test1 = '<div class="code">&lt;script&gt;alert("Escaped");&lt;/script&gt;</div><script>console.log("Real script");</script>'

# Test 2: Entity references that should be preserved
test2 = 'Characters: &amp; &lt; &gt; &#39; &quot; &#x2F; &#x27; &#x2b; &#x5c;'

# Test 3: Nested script tags
test3 = '<div><script>var str = "<script>nested</script>";</script></div>'

# Test 4: Unicode characters
test4 = '<script>\u00A9 \u00AE \u2122 \u20AC \u00A3 \u00A5</script>'

# Run tests
print("=== TEST 1: MIXED CONTENT ===")
print("Original: " + sanitize_html_original(test1)[:150])
print("Fixed   : " + sanitize_html_fixed(test1)[:150])
print("Double encoding in original? " + ("Yes" if "&amp;lt;" in sanitize_html_original(test1) else "No"))
print("Double encoding in fixed? " + ("Yes" if "&amp;lt;" in sanitize_html_fixed(test1) else "No"))
print()

print("=== TEST 2: ENTITY REFERENCES ===")
print("Original: " + sanitize_html_original(test2))
print("Fixed   : " + sanitize_html_fixed(test2))
print("Double encoding in original? " + ("Yes" if "&amp;amp;" in sanitize_html_original(test2) else "No"))
print("Double encoding in fixed? " + ("Yes" if "&amp;amp;" in sanitize_html_fixed(test2) else "No"))
print()

print("=== TEST 3: NESTED SCRIPT TAGS ===")
print("Original: " + sanitize_html_original(test3)[:150])
print("Fixed   : " + sanitize_html_fixed(test3)[:150])
print("Script tags escaped in original? " + ("Yes" if "&lt;script" in sanitize_html_original(test3) else "No"))
print("Script tags escaped in fixed? " + ("Yes" if "&lt;script" in sanitize_html_fixed(test3) else "No"))
print()

print("=== TEST 4: UNICODE CHARACTERS ===")
print("Original: " + sanitize_html_original(test4))
print("Fixed   : " + sanitize_html_fixed(test4))
print("Script tags escaped in original? " + ("Yes" if "&lt;script" in sanitize_html_original(test4) else "No"))
print("Script tags escaped in fixed? " + ("Yes" if "&lt;script" in sanitize_html_fixed(test4) else "No"))
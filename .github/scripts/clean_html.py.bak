#!/usr/bin/env python3
"""
HTML Cleaner Script

This script removes empty anchor tags from HTML files that cause JavaScript syntax errors.
It specifically targets tags like <a id="" href="#"></a> which are being incorrectly inserted
during the documentation generation process.

Relationship to fix_empty_anchors.py:
    - clean_html.py: Uses BeautifulSoup for robust HTML parsing and sanitization. It can handle 
      malformed or complex HTML structure, including script tags. Requires the bs4 dependency.
    - fix_empty_anchors.py: A lightweight alternative using regex-only approach without external 
      dependencies. It works faster but is less robust with malformed HTML.

When to use which script:
    - Use clean_html.py when dealing with complex HTML or when you need robust sanitization
      (especially for HTML inside script tags)
    - Use fix_empty_anchors.py when you need a fast, dependency-free solution for well-formed HTML
      or when installing external dependencies is not possible

Dependencies:
    - beautifulsoup4 (BSD-licensed HTML/XML parser)
    - Install with: pip install -r requirements.txt

Usage:
    python clean_html.py --dir /path/to/html/files --verbose
"""

import os
import re
import argparse
import sys
import html
from bs4 import BeautifulSoup

def sanitize_html(content):
    """
    Sanitize HTML content to prevent XSS vulnerabilities.

    Args:
        content: HTML content to sanitize

    Returns:
        str: Sanitized HTML content
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

def clean_html_file(file_path):
    """
    Removes empty anchor tags from an HTML file using BeautifulSoup.
    Also performs direct string replacement for HTML in script tags that
    BeautifulSoup might not properly handle.

    Args:
        file_path: Path to the HTML file to clean

    Returns:
        Tuple (bool, int): Whether file was modified and count of tags removed
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # First do a direct string replacement for empty anchor tags in the content
        # This is more reliable for script tags where BeautifulSoup might struggle
        initial_content_length = len(content)
        pattern = r'<a\s+id=[\'"]?[\'"]?\s*href=[\'"]?#[\'"]?\s*>\s*</a>'
        content = re.sub(pattern, '', content)

        # Also handle empty anchors with newlines between tags
        pattern = r'<a\s+id=[\'"]?[\'"]?\s*href=[\'"]?#[\'"]?\s*>\s*\n*\s*</a>'
        content = re.sub(pattern, '', content)

        # Count direct replacements
        direct_replacements = (initial_content_length - len(content)) // 30  # Approximate count

        # Parse the HTML with BeautifulSoup
        soup = BeautifulSoup(content, 'html.parser')

        # Find all empty anchor tags
        empty_anchors = [a for a in soup.find_all('a') if not a.contents or (len(a.contents) == 1 and not a.contents[0].strip())]

        # Count the empty anchors before removing
        original_count = len(empty_anchors)

        # Remove the empty anchors found by BeautifulSoup if any exist
        for anchor in empty_anchors:
            anchor.decompose()

        # Special handling for script tags
        script_tags = soup.find_all('script')
        for script in script_tags:
            # Clean script content if it exists
            if script.string:
                # Apply direct regex replacement within script content
                script_content = script.string
                script_content = re.sub(pattern, '', script_content)

                # Apply direct regex removal of problematic anchor tags in script content
                # This is more effective than trying to parse and modify the script with BeautifulSoup
                # since the script content might already be sanitized/escaped
                
                # Remove empty anchor tags with various patterns
                script_content = re.sub(r'<a\s+id=[\'"]?[\'"]?\s*href=[\'"]?#[\'"]?\s*>\s*</a>', '', script_content)
                script_content = re.sub(r'<a\s+id=[\'"]?[\'"]?\s*href=[\'"]?#[\'"]?\s*>\s*\n*\s*</a>', '', script_content)
                script_content = re.sub(r'<a\s+href=[\'"]?#[\'"]?\s*id=[\'"]?[\'"]?\s*>\s*</a>', '', script_content)
                
                # Sanitize javascript: URLs directly in the script
                script_content = re.sub(r'javascript\s*:', 'disabled-javascript:', script_content, flags=re.IGNORECASE)
                
                # Directly use the cleaned script content
                cleaned_script = script_content

                # Update the script content
                script.string = cleaned_script

        # Convert the soup back to HTML
        cleaned_content = str(soup)

        # Perform a final direct replacement to catch any that might still remain
        # (especially within attributes or other places BeautifulSoup might miss)
        cleaned_content = re.sub(pattern, '', cleaned_content)

        # Write the cleaned content back to the file
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(cleaned_content)

        total_removed = original_count + direct_replacements
        return True, total_removed

    except Exception as e:
        print(f"Error cleaning {file_path}: {e}")
        return False, 0

def process_directory(directory, verbose=False):
    """
    Process all HTML files in a directory recursively.
    
    Args:
        directory: Root directory to search for HTML files
        verbose: Whether to print verbose output
    
    Returns:
        dict: Statistics about processed files
    """
    stats = {
        'total_files': 0,
        'modified_files': 0,
        'total_tags_removed': 0,
        'errors': 0
    }
    
    if verbose:
        print(f"Processing directory: {directory}")
    
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith('.html'):
                file_path = os.path.join(root, file)
                stats['total_files'] += 1
                
                try:
                    modified, tags_removed = clean_html_file(file_path)
                    
                    if modified:
                        stats['modified_files'] += 1
                        stats['total_tags_removed'] += tags_removed
                        
                        if verbose:
                            print(f"Cleaned {file_path}: removed {tags_removed} empty anchor tags")
                            
                except Exception as e:
                    stats['errors'] += 1
                    print(f"Error processing {file_path}: {e}")
    
    return stats

def main():
    """Main function to parse arguments and run the script."""
    parser = argparse.ArgumentParser(description='Clean HTML files by removing empty anchor tags')
    parser.add_argument('--dir', required=True, help='Directory containing HTML files to clean')
    parser.add_argument('--verbose', action='store_true', help='Print verbose output')
    args = parser.parse_args()
    
    if not os.path.isdir(args.dir):
        print(f"Error: {args.dir} is not a valid directory")
        sys.exit(1)
    
    stats = process_directory(args.dir, args.verbose)
    
    print("\nProcessing Summary:")
    print(f"Total files processed: {stats['total_files']}")
    print(f"Files modified: {stats['modified_files']}")
    print(f"Total empty anchor tags removed: {stats['total_tags_removed']}")
    print(f"Errors encountered: {stats['errors']}")
    
    if stats['modified_files'] > 0:
        print("\nEmpty anchor tags successfully removed from HTML files.")
    else:
        print("\nNo empty anchor tags found or all files were already clean.")

if __name__ == "__main__":
    main()
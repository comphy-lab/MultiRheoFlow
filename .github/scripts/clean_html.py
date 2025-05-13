#!/usr/bin/env python3
"""
HTML Cleaner Script

This script removes empty anchor tags from HTML files that cause JavaScript syntax errors.
It specifically targets tags like <a id="" href="#"></a> which are being incorrectly inserted
during the documentation generation process.

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

    # Escape HTML special characters to prevent script execution
    escaped_content = html.escape(content)

    # Additional sanitization steps
    # Remove potentially harmful script and iframe tags
    escaped_content = re.sub(r'<\s*script', '&lt;script', escaped_content, flags=re.IGNORECASE)
    escaped_content = re.sub(r'<\s*\/\s*script', '&lt;/script', escaped_content, flags=re.IGNORECASE)
    escaped_content = re.sub(r'<\s*iframe', '&lt;iframe', escaped_content, flags=re.IGNORECASE)
    escaped_content = re.sub(r'<\s*\/\s*iframe', '&lt;/iframe', escaped_content, flags=re.IGNORECASE)

    # Remove on* event handlers (e.g., onclick, onload)
    escaped_content = re.sub(r'on\w+\s*=\s*["\'][^"\']*["\']', '', escaped_content, flags=re.IGNORECASE)

    # Remove javascript: URLs
    escaped_content = re.sub(r'javascript\s*:', 'disabled-javascript:', escaped_content, flags=re.IGNORECASE)

    return escaped_content

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

        if original_count == 0 and direct_replacements == 0:
            return False, 0

        # Remove the empty anchors found by BeautifulSoup
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

                # Sanitize the script content before parsing to prevent XSS vulnerabilities
                # This prevents malicious script content from being executed when parsed by BeautifulSoup
                # and eliminates potential security issues if the HTML content comes from untrusted sources
                sanitized_content = sanitize_html(script_content)

                # Create a temporary soup object for the sanitized script content
                script_soup = BeautifulSoup(f"<div>{sanitized_content}</div>", 'html.parser')

                # Find and remove any anchor tags in the script content
                for anchor in script_soup.find_all('a'):
                    anchor.decompose()

                # Get the cleaned content (excluding the wrapping div)
                cleaned_script = script_soup.div.decode_contents() if script_soup.div else ""

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
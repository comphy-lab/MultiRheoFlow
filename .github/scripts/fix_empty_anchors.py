#!/usr/bin/env python3
"""
Direct HTML Fix Script for Empty Anchors

This script directly removes empty anchor tags from HTML files that cause JavaScript syntax errors.
It uses regex-based string replacement without external dependencies, making it fast and lightweight.

Relationship to clean_html.py:
    - fix_empty_anchors.py: Fast, dependency-free approach using regular expressions. Works best
      with well-formed HTML where the empty anchor patterns are consistent. This script is ideal
      for quick cleanup or environments where installing dependencies is not possible.
    - clean_html.py: Uses BeautifulSoup for robust HTML parsing and sanitization. It can handle
      malformed HTML, complex nested structures, and content within script tags. Requires the
      beautifulsoup4 library.

Trade-offs:
    - Speed: fix_empty_anchors.py is generally faster as it avoids DOM parsing
    - Robustness: clean_html.py is more robust for complex or malformed HTML
    - Dependencies: fix_empty_anchors.py has no external dependencies
    - Script Tag Handling: clean_html.py better handles anchors in script tags

When to use which script:
    - Use fix_empty_anchors.py for: quick fixes, CI/CD environments with limited dependencies,
      well-formed HTML, performance-critical applications
    - Use clean_html.py for: complex HTML documents, when sanitization is important, or when
      working with HTML that may contain script tags with anchors

Dependencies:
    - None - relies only on standard library (re, os, glob)

Usage:
    python fix_empty_anchors.py [options] <path>
"""

import sys
import re
import os
import glob
import argparse

def fix_html_file(file_path, verbose=False, dry_run=False):
    """
    Removes empty anchor tags from an HTML file using direct string replacement.

    Args:
        file_path: Path to the HTML file to clean
        verbose: If True, prints detailed information
        dry_run: If True, shows changes without writing to file

    Returns:
        int: Number of replacements made
    """
    try:
        # Read the file
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Store original content length for comparison
        initial_content_length = len(content)

        if verbose:
            print(f"Processing file: {file_path}")
        
        # Improved patterns for empty anchor tags with proper attribute handling
        patterns = [
            # Handle id attribute followed by href attribute
            r'<a\s+id=[\'"]?([^\s>]*)[\'"]?\s+href=[\'"]?#[\'"]?\s*>\s*</a>',
            r'<a\s+id=[\'"]?([^\s>]*)[\'"]?\s+href=[\'"]?#[\'"]?\s*>\s*(?:\n\s*)*</a>',

            # Handle href attribute followed by id attribute
            r'<a\s+href=[\'"]?#[\'"]?\s+id=[\'"]?([^\s>]*)[\'"]?\s*>\s*</a>',
            r'<a\s+href=[\'"]?#[\'"]?\s+id=[\'"]?([^\s>]*)[\'"]?\s*>\s*(?:\n\s*)*</a>',

            # Handle only id attribute
            r'<a\s+id=[\'"]?([^\s>]*)[\'"]?\s*>\s*</a>',
            r'<a\s+id=[\'"]?([^\s>]*)[\'"]?\s*>\s*(?:\n\s*)*</a>',

            # Handle only href='#' attribute (empty links)
            r'<a\s+href=[\'"]?#[\'"]?\s*>\s*</a>',
            r'<a\s+href=[\'"]?#[\'"]?\s*>\s*(?:\n\s*)*</a>',

            # Handle unquoted attributes (not recommended but seen in some HTML)
            r'<a\s+id=([^\s>]*)\s+href=#\s*>\s*</a>',
            r'<a\s+href=#\s+id=([^\s>]*)\s*>\s*</a>'
        ]
        
        # Apply all patterns
        modified_content = content
        for pattern in patterns:
            modified_content = re.sub(pattern, '', modified_content)

        # Calculate approximate number of replacements
        final_content_length = len(modified_content)
        chars_removed = initial_content_length - final_content_length
        replacements = chars_removed // 20  # Approximate size of each anchor tag

        # Only proceed if changes were made
        if replacements > 0:
            if verbose:
                print(f"  Found approximately {replacements} empty anchor tags")

            if dry_run:
                print(f"[DRY RUN] Would fix {file_path}: {replacements} empty anchor tags")
            else:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(modified_content)
                print(f"Fixed {file_path}: removed approximately {replacements} empty anchor tags")
        elif verbose:
            print(f"  No empty anchors found in {file_path}")
        
        return replacements
        
    except Exception as e:
        print(f"Error fixing {file_path}: {e}")
        return 0

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(
        description="Fix empty anchor tags in HTML files that cause JavaScript syntax errors.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument(
        "path",
        help="Path to an HTML file or directory containing HTML files"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose output"
    )
    parser.add_argument(
        "-d", "--dry-run",
        action="store_true",
        help="Show what would be fixed without making changes"
    )

    args = parser.parse_args()

    path = args.path
    verbose = args.verbose
    dry_run = args.dry_run

    if dry_run:
        print("Running in dry-run mode - no changes will be made")

    if os.path.isfile(path):
        # Fix a single file
        fix_html_file(path, verbose=verbose, dry_run=dry_run)
    elif os.path.isdir(path):
        # Fix all HTML files in the directory and subdirectories
        if verbose:
            print(f"Searching for HTML files in {path} and subdirectories...")

        html_files = glob.glob(os.path.join(path, '**', '*.html'), recursive=True)
        total_files = len(html_files)

        if verbose:
            print(f"Found {total_files} HTML files")

        fixed_files = 0
        total_replacements = 0

        for file in html_files:
            replacements = fix_html_file(file, verbose=verbose, dry_run=dry_run)
            if replacements > 0:
                fixed_files += 1
                total_replacements += replacements

        action = "Would fix" if dry_run else "Fixed"
        print(f"\nSummary: {action} {fixed_files} out of {total_files} files, removing approximately {total_replacements} empty anchor tags")
    else:
        print(f"Error: {path} is not a valid file or directory")
        sys.exit(1)

if __name__ == '__main__':
    main()
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

def clean_html_file(file_path):
    """
    Removes empty anchor tags from an HTML file.
    
    Args:
        file_path: Path to the HTML file to clean
        
    Returns:
        Tuple (bool, int): Whether file was modified and count of tags removed
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Count occurrences before cleaning
        original_count = len(re.findall(r'<a[^>]*>\s*</a>', content))
        
        if original_count == 0:
            return False, 0
            
        # Remove empty anchor tags from the entire document
        cleaned_content = re.sub(r'<a[^>]*>\s*</a>', '', content)
        
        # Special handling for script blocks
        script_pattern = r'(<script[^>]*>.*?</script>)'
        
        def clean_script_blocks(match):
            script_content = match.group(1)
            # Aggressively remove ALL anchor tags from script blocks
            script_content = re.sub(r'<a[^>]*>.*?</a>', '', script_content)
            return script_content
            
        # Clean script blocks separately
        cleaned_content = re.sub(script_pattern, clean_script_blocks, cleaned_content, flags=re.DOTALL)
        
        # Count occurrences after cleaning
        final_count = len(re.findall(r'<a[^>]*>\s*</a>', cleaned_content))
        
        # Write the cleaned content back to the file
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(cleaned_content)
            
        return True, original_count - final_count
    
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

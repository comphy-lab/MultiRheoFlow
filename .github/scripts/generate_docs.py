import os
import subprocess
import re
import shutil
import argparse
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Set, Any, Union

# Parse command line arguments
parser = argparse.ArgumentParser(description='Generate documentation from source files.')
parser.add_argument('--debug', action='store_true', help='Enable debug output')
args = parser.parse_args()

# Global debug flag
DEBUG = args.debug

def debug_print(message):
    """Print debug messages only if debug mode is enabled."""
    if DEBUG:
        print(message)

def extract_seo_metadata(file_path: Path, file_content: str) -> Dict[str, str]:
    """
    Extract SEO metadata from the given file content.
    
    This function scans the file content to obtain SEO metadata by extracting a meta description and a set of keywords. The description is derived from the first comment block in the file, cleaned of markdown formatting, and truncated to approximately 160 characters. Keywords are identified by matching predefined technical patterns in the content and by extracting meaningful tokens from the file name. The metadata is returned as a dictionary with keys "description" and "keywords".
    """
    metadata = {}
    
    # Extract first paragraph as description (up to 160 chars)
    # Try to find a documentation comment or a paragraph with actual text, not code
    description_match = re.search(r'^\s*#\s*(.*?)\s*$\s*([a-zA-Z].*?)(?=^\s*#|\Z)', file_content, re.MULTILINE | re.DOTALL)
    if description_match:
        # First try the paragraph after the heading
        description = description_match.group(2).strip()
        
        # If that's empty or just code, use the heading itself
        if not description or description.startswith(('```', '`', '#', '//')):
            description = description_match.group(1).strip()
        
        # If that's empty or just code, use the heading itself
        if not description or description.startswith(('```', '`', '#', '//')):
            description = description_match.group(1).strip()

# Configuration
# Assume the script is in .github/scripts, REPO_ROOT is the parent of .github
REPO_ROOT = Path(__file__).parent.parent.parent
SOURCE_DIRS = ['src-local', 'simulationCases', 'postProcess']  # Directories within REPO_ROOT to scan
DOCS_DIR = REPO_ROOT / 'docs'
README_PATH = REPO_ROOT / 'README.md'
INDEX_PATH = DOCS_DIR / 'index.html'
# --- New configuration based on page2html ---
BASILISK_DIR = REPO_ROOT / 'basilisk'  # Assuming basilisk dir is at the root
DARCSIT_DIR = BASILISK_DIR / 'src' / 'darcsit'
TEMPLATE_PATH = REPO_ROOT / '.github' / 'assets' / 'custom_template.html'  # Use the modified local template
LITERATE_C_SCRIPT = DARCSIT_DIR / 'literate-c'  # Path to the literate-c script
BASE_URL = "/"  # Relative base URL for links within the site
CSS_PATH = REPO_ROOT / '.github' / 'assets' / 'css' / 'custom_styles.css'  # Path to custom CSS

# Read domain from CNAME file or use default
try:
    CNAME_PATH = REPO_ROOT / 'CNAME'
    BASE_DOMAIN = f"https://{CNAME_PATH.read_text().strip()}" if CNAME_PATH.exists() else "https://test.comphy-lab.org"
except Exception as e:
    print(f"Warning: Could not read CNAME file: {e}")
    BASE_DOMAIN = "https://test.comphy-lab.org"

def extract_h1_from_readme(readme_path: Path) -> str:
    """
    Extract the first markdown H1 header from a README file.
    
    This function reads the file at the specified path using UTF-8 encoding and searches for the
    first line that starts with "# ". If an H1 header is found, the function returns its trimmed text.
    If no header is present or an error occurs during reading, it returns the default title "Documentation".
    """
    try:
        with open(readme_path, 'r', encoding='utf-8') as f:
            content = f.read()
            # Look for # Heading pattern
            h1_match = re.search(r'^# (.+)$', content, re.MULTILINE)
            if h1_match:
                return h1_match.group(1).strip()
            else:
                debug_print("Warning: No h1 heading found in README.md")
                return "Documentation"
    except Exception as e:
        print(f"Error reading README.md: {e}")
        return "Documentation"


# Dynamically get the wiki title from README.md
WIKI_TITLE = extract_h1_from_readme(README_PATH)


def process_template_for_assets(template_path: Path) -> str:
    """
    Process the custom template to ensure correct asset paths.
    
    This function reads the template HTML file and ensures that all asset references
    use the correct paths relative to the root. It converts paths like $base$/assets/...
    to the correct format for the generated documentation.
    
    Args:
        template_path: Path to the custom HTML template
        
    Returns:
        The processed template content as a string
    """
    try:
        with open(template_path, 'r', encoding='utf-8') as f:
            template_content = f.read()
        debug_print("Template processed for correct asset paths")
        return template_content
    except Exception as e:
        print(f"Error processing template for assets: {e}")
        return ""


def validate_config() -> bool:
    """
    Validates that all required configuration paths exist.
    
    Checks if the necessary directories (BASILISK_DIR and DARCSIT_DIR) and files (TEMPLATE_PATH and the literate-c script)
    are present. If any path is missing, an error is printed and the function returns False; otherwise, it returns True.
    """
    global TEMPLATE_PATH
    
    essential_paths = [
        (BASILISK_DIR, "BASILISK_DIR"),
        (DARCSIT_DIR, "DARCSIT_DIR"),
        (TEMPLATE_PATH, "TEMPLATE_PATH"),
        (LITERATE_C_SCRIPT, "literate-c script")
    ]

    for path, name in essential_paths:
        if not (path.is_dir() if name.endswith("DIR") else path.is_file()):
            print(f"Error: {name} not found at {path}")
            return False
    
    # Process the template to ensure correct asset paths
    # Process the template to ensure correct asset paths
    processed_template = process_template_for_assets(TEMPLATE_PATH)
    if not processed_template:
        return False
    # Create a temporary template file with processed content
    temp_template_path = TEMPLATE_PATH.with_suffix('.temp.html')
    
    # Clean up any existing temporary file
    if temp_template_path.exists():
        try:
            temp_template_path.unlink()
        except Exception as e:
            print(f"Warning: Could not delete existing temporary template: {e}")
    
    try:
        with open(temp_template_path, 'w', encoding='utf-8') as f:
            f.write(processed_template)
        # Replace the template path with the temporary one
        TEMPLATE_PATH = temp_template_path
    except Exception as e:
        print(f"Error creating temporary template file: {e}")
        return False
    
    return True


def find_source_files(root_dir: Path, source_dirs: List[str]) -> List[Path]:
    """
    Recursively searches for C, header, Python, and shell script files in specified directories.
    
    This function scans each directory listed in `source_dirs` (relative to `root_dir`)
    using a recursive search for files with extensions .c, .h, .py, and .sh. It also
    identifies .sh files that are directly located in the `root_dir`.
    
    Args:
        root_dir: The root directory to begin the search.
        source_dirs: List of directory names (relative to `root_dir`) to search recursively.
    
    Returns:
        A list of Path objects for all discovered source files.
    """
    files = []
    # Search in source directories
    for dir_name in source_dirs:
        src_path = root_dir / dir_name
        if src_path.is_dir():
            files.extend(src_path.rglob('*.c'))
            files.extend(src_path.rglob('*.h'))
            files.extend(src_path.rglob('*.py'))
            files.extend(src_path.rglob('*.sh'))
    
    # Also search for .sh files directly in the root directory
    for sh_file in root_dir.glob('*.sh'):
        files.append(sh_file)
        
    return files


def process_markdown_file(file_path: Path) -> str:
    """
    Process markdown file content for HTML conversion.
    
    Args:
        file_path: Path to the markdown file
        
    Returns:
        Content ready for pandoc conversion
    
    Raises:
        Exception: If file reading fails
    """
    with open(file_path, 'r', encoding='utf-8') as f:
        file_content = f.read()
    return file_content


def process_shell_file(file_path: Path) -> str:
    """
    Reads the specified shell script and wraps its content in a bash code block.
    
    This function opens the given shell file with UTF-8 encoding, reads its complete 
    content, and encloses it within markdown fenced code block delimiters labeled 
    with "bash" to facilitate HTML conversion.
    
    Args:
        file_path: Path to the shell script file.
    
    Returns:
        A string containing the shell script content formatted as a bash code block.
    """
    with open(file_path, 'r', encoding='utf-8') as f:
        file_content = f.read()
    return f"```bash\n{file_content}\n```"


def process_python_file(file_path: Path) -> str:
    """
    Process a Python file for Markdown conversion.
    
    Reads a Python source file and separates its triple-quoted docstrings from its code. Docstrings
    are cleaned of enclosing quotes and inserted as plain text, while code blocks are wrapped in
    Markdown fences with a Python specifier. This formatting produces a Markdown string suitable for
    HTML conversion via pandoc.
    
    Args:
        file_path: The path to the Python file to process.
    
    Returns:
        A Markdown-formatted string containing the processed content.
    """
    with open(file_path, 'r', encoding='utf-8') as f:
        file_content = f.read()
    
    lines = file_content.split('\n')
    processed_lines = []
    in_code_block = False
    code_block = []
    in_docstring = False
    docstring_lines = []
    
    for line in lines:
        # Check for docstring comments (triple quotes)
        if line.strip().startswith('"""') or line.strip().startswith("'''"):
            # If we're in a docstring, end it
            if in_docstring:
                in_docstring = False
                # Add the docstring as text, but skip any lines that only contain quotes
                clean_docstring = []
                for doc_line in docstring_lines:
                    # Skip lines that only contain quotes
                    if doc_line.strip() in ('"""', "'''"):
                        continue
                    # Remove starting/ending quotes from lines that have content
                    doc_line = doc_line.strip()
                    if doc_line.startswith('"""') or doc_line.startswith("'''"):
                        doc_line = doc_line[3:]
                    if doc_line.endswith('"""') or doc_line.endswith("'''"):
                        doc_line = doc_line[:-3]
                    clean_docstring.append(doc_line.strip())
                
                # Only add non-empty lines
                if clean_docstring:
                    processed_lines.append("")
                    processed_lines.extend(clean_docstring)
                    processed_lines.append("")
                docstring_lines = []
            else:
                # Start a new docstring
                in_docstring = True
                # If we're in a code block, end it
                if in_code_block:
                    processed_lines.append("```python")
                    processed_lines.extend(code_block)
                    processed_lines.append("```")
                    code_block = []
                    in_code_block = False
            continue
        
        # If we're in a docstring, add the line to docstring_lines
        if in_docstring:
            docstring_lines.append(line)
            continue
        
        # For regular code lines (including # comments)
        if not in_code_block and line.strip():
            in_code_block = True
            code_block.append(line)
        elif in_code_block:
            code_block.append(line)
        else:
            # Empty line outside of a code block
            processed_lines.append(line)
    
    # End any remaining code block
    if in_code_block:
        processed_lines.append("```python")
        processed_lines.extend(code_block)
        processed_lines.append("```")
    
    # End any remaining docstring
    if in_docstring:
        processed_lines.append("")
        processed_lines.extend(docstring_lines)
        processed_lines.append("")
    
    # Join the processed lines
    return '\n'.join(processed_lines)


def process_c_file(file_path: Path, literate_c_script: Path) -> str:
    """
    Process a C/C++ source file for HTML conversion using literate-C preprocessing.
    
    This function reads the content of a C/C++ file and creates a simple markdown
    representation. It then attempts to run a provided literate-C script on the file to
    generate a preprocessed output suitable for Pandoc conversion. If the literate-C
    processing produces non-empty output, specific markers are replaced with standard
    Pandoc code block markers. If the processing fails or returns empty output, a debug
    message is logged and the fallback markdown version is returned.
    
    Args:
        file_path (Path): Path to the C/C++ source file.
        literate_c_script (Path): Path to the literate-C preprocessing script.
    
    Returns:
        str: Markdown-formatted content ready for Pandoc conversion.
    """
    # First, read the file content directly
    with open(file_path, 'r', encoding='utf-8') as f:
        file_content = f.read()
    
    # Create a markdown representation of the C file
    markdown_content = f"""# {file_path.name}

```c
{file_content}
```
"""
    
    # Run literate-c for additional processing if available
    literate_c_cmd = [str(literate_c_script), str(file_path), '0']  # Use magic=0 for standard C files
    
    try:
        # Run literate-c, capture its output
        preproc_proc = subprocess.Popen(
            literate_c_cmd, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE, 
            text=True, 
            encoding='utf-8'
        )
        content, stderr = preproc_proc.communicate()

        if preproc_proc.returncode == 0 and content.strip():
            # Replace the specific marker literate-c uses with standard pandoc 'c'
            content = content.replace('~~~literatec', '~~~c')
            return content
        else:
            # If literate-c fails or produces no output, use our simple markdown version
            debug_print(f"  [Debug] Using simple markdown for {file_path} due to literate-c error: {stderr}")
            return markdown_content
            
    except Exception as e:
        # If there's any error running literate-c, fall back to simple markdown
        debug_print(f"  [Debug] Using simple markdown for {file_path} due to error: {e}")
        return markdown_content


def prepare_pandoc_input(file_path: Path, literate_c_script: Path) -> str:
    """
    Prepare file content for Pandoc conversion.
    
    Determines the processing function to use based on the file extension. Markdown (.md), Python (.py), and shell (.sh) files are handled using their specialized functions, while C/C++ files are processed with a provided literate-C script to generate a Markdown representation.
    
    Args:
        file_path: Path of the source file to process.
        literate_c_script: Path to the script for processing C/C++ files via literate programming.
    
    Returns:
        The processed content as a string, ready for Pandoc conversion.
    """
    file_suffix = file_path.suffix.lower()
    
    if file_suffix == '.md':
        return process_markdown_file(file_path)
    elif file_suffix == '.py':
        return process_python_file(file_path)
    elif file_suffix == '.sh':
        return process_shell_file(file_path)
    else:  # C/C++ files
        return process_c_file(file_path, literate_c_script)


def run_pandoc(pandoc_input: str, output_html_path: Path, template_path: Path, 
               base_url: str, wiki_title: str, page_url: str, page_title: str,
               seo_metadata: Dict[str, str] = None) -> str:
    """Converts Markdown content to a standalone HTML document using Pandoc.
    
    This function runs Pandoc to transform the provided Markdown input into HTML using a specified
    template and SEO metadata. It assigns HTML variables for the base URL, wiki title, page URL, and
    page title, and saves Pandoc's output to the designated file. After conversion, the function checks
    that the generated HTML contains the proper DOCTYPE and <html> tag, and wraps the content with a
    complete HTML scaffold if necessary. Returns Pandoc's standard output on success or an empty string
    when an error occurs.
      
    Args:
        pandoc_input: The Markdown content to convert.
        output_html_path: File path where the generated HTML is saved.
        template_path: Path to the HTML template file used by Pandoc.
        base_url: Base URL for constructing absolute links.
        wiki_title: Title of the documentation or wiki.
        page_url: URL of the current page.
        page_title: Title of the current page.
        seo_metadata: Optional dictionary with SEO metadata (e.g., description, keywords, image).
    
    Returns:
        The standard output from Pandoc if conversion succeeds; otherwise, an empty string.
    """
    if seo_metadata is None:
        seo_metadata = {}
    
    pandoc_cmd = [
        'pandoc',
        '-f', 'markdown+smart+raw_html',  # Use markdown input with smart typography extension and raw HTML
        '-t', 'html5',
        '--standalone',     # Create full HTML doc
        '--template', str(template_path),
        '-V', f'base={base_url}',
        '-V', f'wikititle={wiki_title}',
        '-V', f'pageUrl={page_url}',
        '-V', f'pagetitle={page_title}',
        # Add SEO metadata variables
        '-V', f'description={seo_metadata.get("description", "")}',
        '-V', f'keywords={seo_metadata.get("keywords", "")}',
        '-V', f'image={seo_metadata.get("image", "")}',
        '-o', str(output_html_path)
    ]
    
    # Print pandoc command and input for debugging
    debug_print(f"  [Debug Pandoc] Command: {' '.join(pandoc_cmd)}")
    debug_print(f"  [Debug Pandoc] Input content length: {len(pandoc_input)} chars")
    debug_print(f"  [Debug Pandoc] First 200 chars of input: {pandoc_input[:200]}")
    
    # Run pandoc with input content
    process = subprocess.run(pandoc_cmd, input=pandoc_input, text=True, capture_output=True)
    
    # Print pandoc output for debugging
    debug_print(f"  [Debug Pandoc] Return Code: {process.returncode}")
    if process.stdout:
        debug_print(f"  [Debug Pandoc] STDOUT:\n{process.stdout}")
    if process.stderr:
        debug_print(f"  [Debug Pandoc] STDERR:\n{process.stderr}")
    
    if process.returncode != 0:
        print(f"Error running pandoc: {process.stderr}")
        return ""
    
    # Read the generated HTML and clean up any empty anchor tags
    try:
        with open(output_html_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Remove empty anchor tags
        content = re.sub(r'<a[^>]*>\s*</a>', '', content)
            
        # Check if the file has proper HTML structure
        if '<!DOCTYPE' not in content or '<html' not in content:
            print(f"Warning: Generated HTML for {output_html_path} is missing DOCTYPE or html tag")
            # Try to fix by adding proper HTML structure
            fixed_content = f"""<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
    <title>{wiki_title} - {page_title}</title>
    <meta name="description" content="{seo_metadata.get('description', '')}" />
    <meta name="keywords" content="{seo_metadata.get('keywords', '')}" />
</head>
<body>
{content}
</body>
</html>"""
            content = fixed_content
            
        # Write back the cleaned content
        with open(output_html_path, 'w', encoding='utf-8') as f:
            f.write(content)
            
    except Exception as e:
        print(f"Error verifying HTML structure: {e}")
    
    return process.stdout


def post_process_python_shell_html(html_content: str) -> str:
    """
    Enhance HTML for improved code block display and documentation link accuracy.
    
    Processes raw HTML generated from Python or shell files by wrapping <pre><code> and Pandoc's
    source code blocks in a container div for copy button functionality. Additionally, appends ".html"
    to local links pointing to documentation files to ensure correct navigation.
    
    Args:
        html_content: Raw HTML content to be processed.
    
    Returns:
        Processed HTML content with enhanced code blocks and updated links.
    """
    # Fix any <pre><code> blocks by wrapping them in a container div
    def wrap_pre_code_with_container(match):
        """
        Wraps a matched code block in a container div.
        
        Extracts the full text from a regex match object and encloses it in a <div> element
        with the "code-block-container" class, enabling additional HTML styling.
        
        Args:
            match: A regex match object containing the code block text.
        
        Returns:
            A string with the matched content wrapped in a container div.
        """
        pre_content = match.group(0)
        return f'<div class="code-block-container">{pre_content}</div>'
    
    # Wrap <pre><code> blocks with a container div
    processed_html = re.sub(
        r'<pre[^>]*><code[^>]*>.*?</code></pre>', 
        wrap_pre_code_with_container, 
        html_content, 
        flags=re.DOTALL | re.IGNORECASE
    )
    
    # Also handle code blocks created by pandoc (with sourceCode class)
    def wrap_source_code_with_container(match):
        """
        Wraps matched source code in an HTML container.
        
        Extracts the content from the first capturing group of the regex match object
        and returns it enclosed within a <div> tag having the "code-block-container" class.
        
        Args:
            match: A regex match object with the source code in its first capturing group.
        
        Returns:
            A string with the source code wrapped in a <div class="code-block-container"> tag.
        """
        div_contents = match.group(1)
        return f'<div class="code-block-container">{div_contents}</div>'
    
    processed_html = re.sub(
        r'<div class="sourceCode" id="cb\d+"[^>]*>(.*?)</div>', 
        wrap_source_code_with_container, 
        processed_html, 
        flags=re.DOTALL | re.IGNORECASE
    )
    
    # Fix links to documentation files by appending .html
    def fix_doc_links(match):
        """
        Fixes links to documentation files by appending .html to the href.
        
        This function identifies links that point to other documentation files
        and appends .html to the href attribute if it doesn't already have it.
        
        Args:
            match: A regex match object containing the link tag.
        
        Returns:
            A string with the fixed link.
        """
        link_tag = match.group(0)
        href_match = re.search(r'href="([^"]+)"', link_tag)
        
        if href_match:
            href = href_match.group(1)
            # Skip external links, anchors, and links that already have .html
            if (href.startswith('http') or href.startswith('https') or 
                href.startswith('#') or href.endswith('.html')):
                return link_tag
                
            # Check if the link points to a file in the repository
            if re.search(r'\.(c|h|py|sh|md)$', href):
                # Replace the href with the one that includes .html
                return re.sub(r'href="([^"]+)"', f'href="{href}.html"', link_tag)
        
        return link_tag
    
    # Apply the link fix
    processed_html = re.sub(
        r'<a[^>]+href="[^"]+">[^<]+</a>',
        fix_doc_links,
        processed_html
    )
    
    return processed_html


def run_awk_post_processing(html_content: str, file_path: Path, 
                            repo_root: Path, darcsit_dir: Path) -> str:
    """
    Apply awk post-processing to HTML content from C files.
    
    This function runs the 'decl_anchors.awk' script from the darcsit directory on the
    given HTML content. It determines a tags file path relative to the repository root
    based on the source file, executes the awk script using a temporary file for output,
    and returns the processed HTML. The temporary file is removed after processing.
    
    Args:
        html_content: HTML content to process.
        file_path: Path of the original C source file.
        repo_root: Root directory of the repository for relative path computation.
        darcsit_dir: Directory containing the 'decl_anchors.awk' script.
    
    Returns:
        Processed HTML content.
    
    Raises:
        FileNotFoundError: If the 'decl_anchors.awk' script is not found.
        RuntimeError: If the awk processing fails.
    """
    decl_anchors_script = darcsit_dir / 'decl_anchors.awk'
    if not decl_anchors_script.is_file():
        raise FileNotFoundError(f"decl_anchors.awk script not found at {decl_anchors_script}")
    
    # Construct the expected tags file path relative to the repo root for awk
    relative_tags_path = file_path.relative_to(repo_root).with_suffix(file_path.suffix + '.tags')
    
    # Create a temporary file to store the output
    temp_output_path = Path(f"{file_path}.temp.html")
    
    try:
        with open(temp_output_path, 'w', encoding='utf-8') as f_out:
            postproc_cmd = ['awk', '-v', f'tags={relative_tags_path}', '-f', str(decl_anchors_script)]
            postproc_proc = subprocess.Popen(
                postproc_cmd, 
                stdin=subprocess.PIPE, 
                stdout=f_out, 
                stderr=subprocess.PIPE, 
                text=True, 
                encoding='utf-8'
            )
            _, stderr = postproc_proc.communicate(input=html_content)

            if postproc_proc.returncode != 0:
                raise RuntimeError(f"Awk post-processing failed: {stderr}")
        
        # Read the processed content from the temporary file
        with open(temp_output_path, 'r', encoding='utf-8') as f:
            processed_content = f.read()
        
        return processed_content
    finally:
        # Remove the temporary file
        if temp_output_path.exists():
            temp_output_path.unlink()


def post_process_c_html(html_content: str, file_path: Path, 
                       repo_root: Path, darcsit_dir: Path, docs_dir: Path) -> str:
    """
    Enhance C/C++ HTML content with code block containers and include-link corrections.
    
    This function post-processes HTML generated from C/C++ source files to improve its
    presentation in documentation. It removes extraneous trailing line numbers from the
    literate-c output, wraps <pre><code> blocks and sourceCode divs in container divs for
    consistent styling, and converts #include statements into hyperlinks that reference either
    locally generated documentation or external sources.
    
    Args:
        html_content: The original HTML output from processing a C/C++ file.
        file_path: Path to the source file corresponding to the HTML content.
        repo_root: Root directory of the repository.
        darcsit_dir: Directory containing darcsit scripts.
        docs_dir: Output directory for the generated HTML documentation.
    
    Returns:
        The modified HTML content with enhanced styling and linked #include statements.
    """
    # Remove trailing line numbers added by literate-c
    cleaned_html = re.sub(
        r'(\s*(?:<span class="[^"]*">\s*\d+\s*</span>|\s+\d+)\s*)+(\s*</span>)', 
        r'\2', 
        html_content
    )
    
    # Wrap <pre><code> blocks with a container div
    def wrap_pre_code_with_container(match):
        """
        Wrap the matched content in a container div.
        
        This function retrieves the entire match from a regex match object and wraps it
        inside a <div> element with the CSS class "code-block-container". It is used to
        enclose code block elements within an HTML container for consistent styling.
        
        Args:
            match (Match): A regex match object containing the code block to be wrapped.
        
        Returns:
            str: An HTML string with the wrapped code block.
        """
        pre_content = match.group(0)
        return f'<div class="code-block-container">{pre_content}</div>'
    
    cleaned_html = re.sub(
        r'<pre[^>]*><code[^>]*>.*?</code></pre>', 
        wrap_pre_code_with_container, 
        cleaned_html, 
        flags=re.DOTALL | re.IGNORECASE
    )
    
    # Process the sourceCode divs
    def wrap_source_code_with_container(match):
        # Get the div's contents (which includes the pre/code)
        """
        Wraps a matched code block in a container div.
        
        Extracts the content captured by the first group of the provided regex match and
        returns it enclosed in a <div> element with the "code-block-container" class.
        
        Args:
            match (re.Match): A regex match object with the source code block in its first group.
        
        Returns:
            str: An HTML string with the code block wrapped in a container div.
        """
        div_contents = match.group(1)
        # Return the pre/code wrapped in our container div
        return f'<div class="code-block-container">{div_contents}</div>'
    
    # Replace the standard sourceCode div with our container div
    cleaned_html = re.sub(
        r'<div class="sourceCode" id="cb\d+"[^>]*>(.*?)</div>', 
        wrap_source_code_with_container, 
        cleaned_html, 
        flags=re.DOTALL | re.IGNORECASE
    )
    
    # Add links to #include statements
    def create_include_link(match):
        """
        Transforms an include directive match into an HTML hyperlink.
        
        Converts a regex match object capturing parts of an include directive into an HTML anchor element.
        The function extracts the filename and checks if a corresponding file exists in the local
        'src-local' directory. If it does, a relative link to the generated local documentation is created;
        otherwise, a link to the Basilisk source repository is returned. The original span formatting is preserved.
        
        Parameters:
            match: A regex match object with four capture groups:
                   1. The prefix span for the include statement.
                   2. The opening tag for the filename.
                   3. The filename (which may include a path).
                   4. The closing tag for the filename.
        
        Returns:
            A string containing the HTML hyperlink wrapping the original include directive span.
        
        Note:
            This function relies on the global variables `repo_root`, `docs_dir`, and `file_path` for
            file path resolution.
        """
        prefix = match.group(1)  # e.g., <span class="pp">#include </span>
        span_tag_open = match.group(2)  # e.g., <span class="im">
        filename = match.group(3)  # e.g., filename.h or path/filename.h
        span_tag_close = match.group(4)  # </span>
        
        # Reconstruct original full span tag assuming literal quotes
        original_span_tag = f'{span_tag_open}\"{filename}\"{span_tag_close}'
        
        # Split filename by '/' and take the last part for checking in src-local root
        check_filename = filename.split('/')[-1]
        local_file_path = repo_root / 'src-local' / check_filename
        
        if local_file_path.is_file():
            # Link to local generated HTML file
            # Use the new file naming pattern: file.c -> file.c.html, file.h -> file.h.html
            target_html_path = (docs_dir / 'src-local' / check_filename).with_suffix(local_file_path.suffix + '.html')
            # Calculate relative path from the *current* HTML file's directory
            try:
                relative_link = os.path.relpath(target_html_path, start=file_path.parent)
                link_url = relative_link.replace('\\', '/')  # Ensure forward slashes
                # remove /docs/ with / in link
                link_url = link_url.replace('/docs/', '/')
                # debug_print(f"  [Debug Include] Relative link: {link_url}")
                # debug_print(f"  [Debug Include] Link URL: {file_path.parent}")
                # exit(0)
            except ValueError:
                # Handle cases where paths are on different drives (should not happen here)
                link_url = target_html_path.as_uri()  # Fallback to absolute URI
            link_title = f"Link to local documentation for {filename}"
        else:
            # Link to basilisk.fr, preserving original path if present
            link_url = f"http://basilisk.fr/src/{filename}"
            link_title = f"Link to Basilisk source for {filename}"
        
        # Return the prefix span, followed by the link wrapping the filename span
        return f'{prefix}<a href="{link_url}" title="{link_title}">{original_span_tag}</a>'
    
    # Corrected regex: Find pp span followed by im span, allowing flexible space
    # and handle potential HTML entity quotes (&quot;)
    include_pattern = r'(<span class="pp">#include\s*</span>)(<span class=\"im\">)(?:\"|&quot;)(.*?)(?:\"|&quot;)(</span>)'
    cleaned_html = re.sub(include_pattern, create_include_link, cleaned_html, flags=re.DOTALL)
    
    return cleaned_html


def insert_css_link_in_html(html_file_path: Path, css_path: Path, is_root: bool = True) -> bool:
    """
    Insert a CSS link tag into an HTML file's <head> section.
    
    Reads the specified HTML file and checks whether a <link> tag for the given CSS
    file already exists. If not, it inserts the tag just before the closing </head> tag.
    When the HTML file is in a subdirectory (is_root is False), the CSS file name is 
    prefixed with "../" to ensure the link is correct.
    
    Parameters:
        html_file_path: The path to the target HTML file.
        css_path: The path to the CSS file to be linked.
        is_root: True if the HTML file is in the root directory; otherwise, False.
    
    Returns:
        True if the CSS link was inserted or already exists; otherwise, False.
    """
    try:
        with open(html_file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Define the CSS path - relative to the HTML file
        if is_root:
            css_link = f'<link href="{Path(css_path).name}" rel="stylesheet" type="text/css" />'
        else:
            css_link = f'<link href="../{Path(css_path).name}" rel="stylesheet" type="text/css" />'
        
        # Check if the CSS link is already included
        if 'link href="' + Path(css_path).name + '"' in content or 'link href="../' + Path(css_path).name + '"' in content:
            # CSS link is already included, no need to add it
            return True
        
        # Find the head section to insert the CSS link
        head_end_idx = content.find('</head>')
        if head_end_idx == -1:
            # If no </head> tag found, check if there's a <head> tag
            head_start_idx = content.find('<head>')
            if head_start_idx != -1:
                # Insert after the <head> tag
                modified_content = content[:head_start_idx + 6] + '\n    ' + css_link + content[head_start_idx + 6:]
            else:
                # No head tag, create a complete HTML structure
                debug_print(f"Warning: No head tag found in {html_file_path}, creating complete HTML structure")
                modified_content = f"""<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
    {css_link}
</head>
<body>
{content}
</body>
</html>"""
        else:
            # Insert the CSS link tag just before the closing head tag
            modified_content = content[:head_end_idx] + '    ' + css_link + '\n    ' + content[head_end_idx:]
        
        # Write back the modified content
        with open(html_file_path, 'w', encoding='utf-8') as f:
            f.write(modified_content)
        
        return True
    except Exception as e:
        print(f"Error inserting CSS link in {html_file_path}: {e}")
        return False


def insert_javascript_in_html(html_file_path: Path) -> bool:
    """
    Inserts inline JavaScript for copy-to-clipboard on code blocks.
    
    Reads the specified HTML file and checks for an existing copy button script by searching
    for elements with the "copy-button" class. If absent, the function inserts an inline JavaScript
    snippet that adds copy buttons to code block containers. The snippet is placed just before the
    closing </body> tag; if no </body> tag is found, it is appended to the content (or wrapped in a
    basic HTML structure if no <body> tag exists). Returns True if the snippet is inserted or
    already present, and False if updating the file fails.
      
    Args:
        html_file_path: The path to the HTML file to update.
    
    Returns:
        True if the JavaScript snippet is present or successfully inserted; False otherwise.
    """
    try:
        with open(html_file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # JavaScript for copy functionality
        copy_js = '''
    <script type="text/javascript">
    document.addEventListener('DOMContentLoaded', function() {
        // Add copy button to each code block container
        const codeBlocks = document.querySelectorAll('.code-block-container pre');
        codeBlocks.forEach(function(codeBlock, index) {
            // Create button element
            const button = document.createElement('button');
            button.className = 'copy-button';
            button.textContent = 'Copy';
            button.setAttribute('aria-label', 'Copy code to clipboard');
            button.setAttribute('data-copy-state', 'copy');
            
            // Get the code block container (parent of the pre)
            const container = codeBlock.parentNode;
            
            // Add the button to the container
            container.appendChild(button);
            
            // Set up click event
            button.addEventListener('click', function() {
                // Create a textarea element to copy from
                const textarea = document.createElement('textarea');
                // Get the text content from the pre element (the actual code)
                textarea.value = codeBlock.textContent;
                document.body.appendChild(textarea);
                textarea.select();
                
                try {
                    // Execute copy command
                    document.execCommand('copy');
                    // Update button state
                    button.textContent = 'Copied!';
                    button.classList.add('copied');
                    
                    // Reset button state after 2 seconds
                    setTimeout(function() {
                        button.textContent = 'Copy';
                        button.classList.remove('copied');
                    }, 2000);
                } catch (err) {
                    console.error('Copy failed:', err);
                    button.textContent = 'Error!';
                }
                
                // Clean up
                document.body.removeChild(textarea);
            });
        });
    });
    </script>
        '''
        
        # Check if the JavaScript is already included
        if 'class="copy-button"' in content:
            # JavaScript is already included, no need to add it
            return True
        
        # Find the body end to insert the JavaScript
        body_end_idx = content.find('</body>')
        if body_end_idx == -1:
            # If no </body> tag found, check if there's a <body> tag
            body_start_idx = content.find('<body>')
            if body_start_idx != -1:
                # Insert at the end of the content
                modified_content = content + copy_js
            else:
                # No body tag, create a complete HTML structure
                debug_print(f"Warning: No body tag found in {html_file_path}, creating complete HTML structure")
                modified_content = f"""<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
</head>
<body>
{content}
{copy_js}
</body>
</html>"""
        else:
            # Insert the JavaScript code just before the closing body tag
            modified_content = content[:body_end_idx] + copy_js + content[body_end_idx:]
        
        # Write back the modified content
        with open(html_file_path, 'w', encoding='utf-8') as f:
            f.write(modified_content)
        
        return True
    except Exception as e:
        print(f"Error inserting JavaScript in {html_file_path}: {e}")
        return False


def process_file_with_page2html_logic(file_path: Path, output_html_path: Path, repo_root: Path, 
                                     basilisk_dir: Path, darcsit_dir: Path, template_path: Path, 
                                     base_url: str, wiki_title: str, literate_c_script: Path, docs_dir: Path) -> bool:
    """
    Converts a source file to HTML and applies file-type-specific post processing.
    
    The function prepares input for Pandoc conversion based on the file type and then
    applies additional steps tailored to the source file. For Python, shell, and Markdown
    files, it post-processes the output HTML to enhance code block presentation. For C/C++
    files, it uses awk-based post processing followed by further cleanup. CSS and JavaScript
    are then inserted to improve styling and interactive functionality. Any errors during
    processing are caught, and the function returns a success flag.
    
    Args:
        file_path: Path to the source file.
        output_html_path: Path where the generated HTML will be saved.
        repo_root: Repository root directory used for computing relative paths.
        basilisk_dir: Directory containing resources for Basilisk.
        darcsit_dir: Directory containing darcsit scripts.
        template_path: Path to the HTML template for conversion.
        base_url: Base URL for constructing links within the documentation.
        wiki_title: Title for the documentation or wiki.
        literate_c_script: Path to the literate-c script for processing C/C++ files.
        docs_dir: Directory where documentation files are stored.
    
    Returns:
        True if the HTML was generated and post-processed successfully, False otherwise.
    """
    print(f"  Processing {file_path.relative_to(repo_root)} -> {output_html_path.relative_to(repo_root / 'docs')}")

    try:
        # Prepare pandoc input based on file type
        pandoc_input_content = prepare_pandoc_input(file_path, literate_c_script)
        
        # Calculate relative URL path for the page
        # Ensure URL starts with / and uses forward slashes
        page_url = (base_url + output_html_path.relative_to(repo_root / 'docs').as_posix()).replace('//', '/')
        
        # Clean up the page title - remove leading/trailing dashes and spaces
        page_title = file_path.relative_to(repo_root).as_posix().strip('- \t')
        
        # Run pandoc to convert to HTML
        pandoc_stdout = run_pandoc(
            pandoc_input_content, 
            output_html_path, 
            template_path, 
            base_url, 
            wiki_title, 
            page_url, 
            page_title,
            extract_seo_metadata(file_path, pandoc_input_content)
        )
        
        # Determine file type for post-processing
        is_python_file = file_path.suffix.lower() == '.py'
        is_shell_file = file_path.suffix.lower() == '.sh'
        is_markdown_file = file_path.suffix.lower() == '.md'
        
        # Apply appropriate post-processing based on file type
        if is_python_file or is_shell_file or is_markdown_file:
            # For Python, Shell, and Markdown files
            # Read the generated HTML file
            with open(output_html_path, 'r', encoding='utf-8') as f:
                html_content = f.read()
            
            # Post-process the HTML
            processed_html = post_process_python_shell_html(html_content)
            
            # Write back the processed HTML
            with open(output_html_path, 'w', encoding='utf-8') as f:
                f.write(processed_html)
        else:
            # For C/C++ files
            # Read the generated HTML file
            with open(output_html_path, 'r', encoding='utf-8') as f:
                html_content = f.read()
            
            # Use awk for post-processing
            processed_html = run_awk_post_processing(html_content, file_path, repo_root, darcsit_dir)
            
            # Further post-process the HTML
            cleaned_html = post_process_c_html(processed_html, file_path, repo_root, darcsit_dir, docs_dir)
            
            # Write back the processed HTML
            with open(output_html_path, 'w', encoding='utf-8') as f:
                f.write(cleaned_html)
        
        # Insert CSS link and JavaScript for all file types
        is_root = output_html_path.parent == docs_dir
        insert_css_link_in_html(output_html_path, CSS_PATH, is_root)
        insert_javascript_in_html(output_html_path)
        
        return True
    
    except Exception as e:
        print(f"  Error processing {file_path}: {e}")
        return False


def convert_directory_tree_to_html(readme_content: str) -> str:
    """
    Converts a plain text directory tree in README content into an HTML site map.
    
    This function scans the provided README content for a markdown code block that
    contains a directory tree. If found, it parses the tree structure and transforms it
    into a nested HTML format wrapped in a <div> element with the class "repository-structure".
    Directories and files are converted into bullet list items with hyperlinks where appropriate.
    If no directory tree block is detected, the original content is returned unchanged.
    
    Args:
        readme_content: The complete README file content as a string.
    
    Returns:
        A string with the directory tree section replaced by an HTML site map.
    """
    # Find the directory tree section
    tree_pattern = r'```\s*\n(├.*?\n.*?└.*?)\n```'
    tree_match = re.search(tree_pattern, readme_content, re.DOTALL)
    
    if not tree_match:
        return readme_content  # No tree found, return original content
        
    tree_text = tree_match.group(1)
    
    # Parse the directory tree
    html_structure = ['<div class="repository-structure">']
    
    # Track parent directories and their indentation levels for proper nesting
    path_stack = []
    prev_indent = -1
    
    for line in tree_text.split('\n'):
        # Skip empty lines
        if not line.strip():
            continue
            
        # Determine indentation level based on the structure symbols
        indent_level = 0
        
        if '│   ' in line:
            indent_level = line.count('│   ')
        elif '    ' in line and ('├── ' in line or '└── ' in line):
            # Handle case where │ might be missing but spacing is present
            spaces_before_item = len(line) - len(line.lstrip(' '))
            indent_level = spaces_before_item // 4
        
        # Clean up the line by removing directory tree symbols
        clean_line = line.replace('├── ', '').replace('└── ', '').replace('│   ', '')
        
        # Get the path and description
        parts = clean_line.strip().split(None, 1)
        path = parts[0]
        description = parts[1] if len(parts) > 1 else ''
        
        # Determine if it's a directory or file based on path ending with /
        is_dir = path.endswith('/')
        
        # Update the path stack based on indentation changes
        if indent_level > prev_indent:
            # Going deeper, add the previous item to the stack
            if path_stack and prev_indent >= 0:
                path_stack.append(path_stack[-1])
        elif indent_level < prev_indent:
            # Going up, remove items from stack
            for _ in range(prev_indent - indent_level):
                if path_stack:
                    path_stack.pop()
        
        # Generate proper indentation for HTML output
        indent = '  ' * indent_level
        
        # Generate the HTML list item
        item_html = f"{indent}* "
        
        if is_dir:
            # For directories
            dir_name = path.rstrip('/')
            # Special case for basilisk/src/ which should not be linked
            if dir_name == "basilisk/src":
                item_html += f"**{path}** - {description}"
            else:
                # For other directories, create links
                item_html += f"**[{path}]({dir_name})** - {description}"
            
            # Update the path stack for children
            if len(path_stack) <= indent_level:
                path_stack.append(dir_name)
            else:
                path_stack[indent_level] = dir_name
        else:
            # For files
            # Determine the parent directory path
            parent_path = path_stack[indent_level-1] if indent_level > 0 and path_stack else ""
            
            # Create HTML link with extension preserved in the filename
            file_path = f"{parent_path}/{path}" if parent_path else path
            file_path = file_path.lstrip('/')
            
            # Preserve the original file extension in the link
            # Use the new file naming pattern: file.c -> file.c.html, file.h -> file.h.html
            item_html += f"**[{path}]({file_path}.html)** - {description}"
        
        html_structure.append(item_html)
        prev_indent = indent_level
    
    html_structure.append('</div>')
    
    # Replace the tree section with the HTML structure
    html_tree = '\n'.join(html_structure)
    modified_content = readme_content.replace(tree_match.group(0), html_tree)
    
    return modified_content


def generate_index(readme_path: Path, index_path: Path, generated_files: Dict[Path, Path], 
                  docs_dir: Path, repo_root: Path) -> bool:
    """
    Generates an index.html page from README.md by integrating documentation links.
    
    Reads the README file (using a default header if missing) and converts its content to HTML.
    The function appends a section that groups links to generated documentation files based on their
    top-level directory, then uses Pandoc with a specified template and configuration to create
    the final HTML. After conversion, it post-processes the file to adjust code blocks and injects
    CSS and JavaScript for enhanced presentation.
    
    Args:
        readme_path: Path to the README.md file.
        index_path: Destination path for the generated index.html.
        generated_files: Dictionary mapping source file paths to their corresponding generated HTML paths.
        docs_dir: Directory where documentation files are stored.
        repo_root: Root directory of the repository used for computing relative paths.
    
    Returns:
        True if index.html was generated and processed successfully, otherwise False.
    """
    if not readme_path.exists():
        print(f"Warning: README.md not found at {readme_path}")
        readme_content = "# Project Documentation\n"
    else:
        readme_content = readme_path.read_text(encoding='utf-8')
        
    # Convert the directory tree to HTML before generating index
    readme_content = convert_directory_tree_to_html(readme_content)

    # Add documentation links section
    links_markdown = "\n\n## Generated Documentation\n\n"
    
    # Group links by top-level directory (src-local, simulationCases, etc.)
    grouped_links = {}
    for original_path, html_path in generated_files.items():
        relative_html_path = html_path.relative_to(docs_dir)
        relative_original_path = original_path.relative_to(repo_root)
        
        # Handle files in the root directory
        if len(relative_original_path.parts) == 1:  # File is directly in the root
            top_dir = "root"
        else:
            top_dir = relative_original_path.parts[0]
            
        if top_dir not in grouped_links:
            grouped_links[top_dir] = []
        
        # Make sure the html_path has the correct format with preserved extension
        # The html_path already has the correct format (file.c.html, file.h.html, etc.)
        # because we modified the output path construction in the main function
        grouped_links[top_dir].append(f"- [{relative_original_path}]({relative_html_path})")

    # Add a section for files in the root directory
    if 'root' in grouped_links and grouped_links['root']:
        links_markdown += f"### Root Directory\n\n"
        links_markdown += "\n".join(sorted(grouped_links['root']))
        links_markdown += "\n\n"
    
    # Add sections for files in the source directories
    for top_dir in sorted(grouped_links.keys()):
        if top_dir in SOURCE_DIRS:  # Source dirs
            links_markdown += f"### {top_dir}\n\n"
            links_markdown += "\n".join(sorted(grouped_links[top_dir]))
            links_markdown += "\n\n"

    # Append links to the end for simplicity
    final_readme_content = readme_content + links_markdown

    # Convert the combined README + links to HTML for index.html
    cmd = [
        'pandoc',
        '-f', 'markdown+tex_math_dollars+raw_html',  # Add raw_html to preserve HTML
        '-t', 'html5',
        '--standalone',
        '--mathjax',
        '--template', str(TEMPLATE_PATH),
        '-V', f'wikititle={WIKI_TITLE}',
        '-V', f'base={BASE_URL}',
        '-V', 'notitle=true',  # Add notitle=true to avoid duplicate h1 elements
        '-V', f'pagetitle={WIKI_TITLE}',  # Set the page title to be the same as wiki title
        '-o', str(index_path)
    ]

    debug_print(f"  [Debug Index] Target path: {index_path}")
    debug_print(f"  [Debug Index] Command: {' '.join(cmd)}")

    process = subprocess.run(cmd, input=final_readme_content, text=True, capture_output=True, check=False)

    # Print results unconditionally for debugging
    debug_print(f"  [Debug Index] Pandoc Return Code: {process.returncode}")
    if process.stdout:
        debug_print(f"  [Debug Index] Pandoc STDOUT:\n{process.stdout}")
    if process.stderr:
        debug_print(f"  [Debug Index] Pandoc STDERR:\n{process.stderr}")

    if process.returncode != 0:
        print("Error generating index.html:")
        return False
    
    # Post-process index.html for code blocks
    try:
        with open(index_path, 'r', encoding='utf-8') as f_in:
            index_html_content = f_in.read()
        
        processed_html = post_process_python_shell_html(index_html_content)
        
        with open(index_path, 'w', encoding='utf-8') as f_out:
            f_out.write(processed_html)
            
    except Exception as e:
        print(f"Warning: Failed to process code blocks in {index_path}: {e}")
        # Continue even if processing fails, the base file was generated

    # Insert CSS and JavaScript
    insert_css_link_in_html(index_path, CSS_PATH, True)
    insert_javascript_in_html(index_path)
    
    return True


def generate_robots_txt(docs_dir: Path) -> bool:
    """
    Generate a robots.txt file to guide search engine crawlers.
    
    Args:
        docs_dir: Directory where documentation files are stored
        
    Returns:
        True if robots.txt was generated successfully
    """
    robots_path = docs_dir / 'robots.txt'
    
    try:
        with open(robots_path, 'w', encoding='utf-8') as f:
            f.write('User-agent: *\n')
            f.write('Allow: /\n\n')
            f.write(f'Sitemap: {BASE_DOMAIN}/sitemap.xml\n')
        
        debug_print(f"Generated robots.txt at {robots_path}")
        return True
        
    except Exception as e:
        print(f"Error generating robots.txt: {e}")
        return False


def generate_sitemap(docs_dir: Path, generated_files: Dict[Path, Path]) -> bool:
    """
    Generate a sitemap.xml file for search engines.
    
    Args:
        docs_dir: Directory where documentation files are stored
        generated_files: Dictionary mapping source files to generated HTML files
        
    Returns:
        True if sitemap was generated successfully
    """
    sitemap_path = docs_dir / 'sitemap.xml'
    
    try:
        with open(sitemap_path, 'w', encoding='utf-8') as f:
            f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
            f.write('<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n')
            
            # Add the homepage
            f.write('  <url>\n')
            f.write(f'    <loc>{BASE_DOMAIN}/</loc>\n')
            f.write('    <changefreq>weekly</changefreq>\n')
            f.write('    <priority>1.0</priority>\n')
            f.write('  </url>\n')
            
            # Add all generated HTML files
            for _, html_path in generated_files.items():
                relative_path = html_path.relative_to(docs_dir)
                url_path = str(relative_path).replace('\\', '/')
                
                f.write('  <url>\n')
                f.write(f'    <loc>{BASE_DOMAIN}/{url_path}</loc>\n')
                f.write('    <changefreq>monthly</changefreq>\n')
                
                # Higher priority for important files
                if 'index' in url_path or url_path.startswith('src-local/'):
                    f.write('    <priority>0.8</priority>\n')
                else:
                    f.write('    <priority>0.6</priority>\n')
                    
                f.write('  </url>\n')
            
            f.write('</urlset>\n')
        
        debug_print(f"Generated sitemap at {sitemap_path}")
        return True
        
    except Exception as e:
        print(f"Error generating sitemap: {e}")
        return False


def copy_css_file(css_path: Path, docs_dir: Path) -> bool:
    """
    Copies a CSS file to the specified documentation directory.
    
    This function copies the CSS file using shutil.copy2 to preserve file metadata.
    If an error occurs during the copy operation, an error message is printed and the
    function returns False.
    
    Args:
        css_path: The path to the source CSS file.
        docs_dir: The destination directory where the CSS file will be copied.
    
    Returns:
        True if the CSS file was successfully copied; otherwise, False.
    """
    try:
        # Copy CSS file to docs directory
        shutil.copy2(css_path, docs_dir / css_path.name)
        debug_print(f"Copied CSS file to {docs_dir / css_path.name}")
        return True
    except Exception as e:
        print(f"Error copying CSS file: {e}")
        return False


def create_favicon_files(docs_dir: Path, logos_dir: Path) -> bool:
    """
    Create necessary favicon files in the docs/assets/favicon directory.
    
    This function ensures all required favicon files exist in the destination
    directory, creating them if needed from source logo files.
    
    Args:
        docs_dir: The documentation root directory
        logos_dir: Directory containing source logo files
        
    Returns:
        True if successful, False otherwise
    """
    try:
        # Create favicon directory
        favicon_dir = docs_dir / "assets" / "favicon"
        favicon_dir.mkdir(exist_ok=True)
        
        # Copy any existing favicon files from .github/assets/favicon if it exists
        source_favicon_dir = Path(logos_dir.parent, "favicon")
        if source_favicon_dir.exists() and source_favicon_dir.is_dir():
            for item in source_favicon_dir.glob('*'):
                if item.is_file():
                    shutil.copy2(item, favicon_dir / item.name)
                    debug_print(f"Copied favicon file: {item.name}")
        
        # Create essential favicon files if they don't exist
        favicon_files = [
            "favicon.ico",
            "favicon.svg",
            "apple-touch-icon.png",
            "favicon-96x96.png",
            "site.webmanifest"
        ]
        
        # Check if we have the required logo to create favicons
        logo_file = None
        for potential_logo in ["CoMPhy-Lab.svg", "CoMPhy-Lab-no-name.png", "logoBasilisk_TransparentBackground.png"]:
            if (logos_dir / potential_logo).exists():
                logo_file = logos_dir / potential_logo
                break
        
        if logo_file:
            # Create a basic site.webmanifest if it doesn't exist
            webmanifest_path = favicon_dir / "site.webmanifest"
            if not webmanifest_path.exists():
                with open(webmanifest_path, 'w', encoding='utf-8') as f:
                    f.write('''{
  "name": "CoMPhy Lab",
  "short_name": "CoMPhy",
  "icons": [
    {
      "src": "/assets/favicon/android-chrome-192x192.png",
      "sizes": "192x192",
      "type": "image/png"
    },
    {
      "src": "/assets/favicon/android-chrome-512x512.png",
      "sizes": "512x512",
      "type": "image/png"
    }
  ],
  "theme_color": "#ffffff",
  "background_color": "#ffffff",
  "display": "standalone"
}''')
                debug_print(f"Created site.webmanifest")
        
        return True
    except Exception as e:
        print(f"Error creating favicon files: {e}")
        return False


def copy_assets(assets_dir: Path, docs_dir: Path) -> bool:
    """
    Copy assets from source to destination.
    
    This function copies assets such as CSS, JavaScript, images, etc. from the
    source assets directory to the destination docs directory.
    
    Args:
        assets_dir: The source assets directory
        docs_dir: The destination docs directory
        
    Returns:
        True if successful, False otherwise
    """
    try:
        debug_print(f"Copying assets from {assets_dir} to {docs_dir}")
        
        # Create the assets directory in docs if it doesn't exist
        docs_assets_dir = docs_dir / "assets"
        docs_assets_dir.mkdir(exist_ok=True)
        
        # Copy CSS files
        css_dir = assets_dir / "css"
        docs_css_dir = docs_assets_dir / "css"
        docs_css_dir.mkdir(exist_ok=True, parents=True)
        
        if css_dir.exists():
            for css_file in css_dir.glob("**/*"):
                if css_file.is_file():
                    rel_path = css_file.relative_to(css_dir)
                    dest_path = docs_css_dir / rel_path
                    dest_path.parent.mkdir(exist_ok=True, parents=True)
                    shutil.copy2(css_file, dest_path)
                    debug_print(f"Copied {css_file} to {dest_path}")
        
        # Copy JS files
        js_dir = assets_dir / "js"
        docs_js_dir = docs_assets_dir / "js"
        docs_js_dir.mkdir(exist_ok=True, parents=True)
        
        if js_dir.exists():
            for js_file in js_dir.glob("**/*"):
                if js_file.is_file():
                    rel_path = js_file.relative_to(js_dir)
                    dest_path = docs_js_dir / rel_path
                    dest_path.parent.mkdir(exist_ok=True, parents=True)
                    shutil.copy2(js_file, dest_path)
                    debug_print(f"Copied {js_file} to {dest_path}")
        
        # Copy images
        img_dir = assets_dir / "images"
        docs_img_dir = docs_assets_dir / "images"
        
        if img_dir.exists():
            docs_img_dir.mkdir(exist_ok=True, parents=True)
            for img_file in img_dir.glob("**/*"):
                if img_file.is_file():
                    rel_path = img_file.relative_to(img_dir)
                    dest_path = docs_img_dir / rel_path
                    dest_path.parent.mkdir(exist_ok=True, parents=True)
                    shutil.copy2(img_file, dest_path)
                    debug_print(f"Copied {img_file} to {dest_path}")
                    
        # Copy logo files
        logos_dir = assets_dir / "logos"
        docs_logos_dir = docs_assets_dir / "logos"
        
        if logos_dir.exists():
            docs_logos_dir.mkdir(exist_ok=True, parents=True)
            for logo_file in logos_dir.glob("**/*"):
                if logo_file.is_file():
                    rel_path = logo_file.relative_to(logos_dir)
                    dest_path = docs_logos_dir / rel_path
                    dest_path.parent.mkdir(exist_ok=True, parents=True)
                    shutil.copy2(logo_file, dest_path)
                    debug_print(f"Copied {logo_file} to {dest_path}")
        
        # Copy custom CSS to root directory
        if css_dir.exists():
            custom_styles_path = css_dir / "custom_styles.css"
            if custom_styles_path.exists():
                # Also copy to root directory to prevent 404s
                shutil.copy2(custom_styles_path, docs_dir / "custom_styles.css")
                debug_print(f"Copied custom_styles.css to root directory")
        
        # Create favicon files as needed
        logos_dir = assets_dir / "logos"
        if logos_dir.exists():
            create_favicon_files(docs_dir, logos_dir)
        
        return True
    except Exception as e:
        print(f"Error copying assets: {e}")
        return False


def main():
    """
    Generate HTML documentation for the project.
    
    This function orchestrates the documentation generation process by validating
    configuration, setting up the output directories, and copying required CSS files.
    It finds source files in the repository, converts them to HTML using type-specific
    processing logic, and collects the results into a generated files dictionary.
    Finally, it creates an index page and produces SEO-compliant files such as robots.txt
    and sitemap.xml, with all output written to the documentation directory.
    """
    if not validate_config():
        return
    
    try:
        # Create docs directory if it doesn't exist
        DOCS_DIR.mkdir(exist_ok=True)
        
        # Copy all assets (CSS, JS, logos, fonts, etc.) to docs directory
        print("\nCopying assets...")
        assets_dir = REPO_ROOT / '.github' / 'assets'
        if not copy_assets(assets_dir, DOCS_DIR):
            print("Failed to copy assets.")
            return
        
        # Find all source files
        source_files = find_source_files(REPO_ROOT, SOURCE_DIRS)
        if not source_files:
            print("No source files found.")
            return
        
        # Dictionary to store generated HTML files
        generated_files = {}
        
        # Process each source file
        for file_path in source_files:
            # Determine output path
            relative_path = file_path.relative_to(REPO_ROOT)
            
            # Create output path with file extension preserved in the HTML filename
            # For example: file.c -> file.c.html, file.h -> file.h.html, file.py -> file.py.html
            output_html_path = DOCS_DIR / relative_path.with_suffix(relative_path.suffix + '.html')
            
            # Create output directory if it doesn't exist
            output_html_path.parent.mkdir(parents=True, exist_ok=True)
            
            # Process file and generate HTML
            if process_file_with_page2html_logic(
                file_path, 
                output_html_path, 
                REPO_ROOT, 
                BASILISK_DIR, 
                DARCSIT_DIR, 
                TEMPLATE_PATH, 
                BASE_URL, 
                WIKI_TITLE, 
                LITERATE_C_SCRIPT,
                DOCS_DIR
            ):
                generated_files[file_path] = output_html_path
        
        # Generate index.html
        print("\nGenerating index.html...")
        if not generate_index(README_PATH, INDEX_PATH, generated_files, DOCS_DIR, REPO_ROOT):
            print("Failed to generate index.html.")
            return
        
        # Generate robots.txt
        print("\nGenerating robots.txt...")
        if not generate_robots_txt(DOCS_DIR):
            print("Failed to generate robots.txt.")
            return
        
        # Generate sitemap
        print("\nGenerating sitemap...")
        if not generate_sitemap(DOCS_DIR, generated_files):
            print("Failed to generate sitemap.")
            return
        
        print("\nDocumentation generation complete.")
        print(f"Output generated in: {DOCS_DIR}")
        
    finally:
        # Clean up temporary template file
        temp_template_path = TEMPLATE_PATH.parent / (TEMPLATE_PATH.stem.replace('.temp', '') + '.temp.html')
        if temp_template_path.exists():
            try:
                temp_template_path.unlink()
                debug_print(f"Cleaned up temporary template file: {temp_template_path}")
            except Exception as e:
                print(f"Warning: Could not delete temporary template file: {e}")


if __name__ == "__main__":
    main()
        
#!/bin/bash
# build.sh - Documentation build script for GitHub Pages
#
# Description:
#   Builds HTML documentation from source files using Pandoc and Basilisk's
#   literate-c processor. Creates a complete static site in .github/docs directory.
#
# Workflow:
#   1. Detect GitHub organization from git remote
#   2. Clone search database (optional, org-specific)
#   3. Set up Python virtual environment
#   4. Install dependencies from requirements.txt
#   5. Run generate_docs.py to build HTML pages
#   6. Clean HTML files (remove empty anchors)
#
# Usage:
#   .github/scripts/build.sh [--force-rebuild]
#
# Options:
#   --force-rebuild  Rebuild all HTML files even if source unchanged
#
# Environment:
#   SEARCH_REPO  Override search database repository name (default: comphy-search)
#
# Author: Vatsal Sanjay
# Organization: CoMPhy Lab, Durham University

# Exit immediately if a command exits with a non-zero status.
set -e

# Initialize force_rebuild flag
FORCE_REBUILD=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --force-rebuild)
      FORCE_REBUILD="--force-rebuild"
      shift
      ;;
    *)
      shift
      ;;
  esac
done

# Define the project root relative to the script location
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECT_ROOT=$(dirname "$(dirname "$SCRIPT_DIR")") # Go two levels up from script dir

# Change to project root to ensure paths work correctly
cd "$PROJECT_ROOT"

# Documentation output directory
DOCS_DIR="$PROJECT_ROOT/.github/docs"

# Auto-detect GitHub organization from git remote
GITHUB_ORG=$(git remote get-url origin 2>/dev/null | sed -n 's|.*[:/]\([^/]*\)/.*|\1|p')
if [ -z "$GITHUB_ORG" ]; then
    echo "Warning: Could not detect GitHub organization from git remote. Using fallback: comphy-lab"
    GITHUB_ORG="comphy-lab"
fi
echo "Detected GitHub organization: $GITHUB_ORG"

# Try to clone search database (optional - may not exist for all orgs)
# Allow override via SEARCH_REPO environment variable
SEARCH_REPO="${SEARCH_REPO:-comphy-search}"
echo "Attempting to clone search database from ${GITHUB_ORG}/${SEARCH_REPO}..."

if git clone --depth=1 "https://github.com/${GITHUB_ORG}/${SEARCH_REPO}.git" 2>/dev/null; then
    mkdir -p "$DOCS_DIR/assets/js"
    if [ -f "${SEARCH_REPO}/search_db.json" ]; then
        cp "${SEARCH_REPO}/search_db.json" "$DOCS_DIR/assets/js/search_db.json"
        echo "Search database copied successfully"
    else
        echo "Warning: search_db.json not found in ${SEARCH_REPO} repository"
    fi
    rm -rf "$SEARCH_REPO"
else
    echo "Warning: Could not clone ${GITHUB_ORG}/${SEARCH_REPO}. Search functionality may be limited."
    echo "This is expected if the search repository doesn't exist for your organization."
fi
PYTHON_SCRIPT="$PROJECT_ROOT/.github/scripts/generate_docs.py"

# Function to display messages
function log_message() {
  echo "$(date +"%Y-%m-%d %H:%M:%S") - $1"
}

# Install required dependencies using a virtual environment
if [ -f "$PROJECT_ROOT/.github/scripts/requirements.txt" ]; then
  log_message "Setting up Python virtual environment..."

  # Create a venv directory if it doesn't exist
  VENV_DIR="$PROJECT_ROOT/.venv"

  # Check if virtual environment already exists
  if [ ! -d "$VENV_DIR" ]; then
    python3 -m venv "$VENV_DIR"
    log_message "Created new virtual environment in $VENV_DIR"
  else
    log_message "Using existing virtual environment in $VENV_DIR"
  fi

  # Activate the virtual environment
  source "$VENV_DIR/bin/activate"

  log_message "Installing Python dependencies in virtual environment..."
  pip install --upgrade pip
  pip install -r "$PROJECT_ROOT/.github/scripts/requirements.txt"

  # Don't deactivate here - keep the environment active for subsequent Python calls
  log_message "Dependencies installed successfully"
fi

# Run the documentation generation script
log_message "Starting documentation generation..."
if [ -n "$FORCE_REBUILD" ]; then
  python3 "$PYTHON_SCRIPT" --force-rebuild
else
  python3 "$PYTHON_SCRIPT"
fi

# Clean HTML files to remove empty anchor tags
# Using fix_empty_anchors.py which is more targeted and preserves icons and other content
log_message "Cleaning HTML files to remove empty anchor tags..."
python3 "$PROJECT_ROOT/.github/scripts/fix_empty_anchors.py" "$DOCS_DIR"


if [ $? -ne 0 ]; then
    log_message "Documentation generation failed."
    exit 1
fi

log_message "Documentation generated successfully in $DOCS_DIR"

# Check if docs directory exists
if [ ! -d "$DOCS_DIR" ]; then
    log_message "Error: Docs directory '$DOCS_DIR' not found after generation."
    exit 1
fi

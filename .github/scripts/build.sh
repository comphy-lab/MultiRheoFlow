#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Define the project root relative to the script location
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECT_ROOT=$(dirname $(dirname "$SCRIPT_DIR")) # Go two levels up from script dir
DOCS_DIR="$PROJECT_ROOT/docs"
PYTHON_SCRIPT="$PROJECT_ROOT/.github/scripts/generate_docs.py"

echo "Running documentation generation script..."
python3 "$PYTHON_SCRIPT"

if [ $? -ne 0 ]; then
    echo "Documentation generation failed."
    exit 1
fi

echo "Documentation generated successfully in $DOCS_DIR"

# Check if docs directory exists
if [ ! -d "$DOCS_DIR" ]; then
    echo "Error: Docs directory '$DOCS_DIR' not found after generation."
    exit 1
fi

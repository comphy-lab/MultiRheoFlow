#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Define the project root relative to the script location
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECT_ROOT=$(dirname $(dirname "$SCRIPT_DIR")) # Go two levels up from script dir
DOCS_DIR="$PROJECT_ROOT/docs"

# Check if docs directory exists
if [ ! -d "$DOCS_DIR" ]; then
    echo "Error: Docs directory '$DOCS_DIR' not found after generation."
    exit 1
fi


# Change to the docs directory
cd "$DOCS_DIR"

echo "Starting local web server in $PWD on port 8000..."
echo "Access the site at http://localhost:8000 or http://127.0.0.1:8000"
echo "Press Ctrl+C to stop the server."

# Start the server in the foreground
python3 -m http.server 8000
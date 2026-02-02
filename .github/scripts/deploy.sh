#!/bin/bash
# deploy.sh - Local development server for documentation preview
#
# Description:
#   Starts a Python HTTP server to preview generated documentation locally.
#   Automatically finds an available port in the range 8000-8010.
#
# Usage:
#   .github/scripts/deploy.sh
#
# Prerequisites:
#   - Run build.sh first to generate .github/docs directory
#   - Python 3 must be installed
#
# Notes:
#   - Press Ctrl+C to stop the server
#   - Access at http://localhost:<port> shown in output
#
# Author: Vatsal Sanjay
# Organization: CoMPhy Lab, Durham University

# Exit immediately if a command exits with a non-zero status.
set -e

# Define the project root relative to the script location
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECT_ROOT=$(dirname "$(dirname "$SCRIPT_DIR")") # Go two levels up from script dir
DOCS_DIR="$PROJECT_ROOT/.github/docs"

# Check if docs directory exists
if [ ! -d "$DOCS_DIR" ]; then
    echo "Error: Docs directory '$DOCS_DIR' not found after generation."
    exit 1
fi

# Always change to the project root directory to ensure all paths work correctly
cd "$DOCS_DIR"

# Try ports 8000-8010 and use the first available one
START_PORT=8000
END_PORT=8010
PORT=$START_PORT

# Start the local web server in the docs directory
while [ $PORT -le $END_PORT ]; do
    # Check if the port is free
    if ! lsof -i :$PORT &> /dev/null; then
        echo "Starting local web server in $DOCS_DIR on port $PORT..."
        echo "Access the site at http://localhost:$PORT or http://127.0.0.1:$PORT"
        echo "Press Ctrl+C to stop the server."
        (cd "$DOCS_DIR" && python3 -m http.server $PORT)
        exit 0
    fi
    PORT=$((PORT+1))
done

echo "Error: All ports from $START_PORT to $END_PORT are busy."
exit 1

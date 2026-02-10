#!/bin/bash

set -euo pipefail

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Usage: $0 case_name [params_file]"
  exit 1
fi

CASE_NAME="$1"
CASE_PATH="simulationCases/${CASE_NAME}.c"

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)

if [ "$#" -eq 2 ]; then
  "${REPO_ROOT}/runSimulation.sh" --case "$CASE_PATH" --input "$2"
else
  "${REPO_ROOT}/runSimulation.sh" --case "$CASE_PATH"
fi

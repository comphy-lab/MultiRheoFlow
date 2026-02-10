#!/bin/bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: runSimulation.sh [--case simulationCases/SOME_CASE.c] [--input file.params]

Options:
  --case    Path to case source file (.c), optional.
  --input   Path to params file, optional.
  --help    Show this help.

Notes:
  - Defaults:
    --case  -> simulationCases/dropImpact.c
    --input -> default-VE.params
  - If --input is not provided, defaults are used for:
    dropImpact -> default-VE.params
    dropImpact-EVP -> default-EVP.params
    dropImpact-EVP-HB -> default-EVP-HB.params
  - Default params are expected at repository root.
EOF
}

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT="$SCRIPT_DIR"

resolve_file() {
  local candidate="$1"
  if [ -f "$candidate" ]; then
    echo "$candidate"
  elif [ -f "${REPO_ROOT}/${candidate}" ]; then
    echo "${REPO_ROOT}/${candidate}"
  else
    echo ""
  fi
}

default_params_for_case() {
  case "$1" in
    dropImpact)
      echo "default-VE.params"
      ;;
    dropImpact-EVP)
      echo "default-EVP.params"
      ;;
    dropImpact-EVP-HB)
      echo "default-EVP-HB.params"
      ;;
    *)
      echo ""
      ;;
  esac
}

CASE_ARG="simulationCases/dropImpact.c"
INPUT_ARG=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --case)
      if [ "$#" -lt 2 ]; then
        echo "Missing value for --case"
        usage
        exit 1
      fi
      CASE_ARG="$2"
      shift 2
      ;;
    --input)
      if [ "$#" -lt 2 ]; then
        echo "Missing value for --input"
        usage
        exit 1
      fi
      INPUT_ARG="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      usage
      exit 1
      ;;
  esac
done

CASE_FILE=$(resolve_file "$CASE_ARG")
if [ -z "$CASE_FILE" ]; then
  echo "Case source not found: ${CASE_ARG}"
  exit 1
fi

CASE_DIR=$(cd "$(dirname "$CASE_FILE")" && pwd)
CASE_FILE_NAME=$(basename "$CASE_FILE")
CASE_NAME="${CASE_FILE_NAME%.c}"
RUN_DIR="${CASE_DIR}/${CASE_NAME}"

PARAM_SOURCE=""
if [ -n "$INPUT_ARG" ]; then
  PARAM_SOURCE=$(resolve_file "$INPUT_ARG")
  if [ -z "$PARAM_SOURCE" ]; then
    echo "Parameter file not found: ${INPUT_ARG}"
    exit 1
  fi
else
  PARAM_DEFAULT=$(default_params_for_case "$CASE_NAME")
  if [ -z "$PARAM_DEFAULT" ]; then
    PARAM_DEFAULT="default-VE.params"
  fi
  PARAM_SOURCE="${REPO_ROOT}/${PARAM_DEFAULT}"
  if [ ! -f "$PARAM_SOURCE" ]; then
    echo "Default parameter file not found: ${PARAM_SOURCE}"
    exit 1
  fi
fi

mkdir -p "$RUN_DIR"
cp "$CASE_FILE" "$RUN_DIR/"

PARAM_BASENAME=""
if [ -n "$PARAM_SOURCE" ]; then
  PARAM_BASENAME=$(basename "$PARAM_SOURCE")
  cp "$PARAM_SOURCE" "$RUN_DIR/$PARAM_BASENAME"
fi

cd "$RUN_DIR"

qcc -I"${REPO_ROOT}/src-local" -I"${REPO_ROOT}/../src-local" -O2 -Wall \
  -disable-dimensions "$CASE_FILE_NAME" -o "$CASE_NAME" -lm

if [ -n "$PARAM_BASENAME" ]; then
  ./"$CASE_NAME" "$PARAM_BASENAME"
else
  ./"$CASE_NAME"
fi

#!/bin/bash

set -euo pipefail

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Usage: $0 case_name [params_file]"
  exit 1
fi

CASE_NAME="$1"
CASE_FILE="${CASE_NAME}.c"
ORIG_DIR=$(pwd)

if [ ! -f "$CASE_FILE" ]; then
  echo "Case source not found: $CASE_FILE"
  exit 1
fi

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

PARAM_SOURCE=""
if [ "$#" -eq 2 ]; then
  PARAM_SOURCE="$2"
else
  PARAM_SOURCE=$(default_params_for_case "$CASE_NAME")
fi

mkdir -p "$CASE_NAME"
cp "$CASE_FILE" "$CASE_NAME/"

PARAM_BASENAME=""
if [ -n "$PARAM_SOURCE" ]; then
  if [ ! -f "$PARAM_SOURCE" ]; then
    echo "Parameter file not found: $PARAM_SOURCE"
    exit 1
  fi
  PARAM_BASENAME=$(basename "$PARAM_SOURCE")
  cp "$PARAM_SOURCE" "$CASE_NAME/$PARAM_BASENAME"
fi

cd "$CASE_NAME"

qcc -I"${ORIG_DIR}/src-local" -I"${ORIG_DIR}/../src-local" -O2 -Wall \
  -disable-dimensions "$CASE_FILE" -o "$CASE_NAME" -lm

if [ -n "$PARAM_BASENAME" ]; then
  ./"$CASE_NAME" "$PARAM_BASENAME"
else
  ./"$CASE_NAME"
fi

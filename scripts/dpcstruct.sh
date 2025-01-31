#!/bin/bash

# Compute the directory where this script is located
SCRIPT_PATH="$(readlink -f "$0" 2>/dev/null || echo "$0")"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"

if [ $# -lt 1 ]; then
  echo "Usage: dpcstruct <subcommand> <options>"
  echo
  echo "Available subcommands:"
  echo "  prefilters        - Apply pre filters to the analysis"
  echo "  primarycluster    - Run the primary cluster analysis"
  echo "  secondarycluster  - Run the secondary cluster analysis"
  echo "  traceback         - Run the traceback analysis"
  echo "  postfilters       - Apply post filters to the analysis"
  exit 1
fi

subcommand="$1"
shift

# Helper function to run sub-binary, fallback to script dir if not found in PATH
run_subbinary() {
  local bin_name="$1"
  shift

  # Try finding sub-binary in PATH first
  if command -v "$bin_name" >/dev/null 2>&1; then
    exec "$bin_name" "$@"
  else
    # Fallback: same directory as this script
    if [ -x "$SCRIPT_DIR/$bin_name" ]; then
      exec "$SCRIPT_DIR/$bin_name" "$@"
    else
      echo "Error: Could not find $bin_name in PATH or in $SCRIPT_DIR"
      exit 1
    fi
  fi
}

case "$subcommand" in
  prefilters)
    run_subbinary "dpcstruct-prefilters" "$@"
    ;;
  primarycluster)
    run_subbinary "dpcstruct-primarycluster" "$@"
    ;;
  secondarycluster)
    run_subbinary "dpcstruct-secondarycluster" "$@"
    ;;
  traceback)
    run_subbinary "dpcstruct-traceback" "$@"
    ;;
  postfilters)
    run_subbinary "dpcstruct-postfilters" "$@"
    ;;
  *)
    echo "Unknown subcommand: $subcommand"
    exit 1
    ;;
esac

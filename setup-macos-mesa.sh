#!/bin/bash
# Compatibility entry point for the original Mac setup instructions.
exec /bin/bash "$(dirname "$0")/setup-macos.sh" "$@"

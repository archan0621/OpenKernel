#!/bin/bash
set -euo pipefail

# Keep Makefile as the single source of truth for the kernel build.
exec make rebuild

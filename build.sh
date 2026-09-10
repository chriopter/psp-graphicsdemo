#!/bin/sh
# Builds EBOOT.PBP with the pspdev toolchain image. Needs Docker.
#   ./build.sh          build
#   ./build.sh clean    remove objects and the EBOOT
set -e
cd "$(dirname "$0")"
docker run --rm -v "$PWD:/src" -w /src pspdev/pspdev:latest make "$@"

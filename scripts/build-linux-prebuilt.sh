#!/usr/bin/env bash
# Build Linux x64 prebuilt dependencies into prebuilt/linux.
# Intended to run on Ubuntu GitHub Actions runner.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="$ROOT/prebuilt/linux"
BUILD="$ROOT/build-linux-prebuilt"
SRC="$BUILD/src"

mkdir -p "$PREFIX" "$SRC" "$BUILD/obj"

clone_recursive() {
    local name="$1" tag="$2" dir="$3"
    if [ ! -d "$dir/.git" ]; then
        rm -rf "$dir"
        git clone --depth 1 --branch "$tag" --recursive "https://github.com/libsdl-org/${name}.git" "$dir"
    fi
}

# 1) SDL3 (already vendored in repo)
cmake -S "$ROOT/external/SDL-release-3.4.2" -B "$BUILD/obj/sdl3" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF \
    -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF
cmake --build "$BUILD/obj/sdl3" -j 2
cmake --install "$BUILD/obj/sdl3"

# 2) Optional SDL extensions (recursive clone for vendored codecs)
clone_recursive SDL_image release-3.2.4 "$SRC/SDL_image"
clone_recursive SDL_mixer release-3.2.0 "$SRC/SDL_mixer"
clone_recursive SDL_ttf   release-3.2.2 "$SRC/SDL_ttf"

cmake -S "$SRC/SDL_image" -B "$BUILD/obj/sdl_image" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DSDL3_DIR="$PREFIX/lib/cmake/SDL3" \
    -DSDLIMAGE_VENDORED=ON \
    -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_AVIF_SHARED=OFF -DSDLIMAGE_AVIF_SAVE=OFF \
    -DSDLIMAGE_TESTS=OFF -DSDLIMAGE_EXAMPLES=OFF
cmake --build "$BUILD/obj/sdl_image" -j 2
cmake --install "$BUILD/obj/sdl_image"

cmake -S "$SRC/SDL_mixer" -B "$BUILD/obj/sdl_mixer" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DSDL3_DIR="$PREFIX/lib/cmake/SDL3" \
    -DSDLMIXER_VENDORED=ON \
    -DSDLMIXER_TESTS=OFF -DSDLMIXER_EXAMPLES=OFF
cmake --build "$BUILD/obj/sdl_mixer" -j 2
cmake --install "$BUILD/obj/sdl_mixer"

cmake -S "$SRC/SDL_ttf" -B "$BUILD/obj/sdl_ttf" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DSDL3_DIR="$PREFIX/lib/cmake/SDL3" \
    -DSDLTTF_VENDORED=ON \
    -DSDLTTF_TESTS=OFF -DSDLTTF_EXAMPLES=OFF
cmake --build "$BUILD/obj/sdl_ttf" -j 2
cmake --install "$BUILD/obj/sdl_ttf"

# 3) Header-only / static deps (repo vendored sources)
cmake -S "$ROOT/external/glm-1.0.1" -B "$BUILD/obj/glm" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DGLM_BUILD_TESTS=OFF -DBUILD_TESTING=OFF
cmake --build "$BUILD/obj/glm" -j 2
cmake --install "$BUILD/obj/glm"

cmake -S "$ROOT/external/json-3.12.0" -B "$BUILD/obj/json" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DJSON_BuildTests=OFF -DBUILD_TESTING=OFF
cmake --build "$BUILD/obj/json" -j 2
cmake --install "$BUILD/obj/json"

cmake -S "$ROOT/external/entt-3.15.0" -B "$BUILD/obj/entt" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DENTT_BUILD_TESTING=OFF -DBUILD_TESTING=OFF
cmake --build "$BUILD/obj/entt" -j 2
cmake --install "$BUILD/obj/entt"

cmake -S "$ROOT/external/spdlog-1.15.3" -B "$BUILD/obj/spdlog" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DSPDLOG_BUILD_SHARED=OFF -DSPDLOG_BUILD_EXAMPLE=OFF \
    -DSPDLOG_BUILD_TESTS=OFF -DBUILD_TESTING=OFF
cmake --build "$BUILD/obj/spdlog" -j 2
cmake --install "$BUILD/obj/spdlog"

echo "Linux prebuilt dependencies installed to: $PREFIX"

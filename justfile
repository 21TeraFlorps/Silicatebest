# taken from silicate v2
os := os()
num_cpus := num_cpus()

configure:
  just _configure-{{ os }}

pure-build:
  cmake --build build --config RelWithDebInfo --parallel

# Build for the current platform
build: configure
  cmake --build build --config RelWithDebInfo --parallel

debug: build
    #!/usr/bin/env bash
    steam -applaunch 322170 >/tmp/gd-silicate.log 2>&1 &
    while ! pid="$(pgrep -n -f 'Geometry Dash\\GeometryDash\.exe')"; do
        sleep 0.25
    done
    echo "Attaching to GD with PID $pid"

    setsid gdbserver --attach localhost:2345 "$pid" \
        >/tmp/gd-silicate-gdb.log 2>&1 </dev/null &

    for _ in {1..67}; do
        if ss -ltn | grep -q ':2345'; then
            echo "gdbserver ready"
            exit 0
        fi
        sleep 0.1
    done

    echo "gdbserver failed:"
    cat /tmp/gd-silicate-gdb.log
    exit 1

# Run game with built
run: build
  {{ if os == "linux" { `echo "geode run isn't supported on Linux!"` } else { `geode run` } }}

_configure-linux:
  cmake -S . -B build -G "Ninja" \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/.local/share/Geode/cross-tools/clang-msvc-sdk/clang-cl-msvc.cmake \
    -DSPLAT_DIR=$HOME/.local/share/Geode/cross-tools/splat \
    -DCMAKE_C_COMPILER_LAUNCHER=sccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=sccache \
    -DHOST_ARCH=x64 \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

_configure-windows:
  cmake -S . -B build -G "Ninja" \
    -DCMAKE_C_COMPILER=clang-cl \
    -DCMAKE_CXX_COMPILER=clang-cl \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_BUILD_PARALLEL_LEVEL={{num_cpus}}

clean:
  rm -rf build

build-ci:
  cmake -S . -B build -G "Ninja" \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/.local/share/Geode/cross-tools/clang-msvc-sdk/clang-cl-msvc.cmake \
    -DSPLAT_DIR=$HOME/.local/share/Geode/cross-tools/splat \
    -DHOST_ARCH=x64 \
    -DCMAKE_C_COMPILER_LAUNCHER=sccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=sccache \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DGEODE_DONT_INSTALL_MODS=ON
  cmake --build build --config RelWithDebInfo --parallel

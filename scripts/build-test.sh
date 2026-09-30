#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$project_dir/build/linux"
configuration=Release
qt_prefix=""
generator=""

usage() {
    cat <<'EOF'
Usage: bash scripts/build-test.sh [options]
  --qt-prefix PATH   Qt kit directory (omit for system-installed Qt)
  --build-dir PATH   Build directory (default: PROJECT/build/linux)
  --config NAME      Release, Debug, RelWithDebInfo, or MinSizeRel
  --generator NAME   Optional CMake generator, e.g. Ninja
  --help             Show this help
EOF
}

while (($#)); do
    case "$1" in
        --help) usage; exit 0 ;;
        --qt-prefix|--build-dir|--config|--generator)
            if (($# < 2)) || [[ -z "$2" || "$2" == --* ]]; then
                echo "Missing value for $1" >&2
                exit 2
            fi
            case "$1" in
                --qt-prefix) qt_prefix="$2" ;;
                --build-dir) build_dir="$2" ;;
                --config) configuration="$2" ;;
                --generator) generator="$2" ;;
            esac
            shift 2
            ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

case "$configuration" in
    Release|Debug|RelWithDebInfo|MinSizeRel) ;;
    *) echo "Invalid configuration: $configuration" >&2; exit 2 ;;
esac

for tool in cmake ctest; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "$tool is required. Install the build prerequisites in README.md." >&2
        exit 1
    fi
done

configure_args=(-S "$project_dir" -B "$build_dir"
    "-DCMAKE_BUILD_TYPE=$configuration" -DBUILD_TESTING=ON)
if [[ -n "$qt_prefix" ]]; then
    configure_args+=("-DCMAKE_PREFIX_PATH=$qt_prefix")
fi
if [[ -n "$generator" ]]; then
    configure_args+=(-G "$generator")
fi

cmake "${configure_args[@]}"
cmake --build "$build_dir" --config "$configuration" --parallel
ctest --test-dir "$build_dir" -C "$configuration" --output-on-failure --no-tests=error

echo "Build and tests passed. Output directory: $build_dir"

#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export HOMEBREW_NO_AUTO_UPDATE="${HOMEBREW_NO_AUTO_UPDATE:-1}"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This script is intended for macOS." >&2
  exit 1
fi

INSTALL_DEPS=0
if [[ "${1:-}" == "--install-deps" ]]; then
  INSTALL_DEPS=1
elif [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  cat <<'USAGE'
Usage:
  scripts/build_macos.sh [--install-deps]

Builds the same project components as the Docker deps-builder stage on macOS:
  - gofunct C archive
  - ilfreporter
  - ilf CLIs
  - ilfx CLIs
  - xsltcli
USAGE
  exit 0
fi

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required. Install it from https://brew.sh, then rerun this script." >&2
  exit 1
fi

BREW_PREFIX="$(brew --prefix)"

if [[ "${INSTALL_DEPS}" == "1" ]]; then
  brew install \
    autoconf \
    automake \
    cmake \
    glib \
    go \
    googletest \
    gsl \
    libtool \
    libxml2 \
    libxslt \
    pkg-config \
    xerces-c \
    xqilla \
    antlr4-cpp-runtime

  cat <<'EOF'
Note: TinyCC is also required for ilf. Homebrew may not provide a TinyCC formula
on all macOS setups, so this script detects an existing libtcc installation from
pkg-config, tcc in PATH, /usr/local, or /opt/homebrew.
EOF
fi

LIBXML2_PREFIX="$(brew --prefix libxml2)"
LIBXSLT_PREFIX="$(brew --prefix libxslt)"
GLIB_PREFIX="$(brew --prefix glib)"
GSL_PREFIX="$(brew --prefix gsl)"

detect_tinycc_prefix() {
  local pkg_prefix
  if pkg_prefix="$(pkg-config --variable=prefix libtcc 2>/dev/null)" && [[ -n "${pkg_prefix}" ]]; then
    echo "${pkg_prefix}"
    return
  fi

  if command -v tcc >/dev/null 2>&1; then
    dirname "$(dirname "$(command -v tcc)")"
    return
  fi

  if [[ -f /usr/local/include/libtcc.h && -e /usr/local/lib/libtcc.a ]]; then
    echo /usr/local
    return
  fi

  if [[ -f /opt/homebrew/include/libtcc.h && -e /opt/homebrew/lib/libtcc.a ]]; then
    echo /opt/homebrew
    return
  fi

  echo "TinyCC/libtcc was not found. Install TinyCC with libtcc headers/libs, then rerun this script." >&2
  exit 1
}

TINYCC_PREFIX="$(detect_tinycc_prefix)"

export PATH="${LIBXML2_PREFIX}/bin:${LIBXSLT_PREFIX}/bin:${PATH}"
export PKG_CONFIG_PATH="${GLIB_PREFIX}/lib/pkgconfig:${LIBXML2_PREFIX}/lib/pkgconfig:${LIBXSLT_PREFIX}/lib/pkgconfig:${GSL_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export CMAKE_PREFIX_PATH="${BREW_PREFIX}:${CMAKE_PREFIX_PATH:-}"

echo "==> Building gofunct C archive"
mkdir -p "${ROOT_DIR}/gofunct/build-macos"
(
  cd "${ROOT_DIR}"
  GOCACHE="${ROOT_DIR}/gofunct/build-macos/go-cache" \
  GOPATH="${ROOT_DIR}/gofunct/build-macos/go" \
    go build -buildmode=c-archive -mod=vendor -o "${ROOT_DIR}/gofunct/build-macos/libgofunct.a" ./gofunct
)

echo "==> Building ilfreporter"
cmake -S "${ROOT_DIR}/ilfreporter-0.0.1" -B "${ROOT_DIR}/ilfreporter-0.0.1/build-macos" \
  -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}"
cmake --build "${ROOT_DIR}/ilfreporter-0.0.1/build-macos"

echo "==> Building ilf CLIs"
make -C "${ROOT_DIR}/ilf" BUILD_DIR=build-macos/obj TARGET_DIR=build-macos/bin clean
make -C "${ROOT_DIR}/ilf" \
  BUILD_DIR=build-macos/obj \
  TARGET_DIR=build-macos/bin \
  TCC_PREFIX="${TINYCC_PREFIX}" \
  kpmr_source_tree_cli \
  build_tree_kpmr_cli \
  eval_cli \
  eval_server \
  build_tree_cli

echo "==> Building ilfx"
cmake -S "${ROOT_DIR}/ilfx" -B "${ROOT_DIR}/ilfx/build-macos" \
  -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}"
cmake --build "${ROOT_DIR}/ilfx/build-macos"

echo "==> Building xsltcli"
mkdir -p "${ROOT_DIR}/xsltcli/build-macos"
make -C "${ROOT_DIR}/xsltcli" OUTPUT=build-macos/xsltcli clean build

cat <<EOF
==> Done
macOS build outputs:
  ${ROOT_DIR}/gofunct/build-macos/
  ${ROOT_DIR}/ilfreporter-0.0.1/build-macos/
  ${ROOT_DIR}/ilf/build-macos/bin/
  ${ROOT_DIR}/ilfx/build-macos/bin/
  ${ROOT_DIR}/xsltcli/build-macos/xsltcli
EOF

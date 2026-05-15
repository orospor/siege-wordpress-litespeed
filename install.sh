#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${SIEGE_REPO_URL:-https://github.com/orospor/siege-wordpress-litespeed.git}"
BRANCH="${SIEGE_BRANCH:-master}"
PREFIX="${PREFIX:-/usr/local}"
INSTALL_DEPS=1
KEEP_SOURCE=0
SOURCE_DIR=""

usage() {
  cat <<'EOF'
Install siege-wordpress-litespeed.

Usage:
  curl -fsSL https://raw.githubusercontent.com/orospor/siege-wordpress-litespeed/master/install.sh | bash
  bash install.sh [options]

Options:
  --prefix DIR     Install prefix. Default: /usr/local
  --branch NAME    Git branch to install. Default: master
  --repo URL       Git repository URL.
  --source DIR     Use an existing local source checkout.
  --no-deps        Do not install build dependencies.
  --keep-source    Keep the temporary source directory after install.
  -h, --help       Show this help.

Environment:
  PREFIX=/path                 Same as --prefix
  SIEGE_BRANCH=name            Same as --branch
  SIEGE_REPO_URL=url           Same as --repo
  SIEGE_INSTALL_DEPS=0         Same as --no-deps
  SIEGE_KEEP_SOURCE=1          Same as --keep-source
EOF
}

log() {
  printf '[siege-install] %s\n' "$*"
}

die() {
  printf '[siege-install] ERROR: %s\n' "$*" >&2
  exit 1
}

have() {
  command -v "$1" >/dev/null 2>&1
}

run_as_root() {
  if [ "$(id -u)" -eq 0 ]; then
    "$@"
  elif have sudo; then
    sudo "$@"
  else
    die "sudo is required to install dependencies or install into protected directories"
  fi
}

install_deps() {
  if [ "$INSTALL_DEPS" -eq 0 ]; then
    log "Skipping dependency installation"
    return
  fi

  if have git && have make && (have cc || have gcc || have clang) && have autoconf && have automake && have aclocal && have autoreconf; then
    log "Build tools already available"
    return
  fi

  log "Installing build dependencies"
  if have brew; then
    brew install git autoconf automake libtool pkg-config openssl zlib
  elif have apt-get; then
    run_as_root apt-get update
    run_as_root apt-get install -y git build-essential autoconf automake libtool pkg-config libssl-dev zlib1g-dev
  elif have dnf; then
    run_as_root dnf install -y git gcc make autoconf automake libtool pkgconfig openssl-devel zlib-devel
  elif have yum; then
    run_as_root yum install -y git gcc make autoconf automake libtool pkgconfig openssl-devel zlib-devel
  elif have pacman; then
    run_as_root pacman -Sy --needed --noconfirm git base-devel autoconf automake libtool pkgconf openssl zlib
  elif have apk; then
    run_as_root apk add --no-cache git build-base autoconf automake libtool pkgconf openssl-dev zlib-dev
  else
    die "No supported package manager found. Install git, make, a C compiler, autoconf, automake, libtool, OpenSSL, and zlib, then rerun with --no-deps"
  fi
}

prepare_source() {
  if [ -n "$SOURCE_DIR" ]; then
    [ -d "$SOURCE_DIR" ] || die "source directory does not exist: $SOURCE_DIR"
    cd "$SOURCE_DIR"
    return
  fi

  if [ -f "./configure.ac" ] && [ -d "./src" ] && [ -f "./src/main.c" ]; then
    log "Using current directory as source checkout"
    return
  fi

  have git || die "git is required to clone $REPO_URL"
  WORKDIR="$(mktemp -d "${TMPDIR:-/tmp}/siege-install.XXXXXX")"
  SOURCE_DIR="$WORKDIR/siege-wordpress-litespeed"
  log "Cloning $REPO_URL#$BRANCH"
  git clone --depth 1 --branch "$BRANCH" "$REPO_URL" "$SOURCE_DIR"
  cd "$SOURCE_DIR"
}

configure_env_for_brew() {
  if ! have brew; then
    return
  fi

  local openssl_prefix=""
  local zlib_prefix=""
  openssl_prefix="$(brew --prefix openssl@3 2>/dev/null || true)"
  zlib_prefix="$(brew --prefix zlib 2>/dev/null || true)"

  if [ -n "$openssl_prefix" ]; then
    export CPPFLAGS="${CPPFLAGS:-} -I$openssl_prefix/include"
    export LDFLAGS="${LDFLAGS:-} -L$openssl_prefix/lib"
    export PKG_CONFIG_PATH="$openssl_prefix/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
  fi
  if [ -n "$zlib_prefix" ]; then
    export CPPFLAGS="${CPPFLAGS:-} -I$zlib_prefix/include"
    export LDFLAGS="${LDFLAGS:-} -L$zlib_prefix/lib"
    export PKG_CONFIG_PATH="$zlib_prefix/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
  fi
}

install_siege() {
  log "Bootstrapping autotools"
  ./utils/bootstrap

  configure_env_for_brew

  log "Configuring with prefix $PREFIX"
  ./configure --prefix="$PREFIX"

  log "Building"
  make

  log "Installing"
  if [ -w "$PREFIX" ] || { [ ! -e "$PREFIX" ] && [ -w "$(dirname "$PREFIX")" ]; }; then
    make install
  else
    run_as_root make install
  fi
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      PREFIX="${2:-}"
      [ -n "$PREFIX" ] || die "--prefix requires a directory"
      shift 2
      ;;
    --branch)
      BRANCH="${2:-}"
      [ -n "$BRANCH" ] || die "--branch requires a branch name"
      shift 2
      ;;
    --repo)
      REPO_URL="${2:-}"
      [ -n "$REPO_URL" ] || die "--repo requires a repository URL"
      shift 2
      ;;
    --source)
      SOURCE_DIR="${2:-}"
      [ -n "$SOURCE_DIR" ] || die "--source requires a directory"
      shift 2
      ;;
    --no-deps)
      INSTALL_DEPS=0
      shift
      ;;
    --keep-source)
      KEEP_SOURCE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown option: $1"
      ;;
  esac
done

if [ "${SIEGE_INSTALL_DEPS:-1}" = "0" ]; then
  INSTALL_DEPS=0
fi
if [ "${SIEGE_KEEP_SOURCE:-0}" = "1" ]; then
  KEEP_SOURCE=1
fi

ORIGINAL_DIR="$(pwd)"
WORKDIR=""
trap 'if [ "${KEEP_SOURCE:-0}" -eq 0 ] && [ -n "${WORKDIR:-}" ] && [ -d "$WORKDIR" ]; then rm -rf "$WORKDIR"; fi' EXIT

install_deps
prepare_source
install_siege

log "Installed: $(command -v "$PREFIX/bin/siege" 2>/dev/null || printf '%s/bin/siege' "$PREFIX")"
log "Try: $PREFIX/bin/siege --help"
cd "$ORIGINAL_DIR"

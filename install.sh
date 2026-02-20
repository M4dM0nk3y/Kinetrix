#!/bin/bash
# ============================================================
#   KINETRIX INSTALLER v2.0
#   The Robotics Programming Language
#   https://github.com/M4dM0nk3y/Kinetrix
# ============================================================

set -e

REPO="M4dM0nk3y/Kinetrix"
VERSION="v2.0"
BIN_DIR="/usr/local/bin"
KCC="$BIN_DIR/kcc"

# ── Colors ──────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
RESET='\033[0m'

print_banner() {
  echo ""
  echo -e "${CYAN}${BOLD}"
  echo "  ██╗  ██╗██╗███╗   ██╗███████╗████████╗██████╗ ██╗██╗  ██╗"
  echo "  ██║ ██╔╝██║████╗  ██║██╔════╝╚══██╔══╝██╔══██╗██║╚██╗██╔╝"
  echo "  █████╔╝ ██║██╔██╗ ██║█████╗     ██║   ██████╔╝██║ ╚███╔╝ "
  echo "  ██╔═██╗ ██║██║╚██╗██║██╔══╝     ██║   ██╔══██╗██║ ██╔██╗ "
  echo "  ██║  ██╗██║██║ ╚████║███████╗   ██║   ██║  ██║██║██╔╝ ██╗"
  echo "  ╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝"
  echo -e "${RESET}"
  echo -e "  ${BOLD}The Robotics Programming Language${RESET}  ·  ${VERSION}"
  echo -e "  ${CYAN}https://m4dm0nk3y.github.io/Kinetrix/${RESET}"
  echo ""
}

log()     { echo -e "  ${GREEN}✓${RESET}  $1"; }
info()    { echo -e "  ${CYAN}→${RESET}  $1"; }
warn()    { echo -e "  ${YELLOW}⚠${RESET}  $1"; }
error()   { echo -e "  ${RED}✗  ERROR:${RESET} $1"; exit 1; }
section() { echo ""; echo -e "  ${BOLD}$1${RESET}"; echo "  $(printf '─%.0s' {1..50})"; }

# ── Detect OS + Architecture ────────────────────────────────
detect_platform() {
  section "Detecting Platform"

  OS="$(uname -s)"
  ARCH="$(uname -m)"

  case "$OS" in
    Darwin)
      case "$ARCH" in
        arm64)  PLATFORM="macos-arm64"  ;;
        x86_64) PLATFORM="macos-x86_64" ;;
        *) error "Unsupported Mac architecture: $ARCH" ;;
      esac
      ;;
    Linux)
      case "$ARCH" in
        x86_64)  PLATFORM="linux-x86_64"  ;;
        aarch64) PLATFORM="linux-arm64"    ;;
        armv7l)  PLATFORM="linux-armv7"    ;;
        *) error "Unsupported Linux architecture: $ARCH" ;;
      esac
      ;;
    *)
      error "Unsupported OS: $OS. Kinetrix supports macOS and Linux."
      ;;
  esac

  log "Platform: $OS ($ARCH) → $PLATFORM"
}

# ── Check dependencies ───────────────────────────────────────
check_deps() {
  section "Checking Dependencies"

  if command -v curl &>/dev/null; then
    log "curl found"
    DOWNLOADER="curl"
  elif command -v wget &>/dev/null; then
    log "wget found"
    DOWNLOADER="wget"
  else
    error "Neither curl nor wget found. Install one of them first."
  fi
}

# ── Download binary from GitHub Releases ─────────────────────
download_binary() {
  section "Downloading kcc ${VERSION}"

  BINARY_NAME="kcc-${PLATFORM}"
  DOWNLOAD_URL="https://github.com/${REPO}/releases/download/${VERSION}/${BINARY_NAME}"
  TMP_FILE="$(mktemp /tmp/kcc.XXXXXX)"

  info "Downloading from GitHub Releases..."
  info "$DOWNLOAD_URL"

  if [ "$DOWNLOADER" = "curl" ]; then
    HTTP_CODE=$(curl -fsSL -w "%{http_code}" -o "$TMP_FILE" "$DOWNLOAD_URL" 2>/dev/null)
  else
    wget -q -O "$TMP_FILE" "$DOWNLOAD_URL" 2>/dev/null
    HTTP_CODE=200
  fi

  if [ "$HTTP_CODE" = "404" ] || [ ! -s "$TMP_FILE" ]; then
    rm -f "$TMP_FILE"
    echo ""
    echo -e "  ${YELLOW}Pre-built binary not found for ${PLATFORM}.${RESET}"
    echo -e "  ${YELLOW}Please build from source:${RESET}"
    echo ""
    echo "  git clone https://github.com/${REPO}.git"
    echo "  cd Kinetrix"
    echo "  make"
    echo ""
    error "Binary download failed."
  fi

  chmod +x "$TMP_FILE"
  log "Downloaded successfully"
  echo "$TMP_FILE"
}

# ── Install binary ───────────────────────────────────────────
install_binary() {
  local TMP_FILE="$1"
  section "Installing kcc"

  if [ -w "$BIN_DIR" ]; then
    mv "$TMP_FILE" "$KCC"
    chmod +x "$KCC"
    log "Installed to $KCC"
  else
    info "Need sudo to install to $BIN_DIR..."
    sudo mv "$TMP_FILE" "$KCC"
    sudo chmod +x "$KCC"
    log "Installed to $KCC (with sudo)"
  fi
}

# ── Verify ───────────────────────────────────────────────────
verify() {
  section "Verifying Installation"
  if command -v kcc &>/dev/null; then
    log "kcc is available globally — type 'kcc' anywhere"
  else
    warn "kcc installed but not in PATH yet."
    warn "Restart your terminal or run: source ~/.zshrc"
  fi
}

# ── Uninstall ────────────────────────────────────────────────
uninstall() {
  echo ""
  echo -e "  ${RED}${BOLD}Uninstalling Kinetrix...${RESET}"
  if [ -f "$KCC" ]; then
    sudo rm -f "$KCC"
    echo -e "  ${GREEN}✓${RESET}  Removed $KCC"
  else
    echo -e "  ${YELLOW}⚠${RESET}  kcc not found at $KCC"
  fi
  echo ""
  exit 0
}

# ── Done ─────────────────────────────────────────────────────
print_done() {
  echo ""
  echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗"
  echo -e "  ║   Kinetrix ${VERSION} installed successfully!   ║"
  echo -e "  ╚══════════════════════════════════════════╝${RESET}"
  echo ""
  echo -e "  ${BOLD}Quick start:${RESET}"
  echo ""
  echo -e "  ${CYAN}# 1. Write a program${RESET}"
  echo "  echo 'program { loop forever { turn on pin 13  wait 1000  turn off pin 13  wait 1000 } }' > blink.kx"
  echo ""
  echo -e "  ${CYAN}# 2. Compile it${RESET}"
  echo "  kcc blink.kx -o blink.ino"
  echo ""
  echo -e "  ${CYAN}# 3. Upload blink.ino to Arduino via Arduino IDE${RESET}"
  echo ""
  echo -e "  📄 Language Guide : https://m4dm0nk3y.github.io/Kinetrix/KINETRIX_COMPREHENSIVE_GUIDE.pdf"
  echo -e "  🌐 Website        : https://m4dm0nk3y.github.io/Kinetrix/"
  echo -e "  ⭐ Star on GitHub : https://github.com/${REPO}"
  echo ""
}

# ── Entry point ──────────────────────────────────────────────
main() {
  print_banner
  [ "$1" = "uninstall" ] && uninstall

  check_deps
  detect_platform
  TMP=$(download_binary)
  install_binary "$TMP"
  verify
  print_done
}

main "$@"

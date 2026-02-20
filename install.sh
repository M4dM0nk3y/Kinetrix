#!/bin/bash
# ============================================================
#   KINETRIX INSTALLER
#   The Robotics Programming Language
#   https://github.com/M4dM0nk3y/Kinetrix
# ============================================================

set -e

# ── Colors ──────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
RESET='\033[0m'

REPO="https://github.com/M4dM0nk3y/Kinetrix.git"
INSTALL_DIR="$HOME/.kinetrix"
BIN_DIR="/usr/local/bin"
KCC="$BIN_DIR/kcc"

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
  echo -e "  ${BOLD}The Robotics Programming Language${RESET}  ·  v2.0"
  echo -e "  ${CYAN}https://m4dm0nk3y.github.io/Kinetrix/${RESET}"
  echo ""
}

log()     { echo -e "  ${GREEN}✓${RESET}  $1"; }
info()    { echo -e "  ${CYAN}→${RESET}  $1"; }
warn()    { echo -e "  ${YELLOW}⚠${RESET}  $1"; }
error()   { echo -e "  ${RED}✗${RESET}  $1"; exit 1; }
section() { echo ""; echo -e "  ${BOLD}$1${RESET}"; echo "  $(printf '─%.0s' {1..50})"; }

# ── Check dependencies ───────────────────────────────────────
check_deps() {
  section "Checking Dependencies"

  # gcc
  if command -v gcc &>/dev/null; then
    log "gcc found ($(gcc --version | head -1))"
  else
    error "gcc not found. Install it with:
         macOS:  xcode-select --install
         Ubuntu: sudo apt install gcc
         Arch:   sudo pacman -S gcc"
  fi

  # git
  if command -v git &>/dev/null; then
    log "git found ($(git --version))"
  else
    error "git not found. Install git first."
  fi

  # make (optional but nice)
  if command -v make &>/dev/null; then
    log "make found"
  else
    warn "make not found — will compile manually"
  fi
}

# ── Clone / Update repo ──────────────────────────────────────
get_source() {
  section "Getting Source Code"

  if [ -d "$INSTALL_DIR/.git" ]; then
    info "Existing install found — updating..."
    cd "$INSTALL_DIR"
    git pull --quiet origin main && log "Updated to latest version"
  else
    info "Cloning Kinetrix from GitHub..."
    rm -rf "$INSTALL_DIR"
    git clone --quiet "$REPO" "$INSTALL_DIR"
    log "Cloned successfully"
  fi

  cd "$INSTALL_DIR"
}

# ── Build compiler ───────────────────────────────────────────
build() {
  section "Building Compiler (kcc)"

  cd "$INSTALL_DIR"

  SRCS="compiler_v3.c ast.c symbol_table.c error.c parser.c codegen.c pin_tracker.c diagnostics.c"

  # Check all source files exist
  for f in $SRCS; do
    [ -f "$f" ] || error "Missing source file: $f"
  done

  info "Compiling..."
  if command -v make &>/dev/null && [ -f Makefile ]; then
    make clean &>/dev/null || true
    make        &>/dev/null && log "Built with make"
  else
    gcc -Wall -std=c99 -o kcc $SRCS -lm && log "Built with gcc"
  fi

  [ -f "./kcc" ] || error "Build failed — kcc binary not produced"
  log "kcc binary ready"
}

# ── Install binary ───────────────────────────────────────────
install_bin() {
  section "Installing kcc"

  # Try /usr/local/bin first (needs sudo on most systems)
  if [ -w "$BIN_DIR" ]; then
    cp "$INSTALL_DIR/kcc" "$KCC"
    chmod +x "$KCC"
    log "Installed to $KCC"
  else
    info "Need sudo to install to $BIN_DIR..."
    sudo cp "$INSTALL_DIR/kcc" "$KCC"
    sudo chmod +x "$KCC"
    log "Installed to $KCC (with sudo)"
  fi
}

# ── Shell PATH setup ─────────────────────────────────────────
setup_path() {
  # If /usr/local/bin isn't in PATH, add INSTALL_DIR to the shell rc
  if ! echo "$PATH" | grep -q "$BIN_DIR"; then
    SHELL_RC="$HOME/.bashrc"
    [ -n "$ZSH_VERSION" ] && SHELL_RC="$HOME/.zshrc"
    [ -f "$HOME/.zshrc" ]  && SHELL_RC="$HOME/.zshrc"

    echo "" >> "$SHELL_RC"
    echo "# Kinetrix" >> "$SHELL_RC"
    echo "export PATH=\"$INSTALL_DIR:\$PATH\"" >> "$SHELL_RC"

    warn "Added Kinetrix to PATH in $SHELL_RC"
    warn "Run: source $SHELL_RC  (or restart your terminal)"
  fi
}

# ── Verify install ───────────────────────────────────────────
verify() {
  section "Verifying Installation"

  if command -v kcc &>/dev/null; then
    log "kcc is available globally"
  elif [ -f "$INSTALL_DIR/kcc" ]; then
    warn "kcc installed at $INSTALL_DIR/kcc (not yet in PATH)"
    warn "Restart your terminal or run: source ~/.zshrc"
  else
    error "Installation failed"
  fi
}

# ── Done ─────────────────────────────────────────────────────
print_done() {
  echo ""
  echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗"
  echo -e "  ║   Kinetrix v2.0 installed successfully!  ║"
  echo -e "  ╚══════════════════════════════════════════╝${RESET}"
  echo ""
  echo -e "  ${BOLD}Quick start:${RESET}"
  echo ""
  echo -e "  ${CYAN}# Write a program${RESET}"
  echo '  echo '"'"'program { loop forever { turn on pin 13  wait 1000  turn off pin 13  wait 1000 } }'"'"' > blink.kx'
  echo ""
  echo -e "  ${CYAN}# Compile it${RESET}"
  echo "  kcc blink.kx -o blink.ino"
  echo ""
  echo -e "  ${CYAN}# Then upload blink.ino to Arduino via Arduino IDE${RESET}"
  echo ""
  echo -e "  📄 Language Guide : https://m4dm0nk3y.github.io/Kinetrix/KINETRIX_COMPREHENSIVE_GUIDE.pdf"
  echo -e "  🌐 Website        : https://m4dm0nk3y.github.io/Kinetrix/"
  echo -e "  ⭐ GitHub         : https://github.com/M4dM0nk3y/Kinetrix"
  echo ""
}

# ── Uninstall mode ───────────────────────────────────────────
uninstall() {
  echo ""
  echo -e "  ${RED}${BOLD}Uninstalling Kinetrix...${RESET}"
  sudo rm -f "$KCC"
  rm -rf "$INSTALL_DIR"
  echo -e "  ${GREEN}✓${RESET}  Kinetrix removed."
  echo ""
  exit 0
}

# ── Entry point ──────────────────────────────────────────────
main() {
  print_banner

  [ "$1" = "uninstall" ] && uninstall

  check_deps
  get_source
  build
  install_bin
  setup_path
  verify
  print_done
}

main "$@"

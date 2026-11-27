#!/usr/bin/env sh
set -eu

REPO="AchillesOrg/Kestral"
INSTALL_DIR="${KESTRAL_INSTALL_DIR:-$HOME/.local/bin}"

OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
  Darwin)
    ASSET="kestral-macos-arm64"
    if [ "$ARCH" = "x86_64" ]; then
      ASSET="kestral-macos-x64"
    fi
    ;;
  Linux)
    ASSET="kestral-linux-x64"
    ;;
  *)
    echo "Unsupported OS: $OS. Use install.ps1 on Windows." >&2
    exit 1
    ;;
esac

mkdir -p "$INSTALL_DIR"
TAG="$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" \
  | sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -n 1)"

if [ -n "$TAG" ]; then
  URL="https://github.com/$REPO/releases/download/$TAG/$ASSET"
else
  URL="https://github.com/$REPO/releases/latest/download/$ASSET"
fi

TMP_FILE="$INSTALL_DIR/.kestral.tmp.$$"
echo "Downloading $URL..."
curl -fsSL "$URL" -o "$TMP_FILE"
chmod +x "$TMP_FILE"
mv -f "$TMP_FILE" "$INSTALL_DIR/kestral"

echo "Installed Kestral to $INSTALL_DIR/kestral"
"$INSTALL_DIR/kestral" --version

case ":${PATH}:" in
  *":$INSTALL_DIR:"*) ;;
  *)
    echo ""
    echo "Note: add $INSTALL_DIR to your PATH:"
    echo "  export PATH=\"$INSTALL_DIR:\$PATH\""
    ;;
esac

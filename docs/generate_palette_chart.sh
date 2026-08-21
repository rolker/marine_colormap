#!/bin/bash
# Regenerate the palette chart in docs/ from the live registry.
#
# Run after adding or changing a palette. The SVG comes from the `palette_chart`
# tool, which walks the registry, so it cannot drift from the code. The PNG is a
# convenience for README rendering and needs ImageMagick or rsvg-convert; if
# neither is present the SVG is still regenerated.
set -euo pipefail

DOCS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$DOCS_DIR")"

TOOL="${PALETTE_CHART_BIN:-}"
if [[ -z "$TOOL" ]]; then
    # Look in a colcon build space next to the usual workspace layout.
    TOOL="$(find "$REPO_ROOT/../../../build/marine_colormap" -maxdepth 1 -name palette_chart -type f 2>/dev/null | head -1)"
fi
if [[ -z "$TOOL" || ! -x "$TOOL" ]]; then
    echo "error: palette_chart not found. Build the package first, or set" >&2
    echo "       PALETTE_CHART_BIN to the built executable." >&2
    exit 1
fi

"$TOOL" "$DOCS_DIR/palettes.svg"
echo "wrote $DOCS_DIR/palettes.svg"

if command -v rsvg-convert >/dev/null 2>&1; then
    rsvg-convert -o "$DOCS_DIR/palettes.png" "$DOCS_DIR/palettes.svg"
    echo "wrote $DOCS_DIR/palettes.png"
elif command -v convert >/dev/null 2>&1; then
    convert -background white "$DOCS_DIR/palettes.svg" "$DOCS_DIR/palettes.png"
    echo "wrote $DOCS_DIR/palettes.png"
else
    echo "note: no rsvg-convert or ImageMagick; PNG not regenerated." >&2
fi

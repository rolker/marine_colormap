---
issue: 12
---

# Issue #12 — Umbrella: colormap enhancement arc — fixed-domain palettes, topo-bathy breaks, legends, on-disk palettes, authoring tool

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-08-21 11:39 -04:00
**By**: Claude Code Agent (Claude Fable 5)
**Verdict**: approved

**Branch**: feature/issue-12 at `1928130`
**Mode**: pre-push
**Depth**: Light (reason: docs-only, 2 files, no linter profile for Markdown)
**Must-fix**: 7 | **Suggestions**: 11
**Round**: 1 | **Ship**: recommended — all 7 must-fix findings addressed in 1928130; no design concerns raised

Static analysis: no profile configured for these file types (`.md`) — content review only.
Claude Adversarial: 1 pass, accuracy-focused (external-claim verification, internal
contradictions, attribution, consistency with the current library).
Local Adversarial: off (prose diff; low-trust source not worth the wall-clock).

### Findings
- [x] (must-fix) Multi-break provenance contradicted the S-100 section — scoped the claim to pivot semantics — `docs/vision.md`
- [x] (must-fix) Chart of the Future → S-100 lineage stated as fact; now attributed as a participant's account — `docs/vision.md`
- [x] (must-fix) "camp has no legend display today" false — camp has one in the modal dialog — `docs/vision.md`
- [x] (must-fix) Water Level Adjustment read as always-on; it is operator-selectable, off by default — `docs/vision.md`
- [x] (must-fix) "Open items become blocking at step 6" wrong for legend placement (step 5) and authoring tool (step 8) — `docs/vision.md`
- [x] (must-fix) Prior-art survey is four comments, not three — `docs/vision.md`
- [x] (must-fix) Rainbow-paper citation: missing pages, undisclosed author-list discrepancy — `docs/vision.md`
- [x] (suggestion) GLSL parity overstated as current fact; reframed as a goal being adopted — `docs/vision.md`
- [x] (suggestion) "A delegate cannot host a QWidget" false as an absolute; reworded to should not — `docs/vision.md`
- [x] (suggestion) "Prior art is unanimous" rests on two systems; softened — `docs/vision.md`
- [x] (suggestion) xyY called a perceptual choice; it is a conformance choice — `docs/vision.md`
- [x] (suggestion) Stop-list vs hard-discontinuity tension named as an explicit step-6 criterion — `docs/vision.md`
- [x] (suggestion) Kovesi cited for the luminance-modulation principle — `docs/vision.md`
- [x] (suggestion) Layering rule reconciled with its own list order — `docs/vision.md`
- [x] (suggestion) Unverified "five files" catalogue count dropped — `docs/vision.md`
- [x] (suggestion) Trailing whitespace — `docs/vision.md`
- [x] (incidental) Stale registry comments predating the quality palette and viridis/turbo import — `marine_colormap/include/marine_colormap/palette.hpp`, `marine_colormap/src/palette.cpp`

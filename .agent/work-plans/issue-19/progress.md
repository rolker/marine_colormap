---
issue: 19
---

# Issue #19 — Costmap palette: rviz-exact occupancy grid colours as a fixed-domain lookup table

## Local Review (Pre-Push)
**Status**: complete
**When**: 2026-08-21 15:42 -04:00
**By**: Claude Code Agent (Claude Fable 5)
**Verdict**: approved

**Branch**: feature/issue-19 at `8755f68`
**Mode**: pre-push
**Depth**: Light (reason: one additive module with an exact external reference)
**Must-fix**: 0 | **Suggestions**: 4 (3 applied, 1 disputed)
**Round**: 2 | **Ship**: recommended — Copilot returned "Approval recommended"; CI green

Static analysis: cpplint + uncrustify clean via colcon test.
Review came from Copilot on PR #20 rather than a local adversarial pass, and found real
documentation and test-strength issues.

### Findings
- [x] (suggestion) Sentinel comment claimed out-of-band renders transparent; it actually resolves via unset under/over to the illegal-band colours — comment corrected, behaviour kept, test added — `src/occupancy.cpp`
- [x] (suggestion) Header claimed "bit-exact" then noted a 1-LSB ramp difference — softened to exact-discrete / within-one-level-ramps — `include/marine_colormap/occupancy.hpp`
- [x] (suggestion) Determinism test compared only r and a channels — now compares all four — `test/test_occupancy.cpp`
- [ ] (false positive) `<string>` flagged as unused; the file does use std::string via `LookupEntry::label` in an EXPECT_EQ against a literal. Kept; disagreement posted on the PR — `test/test_occupancy.cpp`

### Notes
- **The vision's hypothesis held**: no new machinery was needed for a fixed domain. A
  LookupTable keyed by absolute values owns its domain intrinsically, so no range model is
  involved. A test asserts it so the claim fails loudly if it stops being true.
- Test reference is a transcription of rviz's `palette_builder.cpp` formulae, so a
  transcription error would validate against itself. Flagged for reviewers in the PR body.

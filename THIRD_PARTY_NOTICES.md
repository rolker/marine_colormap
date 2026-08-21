# Third-party notices

This package is licensed under Apache-2.0 (see [`LICENSE`](LICENSE)). It also embeds
colour-table data from the third parties below. Each entry keeps its own licence; the full
terms are reproduced here as those licences require.

If you add or update vendored palette data, add its notice here in the same PR.

---

## Scientific Colour Maps — `oleron`

**Where**: `marine_colormap/src/topobathy_palettes.cpp`, `oleron_stops()`.
**Source**: Fabio Crameri, *Scientific colour maps*, version 8.0.0. Transcribed from the
GMT distribution's `share/cpt/SCM/oleron.cpt`.
**Cite as**: Crameri, F. (2023). *Scientific colour maps*. Zenodo.
<https://doi.org/10.5281/zenodo.1243862>
**Rationale**: Crameri, F., Shephard, G.E. & Heron, P.J. (2020). *The misuse of colour in
science communication*. Nature Communications 11, 5444.
<https://doi.org/10.1038/s41467-020-19160-7>

**Licence**: MIT (Expat)

```
Copyright (c) 2023, Fabio Crameri

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
```

---

## matplotlib perceptual colormaps — `viridis`, `turbo`

**Where**: `marine_colormap/src/perceptual_palettes.cpp`.
**Source**: the canonical 256-entry tables as distributed by matplotlib 3.6.3.

- **`viridis`** — designed by Stéfan van der Walt and Nathaniel J. Smith. Released by its
  authors into the public domain under **CC0**: *"This file and the colormaps in it are
  released under the CC0 license / public domain dedication."*
  (<https://github.com/BIDS/colormap>) No notice is required; it is recorded here for
  attribution.
- **`turbo`** — designed by Anton Mikhailov (Google), distributed with matplotlib.

> **Open item**: the exact licence under which `turbo` is redistributed has not been
> verified against a primary source in this repository. It is widely redistributed
> (matplotlib, GMT, ParaView) and was published by Google for reuse, but that is not the
> same as a confirmed grant. Before this package is redistributed outside the group,
> confirm the terms and record them here. Tracked on
> <https://github.com/rolker/marine_colormap/issues/12>.

---

## GeoZui4D colour lookup table — `hypsometric`

**Where**: `marine_colormap/src/topobathy_palettes.cpp`, `hypsometric_stops()`.
**Source**: GeoZui4D, `GeoZui4D/main/engine/TextureMaps/Clut.cpp`, `Clut::CreateDefault()`.
**Copyright**: 2000–2026 Center for Coastal and Ocean Mapping, University of New Hampshire.
**Licence**: Apache-2.0 — the same licence as this package, so no separate terms apply.

Recorded here for provenance rather than obligation. It is included in place of GMT's
`globe` palette, which carries the same classic hypsometric appearance but is **LGPL-3+**
and therefore cannot be redistributed inside an Apache-2.0 package.

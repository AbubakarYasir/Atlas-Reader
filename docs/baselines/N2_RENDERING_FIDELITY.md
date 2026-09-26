# N2 Rendering Fidelity / Geometry Baseline

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Canonical candidate evidence — N2 still Open**  
**Recorded:** 2026-09-23  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This file records the canonical synthetic rendering/geometry evidence for the Qt PDF and PDFium read/render candidates. It does not select the final renderer and it does not constitute `N2 PASS`.

## 1. Exact identity

Canonical implementation head:

`d9daf12cf3b1a7efc7118733279536585f8831fc`

Qualification runs on that exact implementation head:

- `N2 PDF Fidelity` run `35796991213` — PASS;
- normal Windows CI run `35796991256` — PASS;
- `N2 PDF Performance` run `35796991304` — PASS;
- `N2 qpdf Qualification` run `35796991284` — PASS.

Canonical fidelity artifact:

- name: `atlas-reader-n2-pdf-fidelity-d9daf12cf3b1a7efc7118733279536585f8831fc`;
- artifact ID: `10724751540`;
- artifact digest: `sha256:d37555963b15b739bd5ad1b47517a293f55df238810ae11ad7a59e9c5ba5831d`;
- artifact size: 4,638 bytes.

The fidelity workflow is isolated from the normal product shell. It builds focused Qt PDF/PDFium render probes only.

## 2. Candidate / package identity

- Qt PDF: **6.10.3**;
- PDFium: **156.0.8066.0 / `chromium/8066`**;
- PDFium probe package SHA-256: `739a57d597d864297909cc40a2411eba728490c76a0fa25e3ea299c7f6b07020`.

The PDFium community binary remains qualification-only evidence. This baseline does not approve that package as the production distribution route.

## 3. Deterministic A011 fixture

Fixture:

`A011_visual_geometry.pdf`

SHA-256:

`b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29`

A011 is generated from checked-in explicit PDF syntax with LF-delimited objects and a computed xref. It does not rely on serializer-defined PDF bytes. CI regenerates it and requires the exact SHA before qualification.

The fixture is deliberately synthetic and vector-based. It contains:

- three pages;
- explicit MediaBox/CropBox/Rotate cases;
- stable red/green/blue/yellow/magenta/cyan landmarks;
- an external-URI link annotation with an explicit normal appearance stream;
- no external font dependency.

A011 is licensed as Atlas-dedicated CC0-1.0 test data.

## 4. Independent geometry oracle

Pinned pypdf 5.9.0 independently validates the source PDF before either candidate engine is interpreted.

| Page | MediaBox | CropBox | Rotate | Expected effective visible points |
|---:|---|---|---:|---|
| 0 | `[0,0,200,300]` | `[0,0,200,300]` | 0 | `200 × 300` |
| 1 | `[0,0,300,200]` | `[50,25,250,175]` | 0 | `200 × 150` |
| 2 | `[0,0,200,300]` | `[0,0,200,300]` | 90 | `300 × 200` |

The independent oracle also confirms on page 0:

- exactly one link annotation;
- annotation rectangle `[70,125,130,175]`;
- URI `https://example.com/atlas-a011`;
- an explicit normal `/AP` appearance;
- appearance BBox `[0,0,60,50]`;
- magenta stroke command and six-point line width are present in that appearance stream.

Independent geometry validation reports `passed: true` with no failures.

## 5. Effective visible geometry

Both Qt PDF and PDFium expose the same effective visible sizes required by the independent raw geometry:

| Page | Qt PDF | PDFium | Result |
|---:|---|---|---|
| 0 | `200 × 300` pt | `200 × 300` pt | PASS |
| 1 | `200 × 150` pt | `200 × 150` pt | PASS |
| 2 | `300 × 200` pt | `300 × 200` pt | PASS |

This closes the current synthetic normalized CropBox/rotation geometry gate.

Raw MediaBox/CropBox/Rotate values are established independently by pypdf, not inferred from either engine's normalized page-size API.

## 6. Nominal and 2× raster fidelity

Each engine renders A011 at:

- **1×** — one output pixel per effective PDF point;
- **2×** — two output pixels per effective PDF point.

The acceptance contract intentionally does **not** require byte-identical raster hashes. Different antialiasing, pixel formats, alpha policy or implementation details may legitimately produce different raw bytes while preserving the same visible semantics.

Instead, qualification checks stable interior landmarks with a bounded channel tolerance of 24.

### Page 0 — normal orientation

Both engines produce the expected landmarks at 1× and 2×:

- top-left red;
- top-right green (`0,128,0`);
- bottom-left blue;
- bottom-right yellow.

### Page 1 — CropBox

Both engines produce the expected visible `200 × 150` page and the same crop-relative landmarks at 1× and 2×:

- magenta interior landmark;
- cyan interior landmark;
- empty/background samples at both crop corners.

This demonstrates that content outside the effective CropBox is not incorrectly shifted into the normalized visible page.

### Page 2 — 90° page rotation

Both engines produce `300 × 200` output and the expected rotated landmark orientation at 1× and 2×:

- top-left blue;
- top-right red;
- bottom-left yellow;
- bottom-right green.

This establishes the tested inherent 90° page-rotation render behavior.

## 7. Annotation rendering behavior

Page 0 contains an explicit normal annotation appearance with a magenta border.

For **both** engines and at both 1× and 2×:

- annotation rendering disabled → the annotation probe remains background;
- annotation rendering enabled → the annotation probe is magenta.

Therefore explicit-appearance annotation-off/on behavior is **PASS** on A011.

This does not yet qualify every annotation subtype, popup, widget, highlight, ink, stamp or annotation without an appearance stream.

## 8. Background / alpha policy difference

The native APIs differ at untouched pixels:

- Qt `QPdfDocument::render()` returns transparent untouched pixels in this qualification path (`RGBA 0,0,0,0`);
- the PDFium qualification probe owns its bitmap and deliberately pre-fills it opaque white (`RGBA 255,255,255,255`) before rendering.

This is not treated as a renderer failure. **Background/compositing must be an Atlas adapter policy**, not an accidental consequence of whichever engine is selected.

A future production render boundary should make at least these choices explicit:

- transparent vs opaque target;
- background color;
- premultiplication/pixel format normalization;
- whether annotations are rendered into the page bitmap.

## 9. Current capability result

| Capability | Qt PDF | PDFium | Bound limitation |
|---|---|---|---|
| Effective CropBox geometry | **PASS** | **PASS** | A011 synthetic vector corpus |
| 90° rotation geometry | **PASS** | **PASS** | A011 only; additional rotation/crop combinations still useful |
| Nominal 1× semantic raster fidelity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | solid vector landmarks, not arbitrary documents |
| 2× / high-density semantic raster fidelity | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | 2× synthetic vector evidence only |
| Annotation-off/on explicit appearance | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | one Link annotation with explicit `/AP` only |
| Native untouched background behavior | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | policies differ; Atlas normalization required |
| Cross-engine byte-identical pixels | **N/A** | **N/A** | explicitly not a requirement |
| Real Arabic/Urdu shaped-font visual fidelity | **PENDING** | **PENDING** | A006 is logical Unicode only; A011 uses simple vector/Helvetica content |
| Image/gradient/transparency-group fidelity | **PENDING** | **PENDING** | not present in A011 |
| Broad annotation subtype fidelity | **PENDING** | **PENDING** | current link appearance sentinel only |

## 10. Architectural implication

The tested renderer boundary should expose normalized intent rather than candidate defaults. At minimum Atlas should own:

- effective visible page geometry;
- output pixel size / scale;
- background/compositing policy;
- annotation inclusion policy;
- normalized pixel format for downstream consumers.

Candidate-native image/bitmap representations must not leak through the portable Atlas application/domain contract.

## 11. What this closes and what it does not

This evidence closes the previous N2 synthetic gates for:

- raw geometry oracle on MediaBox/CropBox/Rotate;
- effective visible CropBox/rotation size;
- nominal and 2× semantic vector rendering;
- tested inherent rotation orientation;
- explicit-appearance annotation on/off;
- documenting the native background-policy difference.

It does **not** close:

1. real-world Arabic/Urdu shaped-font raster fidelity;
2. image/gradient/transparency-heavy PDFs;
3. broad annotation subtype rendering;
4. normalized text/search-hit rectangles and destination coordinates;
5. renderer stress/leak behavior under long repeated page lifetimes;
6. final renderer responsibility selection.

N2 remains **Open** and ADR-0004 remains **Proposed**.

## 12. A014 owner-oracle correction (2026-09-27)

The first A014 artifact was not acceptable owner evidence. Its automated check
established that Qt PDF and PDFium produced similar rasters from the same PDF,
but the owner found the fully vocalized Arabic line difficult/broken to read and
correctly noted that Urdu was displayed with the same Arabic typeface. The
prior A014 result is withdrawn; A013 remains valid.

The replacement fixture uses separate pinned inputs:

- Noto Naskh Arabic for Arabic and tashkīl;
- Noto Nastaliq Urdu for Urdu;
- explicit Latin and Arabic runs on the mixed-script visual line.

Automated checks still verify deterministic generation, non-empty bands,
dimensions and cross-engine raster similarity. Human readability at 100% and
200% is now an explicit owner gate that automation cannot waive.

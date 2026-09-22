# N2 PDF Coordinate Normalization Baseline

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Canonical candidate evidence — N2 still Open**  
**Recorded:** 2026-09-23  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This file records the canonical coordinate-normalization evidence for the Qt PDF and PDFium read candidates. It does not select a final engine and it does not constitute `N2 PASS`.

## 1. Exact identity

Canonical implementation head:

`88c7badc5cf7bbcf4b7a211a0b2dda9c5218b08f`

Exact-head workflows:

- `N2 PDF Coordinates` run `35799660904` — PASS;
- normal Windows CI run `35799660800` — PASS;
- `N2 PDF Fidelity` run `35799660935` — PASS;
- `N2 PDF Performance` run `35799660885` — PASS;
- `N2 qpdf Qualification` run `35799660853` — PASS.

Canonical coordinate artifact:

- name: `atlas-reader-n2-pdf-coordinates-88c7badc5cf7bbcf4b7a211a0b2dda9c5218b08f`;
- artifact ID: `10724844302`;
- digest: `sha256:12226ea5eaa5d42f6c609f466e847c18606d46dea12758cf3d43578fda6ae494`;
- size: 3,388 bytes.

## 2. Atlas page-space contract

The qualification defines the portable application coordinate space as:

> effective visible page after crop/rotation; origin at the upper-left; X increases right; Y increases down; units are PDF points.

Candidate-native coordinates remain diagnostic evidence only. Atlas-owned domain/application types must use the normalized convention above.

PDFium normalization in this probe uses `FPDF_PageToDevice` at 100 device units per PDF point and divides back to point units. Qt PDF page/search rectangles are already expressed in effective page-point space, but raw `QRectF` values are normalized to positive width/height before entering Atlas page space.

## 3. Fixtures

### A003 — link source rectangles

A003 SHA-256:

`77aec987979e995b940efabc2faf34da62b19be8b7acb800b95f993a720f42d5`

The source PDF defines two explicit link annotations on page 0:

- internal link rectangle: `[72,660,250,680]` in PDF bottom-left coordinates;
- external link rectangle: `[72,600,280,620]` in PDF bottom-left coordinates.

In Atlas page space on the 612 × 792 point page, the expected rectangles are:

| Link | X | Y | Width | Height |
|---|---:|---:|---:|---:|
| internal → page 2 | 72 | 112 | 178 | 20 |
| external URI | 72 | 172 | 208 | 20 |

### A011 — normal/cropped/rotated search geometry

A011 SHA-256:

`b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29`

The same deterministic fixture used for rendering fidelity exercises:

- page 0: normal 200 × 300 point page;
- page 1: CropBox-normalized 200 × 150 point page;
- page 2: 90° rotated 300 × 200 point page.

Search queries are `A011 PAGE 1`, `A011 PAGE 2`, and `A011 PAGE 3` respectively.

## 4. Link source-rectangle normalization

Both engines normalize the explicit A003 annotations exactly to the expected Atlas coordinates.

### Internal link

| Engine | Normalized Atlas rectangle |
|---|---|
| Qt PDF | `x=72, y=112, w=178, h=20` |
| PDFium | `x=72, y=112, w=178, h=20` |

### External URI annotation

| Engine | Normalized Atlas rectangle |
|---|---|
| Qt PDF | `x=72, y=172, w=208, h=20` |
| PDFium | `x=72, y=172, w=208, h=20` |

Qt continues to expose a second raw row for the same external URI at `x=77, y=180, w=173, h=10`. This is the previously documented raw-link duplication behavior. It is preserved in evidence and does not replace the exact explicit annotation rectangle.

Therefore source-link rectangle normalization is **PASS**, with the existing Qt semantic de-duplication limitation still applying at the raw link-model layer.

## 5. Search-hit rectangle normalization

The cross-engine validator compares the union of each engine's normalized search rectangles. Tolerance is 4 points per X/Y/width/height field because text metrics may differ slightly while still describing the same visible hit region.

### Page 0 — normal

| Field | Qt PDF | PDFium | Absolute delta |
|---|---:|---:|---:|
| X | 68.00 | 68.00 | 0.00 |
| Y | 88.00 | 87.72 | 0.28 |
| Width | 60.00 | 60.43 | 0.43 |
| Height | 7.00 | 7.39 | 0.39 |

### Page 1 — cropped

| Field | Qt PDF | PDFium | Absolute delta |
|---|---:|---:|---:|
| X | 60.00 | 60.00 | 0.00 |
| Y | 73.00 | 72.72 | 0.28 |
| Width | 62.00 | 61.73 | 0.27 |
| Height | 7.00 | 7.39 | 0.39 |

### Page 2 — 90° rotated

| Field | Qt PDF | PDFium | Absolute delta |
|---|---:|---:|---:|
| X | 145.00 | 144.88 | 0.12 |
| Y | 68.00 | 68.00 | 0.00 |
| Width | 7.00 | 7.40 | 0.40 |
| Height | 62.00 | 61.81 | 0.19 |

The maximum observed field delta in the qualified corpus is **0.43 points**, comfortably inside the explicit 4-point tolerance.

All normalized rectangles also remain inside the effective visible page bounds.

## 6. Qt rotated-rectangle diagnostic

The first coordinate run (`35799414250`, implementation `cbe36011b3e36e136dfd1be4b4d4162a76649b6b`) exposed a useful raw Qt representation detail on A011 page 2:

`QRectF(x=152, y=68, width=-7, height=62)`

The rectangle described the correct visible region but used negative width after page rotation. The initial Atlas normalization accidentally copied the raw rectangle unchanged, so the semantic validator failed.

The canonical implementation preserves that raw rectangle for diagnostics but applies `QRectF::normalized()` before emitting Atlas page-space geometry. The resulting Atlas rectangle is:

`x=145, y=68, width=7, height=62`

That aligns with PDFium's independently converted result:

`x=144.88, y=68, width=7.40, height=61.81`

This establishes an important adapter rule: candidate-native rectangle orientation/sign is not portable Atlas geometry.

## 7. Current result

| Capability | Qt PDF | PDFium | Bound limitation |
|---|---|---|---|
| Link source rectangle normalization | **PASS** | **PASS** | A003 explicit link annotations |
| Search rectangle normalization — normal page | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A011 synthetic Helvetica text |
| Search rectangle normalization — CropBox page | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A011 only |
| Search rectangle normalization — 90° rotated page | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A011 only; Qt raw negative width requires canonicalization |
| Effective-visible page bounds | **PASS** | **PASS** | current A011 pages |
| Explicit destination `/XYZ` coordinate normalization | **PENDING** | **PENDING** | current A003/A011 destinations do not carry an explicit coordinate target |
| Arbitrary multi-rectangle/bidi/real-font hit geometry | **PENDING** | **PENDING** | current fixture is synthetic/simple text |

## 8. Architectural implication

Atlas must own the page-space conversion boundary. Engine-native rectangles, destination points and sign/orientation conventions must not leak into portable state.

The future normalized type should encode at least:

- zero-based page index;
- X/Y in effective visible page points from top-left;
- non-negative width/height;
- optional source/raw diagnostics outside the portable type;
- an explicit distinction between source link rectangles, search/extraction rectangles and destination points.

The Qt negative-width diagnostic proves why canonical rectangle normalization belongs at this boundary rather than in callers.

## 9. Remaining coordinate work

This baseline closes the current N2 source-link and search-hit geometry gates for normal, cropped and rotated synthetic pages.

Still pending:

1. explicit `/XYZ` destination-coordinate normalization;
2. broader real-font/multi-line/bidi text rectangles if required before final read-engine selection;
3. extraction-selection geometry if Atlas v2 requires it independently of search results.

N2 remains **Open** and ADR-0004 remains **Proposed**.

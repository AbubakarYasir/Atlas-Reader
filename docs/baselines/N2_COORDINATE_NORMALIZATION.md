# N2 PDF Coordinate Normalization Baseline

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Canonical candidate evidence — N2 still Open**  
**Recorded:** 2026-09-23  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This file records the canonical coordinate-normalization evidence for the Qt PDF and PDFium read candidates. It does not select a final engine and it does not constitute `N2 PASS`.

## 1. Exact identity

Canonical implementation head:

`0bbe1132872a946e9869945be20b8f9f174a1785`

Exact-head workflows:

- `N2 PDF Coordinates` run `35800654969` — PASS;
- normal Windows CI run `35800654960` — Debug/Release PASS;
- `N2 PDF Fidelity` run `35800654993` — PASS;
- `N2 PDF Performance` run `35800654956` — PASS;
- `N2 qpdf Qualification` run `35800655039` — PASS.

Canonical coordinate artifact:

- name: `atlas-reader-n2-pdf-coordinates-0bbe1132872a946e9869945be20b8f9f174a1785`;
- artifact ID: `10725573435`;
- digest: `sha256:bf2ea40d545dc73d8b1bc521b040292a690a4a3b2be3d2bf5f83530ef8ec2427`;
- size: 3,902 bytes.

The previous rectangle-only implementation `88c7badc5cf7bbcf4b7a211a0b2dda9c5218b08f` remains valid historical evidence. The current head supersedes it by adding explicit `/XYZ` destination-point qualification without regressing the earlier rectangle tests.

## 2. Atlas page-space contract

The portable application coordinate space is:

> effective visible page after crop/rotation; origin at the upper-left; X increases right; Y increases down; units are PDF points.

Candidate-native coordinates remain diagnostic evidence only. Atlas-owned domain/application types must use the normalized convention above.

PDFium normalization uses `FPDF_PageToDevice` at 100 device units per PDF point and divides back to point units. Qt PDF page/search rectangles are already in effective page-point space, but raw `QRectF` values must be normalized to positive width/height before entering Atlas page space.

For hyperlink destination **points**, Qt has an additional bounded limitation documented in section 7: its `QPdfLink` location does not expose inherent target-page rotation, and `QPdfDocument` does not expose that stored page-rotation value through the qualified public API surface. Rotated destination normalization therefore needs structural page-rotation metadata or another destination provider if Qt owns link reading.

## 3. Deterministic fixtures

### A003 — link source rectangles

SHA-256:

`77aec987979e995b940efabc2faf34da62b19be8b7acb800b95f993a720f42d5`

Expected Atlas source rectangles on page 0:

| Link | X | Y | Width | Height |
|---|---:|---:|---:|---:|
| internal → page 2 | 72 | 112 | 178 | 20 |
| external URI | 72 | 172 | 208 | 20 |

### A011 — normal/cropped/rotated search geometry

SHA-256:

`b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29`

A011 exercises:

- page 0: normal 200 × 300 point page;
- page 1: CropBox-normalized 200 × 150 point page;
- page 2: 90° rotated 300 × 200 point page.

### A012 — explicit `/XYZ` destination points

SHA-256:

`0c710f3f6e4457a44b8d2dbaa42a476a5c5b429764a0a7569002b66222ef069d`

A012 is generated from checked-in explicit PDF syntax. Page 0 has two internal links whose destination arrays both encode raw PDF point `/XYZ 40 250 null`:

- target page 1: normal 200 × 300 page;
- target page 2: stored `/Rotate 90`, effective 300 × 200 page.

Expected Atlas points:

| Target | Raw PDF `/XYZ` | Expected Atlas point |
|---|---|---|
| page 1, normal | `(40,250)` | `(40,50)` |
| page 2, 90° rotated | `(40,250)` | `(250,40)` |

## 4. Link source-rectangle normalization

Both engines normalize the explicit A003 annotations exactly.

### Internal link

| Engine | Atlas rectangle |
|---|---|
| Qt PDF | `x=72, y=112, w=178, h=20` |
| PDFium | `x=72, y=112, w=178, h=20` |

### External URI annotation

| Engine | Atlas rectangle |
|---|---|
| Qt PDF | `x=72, y=172, w=208, h=20` |
| PDFium | `x=72, y=172, w=208, h=20` |

Qt also exposes a second raw row for the same external URI at `x=77, y=180, w=173, h=10`. The previously documented semantic de-duplication requirement remains.

Source-link rectangle normalization is therefore **PASS** for both engines on A003.

## 5. Search-hit rectangle normalization

The cross-engine validator compares the union of normalized search rectangles with a four-point per-field tolerance for small text-metric differences.

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

Maximum observed field delta is **0.43 points**. All normalized rectangles stay inside effective visible page bounds.

## 6. Qt rotated-rectangle diagnostic

Exploratory run `35799414250` exposed raw Qt A011 page-2 search geometry:

`QRectF(x=152, y=68, width=-7, height=62)`

The region is correct but the width is negative after rotation. Atlas normalization must therefore canonicalize candidate-native rectangles.

The canonical adapter-level representation applies `QRectF::normalized()` and produces:

`x=145, y=68, width=7, height=62`

PDFium independently converts to:

`x=144.88, y=68, width=7.40, height=61.81`

This is **PASS WITH LIMITATION** for Qt raw rectangle representation and **PASS** for the Atlas-normalized rectangle contract.

## 7. Explicit `/XYZ` destination-point result

A012 provides a real engine difference that must remain visible in the architecture decision.

### Normal target page

Both engines reach the expected Atlas destination `(40,50)`.

- Qt native destination location: `(40,50)`;
- PDFium raw PDF location: `(40,250)`;
- PDFium Atlas-normalized location: `(40,50)`.

Result: **PASS both**.

### 90° rotated target page

PDFium exposes raw page-space coordinates and can normalize them through the target page transform:

- PDFium raw: `(40,250)`;
- PDFium Atlas-normalized: `(250,40)`;
- expected Atlas: `(250,40)`.

Result: **PASS PDFium**.

Qt returns the same native destination location `(40,50)` that it returns for the unrotated target. In other words, Qt has converted the PDF's bottom-left Y convention to a top-left point but has not incorporated the target page's stored inherent `/Rotate 90` into the destination location. The qualified `QPdfDocument` API provides the effective point size but not the stored page-rotation value needed to complete that transformation independently.

Result: **PASS WITH LIMITATION Qt PDF** — page identity and destination location are available, but a rotated target's final Atlas point requires inherent page-rotation metadata from a structural source (for example the Atlas structural layer) or a different destination provider.

This is not treated as a malformed fixture or tolerance issue. The first strict A012 run `35800288137` failed exactly the incorrect assumption that Qt and PDFium would both directly emit `(250,40)` for the rotated target; its artifact preserved the raw evidence that led to the bounded limitation classification.

## 8. Current capability result

| Capability | Qt PDF | PDFium | Bound limitation |
|---|---|---|---|
| Link source rectangle normalization | **PASS** | **PASS** | A003 explicit link annotations |
| Search rectangle — normal page | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | synthetic simple text |
| Search rectangle — CropBox page | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | A011 only |
| Search rectangle — 90° rotated page | **PASS WITH LIMITATION** | **PASS WITH LIMITATION** | Qt raw negative width requires canonicalization |
| Effective-visible page bounds | **PASS** | **PASS** | A011 corpus |
| `/XYZ` destination — normal target | **PASS** | **PASS** | A012 |
| `/XYZ` destination — 90° rotated target | **PASS WITH LIMITATION** | **PASS** | Qt requires external inherent-rotation metadata |
| Arbitrary destination modes (`Fit`, `FitH`, etc.) | **PENDING** | **PENDING** | not in current corpus |
| Arbitrary multi-rectangle/bidi/real-font hit geometry | **PENDING** | **PENDING** | current fixture is synthetic/simple text |

## 9. Architectural implication

Atlas must own the page-space conversion boundary. Candidate-native rectangles and destination points must not leak into portable state.

The normalized type should encode at least:

- zero-based page index;
- X/Y in effective visible page points from top-left;
- non-negative rectangle width/height;
- optional raw diagnostics outside the portable type;
- a distinction between source rectangles, text/search rectangles, and destination points;
- whether a destination includes explicit X/Y/zoom or represents a non-XYZ fit mode.

If Qt PDF is selected for navigation/link reading, the adapter must receive inherent page-rotation metadata from the structural layer before normalizing rotated explicit destination points. PDFium does not require that additional source for the tested A012 `/XYZ` case because its public destination + page transform APIs expose enough information directly.

## 10. Remaining coordinate work

The current N2 gates are closed for source-link rectangles, search-hit rectangles, and explicit `/XYZ` points on normal and 90°-rotated synthetic pages.

Still potentially useful, but not automatically a blocker unless required by Atlas v2 scope:

1. non-XYZ destination modes (`Fit`, `FitH`, `FitV`, etc.);
2. broader real-font/multi-line/bidi text rectangles;
3. extraction-selection geometry independently of search hits.

N2 remains **Open** and ADR-0004 remains **Proposed**.

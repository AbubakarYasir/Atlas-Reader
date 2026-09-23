# N2 PDF Scope Review

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Active scope decision — only explicitly listed blockers remain before responsibility assignment**  
**Date:** 2026-09-23  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This record prevents N2 from expanding indefinitely. It maps the remaining open evidence in `N2_PDF_ENGINE_MATRIX.md` to the binding Windows 2.0 product scope in `docs/FEATURE_SCOPE_2_0.md`.

N2 still does not constitute product Reader implementation. The purpose here is only to gather enough evidence to choose safe, replaceable PDF responsibilities for later checkpoints.

## Decision rule

A missing probe remains an **N2 blocker** only when all of the following are true:

1. the behavior is required by Windows 2.0 P0/P1 scope or is a hard safety/distribution gate;
2. the behavior could materially change the engine/responsibility selection;
3. later checkpoints would be expensive or unsafe if N2 selected the wrong responsibility now;
4. the current evidence does not already provide a bounded workaround/normalization contract.

Everything else becomes later hardening/regression coverage rather than an excuse to keep N2 permanently open.

## 1. Remaining N2 blockers

### B1 — image-only PDF reading/rendering

**Decision: N2 BLOCKER.**

Windows 2.0 Reader P0 explicitly states that an image-only PDF must remain readable while Atlas reports that no searchable text layer is available. OCR is deferred, but raster reading is not.

Required N2 evidence:

- deterministic image-only fixture;
- both read/render candidates open and render it correctly;
- text extraction/search absence is represented safely rather than treated as corruption;
- nominal/high-density raster sanity is preserved.

This does not require OCR.

### B2 — representative real-font Arabic/Urdu PDF rendering

**Decision: N2 BLOCKER.**

Arabic/RTL is a P0 cross-cutting Atlas requirement and the project explicitly targets Arabic/Urdu research material. A006 proves logical Unicode extraction/search but deliberately does not prove real embedded-font raster fidelity.

Required N2 evidence:

- redistributable/pinned Arabic-capable font provenance;
- deterministic PDF with embedded real font glyphs for Arabic, tashkīl, Urdu and mixed Latin/Arabic text;
- independent generation/reference provenance;
- Qt PDF and PDFium render non-corrupt, semantically equivalent visible output at at least 1× and 2×;
- this fixture may remain visual-only if extraction semantics are already covered independently by A006.

The test must not silently rely on an unpinned Windows system font.

### B3 — P0 outline/bookmark structural mutation breadth

**Decision: N2 BLOCKER for the structural-write responsibility.**

Windows 2.0 P0 requires complete bookmark editing: create, rename, delete, move/reparent, reorder siblings and nest/unnest. Current qpdf evidence proves preservation and title-only mutation, but that is not enough to select qpdf as the production structural writer.

Required N2 evidence:

- deterministic source outline with duplicate visible names and more than one nesting level;
- add a node;
- delete a node;
- reparent/move a node;
- reorder siblings;
- nest and unnest at least one node;
- preserve destination pages/coordinates and unrelated PDF state;
- include Unicode/Arabic or Urdu titles in the mutation path;
- validate with qpdf and an independent reader after each final output.

The fixture should exercise practical depth/duplicate-title semantics without inventing an arbitrary maximum-depth claim.

### B4 — encrypted writable structural mutation

**Decision: N2 BLOCKER if qpdf is selected for embedded bookmark writes.**

Windows 2.0 distinguishes open password from permissions/owner authority, forbids bypass, allows local-only fallback for restricted/non-writable documents, and requires retrying embed when capability becomes available.

Therefore the selected structural writer must prove that a permitted encrypted PDF can be structurally rewritten without silently dropping/changing encryption policy or unrelated content.

Required N2 evidence:

- use deterministic encrypted fixture(s) with public test passwords only;
- mutation is attempted only with the authority required by the fixture;
- output remains encrypted according to the expected policy unless the operation explicitly requests otherwise;
- user/owner/restriction state is re-inspected after write;
- intended outline mutation survives;
- unrelated preservation invariants remain intact;
- no password/restriction bypass behavior is introduced.

If the selected v2 policy instead forbids embedding into encrypted PDFs entirely and always uses local-only overlay, this blocker may be retired only by an explicit product/ADR scope decision. Current P0 wording does not make that narrower policy implicit.

## 2. Not N2 blockers — later hardening or later checkpoint evidence

### Fit / FitH / FitV destination modes

**Decision: POST-N2 HARDENING unless later Reader implementation proves they are required for a core source document.**

N2 already proves exact page destination identity, source rectangles, search geometry and `/XYZ` coordinates. Windows 2.0 P0 requires exact page destinations and Reader fit-width/fit-page modes, but it does not require preserving every PDF destination view-mode token as a domain semantic at engine-selection time.

### Unsupported-security-scheme fixture

**Decision: POST-N2 HARDENING.**

Qt PDF and PDFium both expose candidate-level unsupported/security error surfaces, while malformed/password behavior is already fixture-qualified. An unsupported-scheme fixture is useful regression coverage, but it is not currently expected to change responsibility selection.

Atlas still needs an `unsupported` application state in later Library/Reader workflows.

### Cryptographic signature validity verification

**Decision: OUT OF N2 / not a Windows 2.0 PDF-engine blocker.**

Windows 2.0 explicitly defers signature creation/certificate-management workflows. A010 already proves the required P0 safety boundary: signed/certified structure is detected before mutation and Atlas must never silently claim integrity survives a rewrite.

If a future version reports cryptographic validity, qualify a dedicated verifier then.

### Advanced image/gradient/transparency-group corpus

**Decision: POST-N2 HARDENING after the image-only blocker above.**

N2 needs representative raster reading, not exhaustive PDF graphics conformance testing.

### Broad annotation-subtype raster fidelity

**Decision: N6/P1 evidence, not N2.**

Annotations are a later product checkpoint. A011's explicit-appearance Link annotation is sufficient to establish the current rendering-policy boundary.

### Large-document viewport/cache behavior

**Decision: N3+ Reader architecture/performance evidence, not N2 engine selection.**

N2 stress already proves repeated engine lifetimes and PDFium serialization. Bounded visible-page caches/cancellation/resource retention belong to the production Reader/viewport checkpoint.

### Multi-hour soak / universal memory-leak threshold

**Decision: POST-N2 product stress.**

The canonical 500-cycle stress series remains a baseline. Qt's upward working-set trend must be retained as a follow-up signal, but N2 will not invent a leak verdict or universal RSS threshold from one hosted-runner series.

## 3. Provisional responsibility direction

This is a **proposal for the final ADR**, not an accepted decision yet.

Based on evidence captured so far, the direction to validate through B1–B4 is:

| Responsibility | Provisional implementation | Why this is the current direction |
|---|---|---|
| document open/read metadata | **PDFium** | correctness parity; direct low-level API; serialized execution has now been stress-qualified |
| page geometry/labels | **PDFium + Atlas normalization** | complete current geometry evidence, including direct rotated `/XYZ` normalization |
| page raster rendering | **PDFium** | current fidelity parity with Qt; one read engine avoids duplicate read/render stacks |
| text extraction | **PDFium + Atlas Unicode normalization** | current Unicode semantics agree; normalization is engine-independent Atlas policy |
| search | **PDFium + Atlas normalization/index layer** | semantic tests pass; direct result geometry is available; repeated timing evidence is substantially lower though execution models differ |
| links/navigation | **PDFium + Atlas normalized contracts** | current raw link model is simpler and rotated `/XYZ` normalization is complete without borrowing rotation metadata from another layer |
| outline read | **PDFium for reader-facing navigation** | keeps read/navigation responsibility in one serialized adapter |
| security/capability structural inspection | **qpdf** | strongest current permissions/encryption/signature-structure evidence |
| structural bookmark mutation/write | **qpdf** | preservation/title-mutation evidence is strong; B3/B4 must close before selection |
| independent post-write validation | **qpdf `--check` + independent Atlas test oracle in qualification** | already proven useful for mutation safety; production app must not depend on pypdf |

Qt PDF remains a qualified fallback/read candidate and the UI stack remains Qt. Not selecting Qt PDF for a PDF responsibility would not reject Qt itself; it would avoid carrying a second PDF read/render abstraction merely for integration convenience.

## 4. Why the provisional read direction is PDFium rather than Qt PDF

This is not based on one benchmark score.

Evidence currently favoring PDFium for the combined read/render/text/navigation responsibility includes:

- comparable correctness/fidelity on the deterministic core/render corpus;
- no Qt-style duplicate raw external-URI row in the tested link model;
- direct normalization of rotated `/XYZ` destinations rather than requiring target-page rotation metadata from another provider;
- much lower repeated search completion measurements on the current corpus, while preserving the caveat that Qt and PDFium search execution models differ;
- a smaller upward working-set trend in the canonical 500-lifetime synthetic stress series;
- the serialized multi-producer execution boundary has now been demonstrated with 1,000/1,000 successful jobs and no API overlap.

Evidence against immediate final selection:

- the current PDFium binary is a community qualification package, not the proposed production supply chain;
- an Atlas-owned pinned upstream source build + notices/SBOM + requalification still has to be frozen;
- B1/B2 representative rendering blockers still need to pass.

Qt PDF's principal remaining advantage is substantially simpler official integration/distribution inside Atlas's existing Qt stack. That advantage remains relevant until the PDFium production build route is actually proven.

## 5. Why the provisional structural direction is qpdf

qpdf already provides evidence for:

- structural checks/JSON inspection;
- encryption/user-owner/permission inspection;
- preservation-sensitive rewrites;
- ASCII and Unicode existing-outline title mutation;
- signed/certified pre-mutation safety detection;
- first-party Windows CLI provenance and process isolation.

B3 and B4 are the missing pieces before qpdf can be selected for the actual P0 embedded bookmark-writing responsibility.

## 6. Exit from scope review

This scope review is complete when:

- B1 image-only reading passes;
- B2 real-font Arabic/Urdu rendering passes;
- B3 structural bookmark mutation breadth passes;
- B4 encrypted writable structural mutation passes or is explicitly retired by a narrower product decision;
- the production route for the selected read engine and qpdf is frozen/requalified;
- ADR-0004 is revised with the final responsibility assignment;
- final exact-head strict CI is green;
- the owner explicitly records `N2 PASS`.

Until then ADR-0004 remains **Proposed**, PR #4 remains draft/open/unmerged, and N3 remains **Not started**.

# N2 PDF Scope Review

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Scope blockers satisfied — production-route freeze, final exact-head CI and overall owner acceptance remain**
**Date:** 2026-09-27
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

**Decision: SATISFIED.**

Windows 2.0 Reader P0 explicitly states that an image-only PDF must remain readable while Atlas reports that no searchable text layer is available. OCR is deferred, but raster reading is not.

Required N2 evidence:

- deterministic image-only fixture;
- both read/render candidates open and render it correctly;
- text extraction/search absence is represented safely rather than treated as corruption;
- nominal/high-density raster sanity is preserved.

This does not require OCR.

The deterministic A013 image-only fixture passes in both Qt PDF and PDFium at
1x and 2x. Both candidates produce a non-blank raster while reporting no text
or search hits. Canonical content-fidelity run `35813475471` passed at
implementation `213efb595b438e5b967c2e499dcb061eff8a8664`; artifact
`10730227985`, digest
`sha256:669e90dc03bf99a315b202dabc89e0ecab84d4ab38c54b7b0f58f1744b0a996b`.

### B2 — representative real-font Arabic/Urdu PDF rendering

**Decision: SATISFIED after corrected fixture and owner retest.**

Arabic/RTL is a P0 cross-cutting Atlas requirement and the project explicitly targets Arabic/Urdu research material. A006 proves logical Unicode extraction/search but deliberately does not prove real embedded-font raster fidelity.

Required N2 evidence:

- redistributable/pinned Arabic-capable font provenance;
- deterministic PDF with embedded real font glyphs for Arabic, tashkīl, Urdu and mixed Latin/Arabic text;
- independent generation/reference provenance;
- Qt PDF and PDFium render non-corrupt, semantically equivalent visible output at at least 1× and 2×;
- this fixture may remain visual-only if extraction semantics are already covered independently by A006.

The original A014 used Amiri for both Arabic and Urdu. Automated comparison
proved only that Qt PDF and PDFium rendered the same source similarly; it did
not prove that the source itself was readable or typographically appropriate.
On 2026-09-27 the owner rejected the original oracle because the fully
vocalized Arabic line read poorly/broke visually and Urdu was rendered in an
Arabic rather than Urdu-appropriate face. Therefore run `35813475471` and
artifact `10730227985` remain valid for B1/A013 only and no longer close B2.

The first replacement joined the letters but misplaced the diacritics and was
also owner-rejected. Manual presentation-form reshaping is excluded. The next
A014 pins Noto Naskh Arabic for Arabic and Noto Nastaliq Urdu for Urdu and uses
Qt's complete Unicode text-layout path to position base glyphs and combining
marks together before writing the PDF. Owner review confirmed that improvement
but found the Arabic/Urdu paragraphs physically left-aligned despite correct
RTL word order. The generator now requires absolute physical right alignment,
backed by automated right-edge assertions. B2 remained open until the corrected
artifact passed CI and the owner accepted its 100%/200% rendering.

Corrected implementation
`6ac322d9ddb4c9f53da158acf69265d65cdd5106` passed content-fidelity run
`36274881956`; artifact `10917256207`, digest
`sha256:5976b05c903f1babd22f60da87bc78b1b06cd51604572b03f37a8b22537e96b`.
The validator confirmed both engines' Arabic/Urdu bands reached the physical
right margin at 1x and 2x. The owner then reported the corrected artifact was
working from their visual review, closing the bounded A014 owner gate.

### B3 — P0 outline/bookmark structural mutation breadth

**Decision: SATISFIED.**

Windows 2.0 P0 requires complete bookmark editing: create, rename, delete, move/reparent, reorder siblings and nest/unnest. Title-only mutation was therefore insufficient evidence by itself.

The dedicated qpdf breadth probe closes this gap.

Exact implementation head:

`21f3226b12b5f6b2aba37ae8b975f3882e1d4c43`

Canonical run:

- `N2 qpdf Outline Breadth` run `35811968277` — **PASS**.

Artifact:

- ID `10729912313`;
- digest `sha256:633b09c9820177002abb6bb486da455c46e6dc7bad38435195fde9ca93c83c0b`.

Source fixture:

- A003 SHA-256 `77aec987979e995b940efabc2faf34da62b19be8b7acb800b95f993a720f42d5`.

Output:

- mutated PDF SHA-256 `6c740bebfa83ebe900e0b68b3788cbdbd6e5ef2a6d60380fccd765f1c752cad6`;
- qpdf rebuild exit `0`;
- rewritten `qpdf --check` exit `0`;
- qpdf JSON reopen PASS;
- independent pypdf snapshot exactly matches the intended final outline.

The single deterministic mutation proves:

- add a new Unicode node (`فوائد`);
- delete an existing node;
- move/reparent an existing node;
- reorder siblings;
- nest a new child;
- unnest an existing child;
- preserve duplicate visible titles as distinct structural nodes.

Expected and actual final outline agree exactly:

1. `Chapter 1`, depth 0, destination page 1;
2. `فوائد`, depth 1, destination page 2;
3. `Chapter 1`, depth 0, destination page 0.

Every asserted unrelated invariant remains equal before/after:

- encryption state;
- page count and page labels;
- page text hashes;
- MediaBox/CropBox/rotation and decoded page-content streams;
- existing annotations;
- Document Info;
- XMP;
- attachments.

All seven workflows on the same implementation head are green: Windows CI `35811968235`, Performance `35811968246`, Coordinates `35811968234`, Stress `35811968244`, Fidelity `35811968249`, qpdf Qualification `35811968263`, and qpdf Outline Breadth `35811968277`.

B3 is therefore closed as an N2 blocker. Production bookmark transactions, undo/reconciliation, conflict handling and safe-save remain N5 responsibilities.

### B4 — encrypted writable structural mutation

**Decision: SATISFIED for the proposed qpdf structural-write responsibility.**

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

The repaired A015 fixture has an explicit valid empty page resource dictionary
and SHA-256
`f01c4422597f015eedd3c4216ba0097de3f6d90ee7f943d9a703a1ad84954694`.
The source and rewritten output both pass strict qpdf checking. The mutation
preserves encryption V/R/P, wrong/user/owner password identity, permissive
capabilities, page and content invariants, metadata/XMP/attachments, and the
intended Unicode outline title.

Canonical implementation `28a072bbedf07808773068941fc8c7ed22252ba8`;
`N2 qpdf Encrypted Write` run `36271839766` — **PASS**; artifact
`10916290827`, digest
`sha256:1ed550fa9eae089710ee5c2891bf9163f5477e6ab056afa23512a0d73aad4151`.
Restricted A009 remains intentionally non-writable; this evidence does not
authorize bypassing PDF permissions.

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

With B1 through B4 now satisfied, the proposed final responsibility assignment is:

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
| structural bookmark mutation/write | **qpdf** | preservation, full P0 outline mutation breadth and permitted encrypted rewrite now pass |
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

Qt PDF's principal remaining advantage is substantially simpler official integration/distribution inside Atlas's existing Qt stack. That advantage remains relevant until the PDFium production build route is actually proven.

## 5. Why the provisional structural direction is qpdf

qpdf now provides evidence for:

- structural checks/JSON inspection;
- encryption/user-owner/permission inspection;
- preservation-sensitive rewrites;
- ASCII and Unicode existing-outline title mutation;
- add/delete/reparent/reorder/nest/unnest outline operations with duplicate titles and Unicode content;
- signed/certified pre-mutation safety detection;
- first-party Windows CLI provenance and process isolation.

B4 now passes for permitted encrypted writes. Restricted, signed/certified,
read-only and conflicting documents still use the later local-overlay/safe-save
policy; N2 does not implement that production workflow.

## 6. Exit from scope review

The scope review is complete: B1, corrected B2, B3 and B4 all have bounded PASS
evidence. N2 still requires the selected PDFium/qpdf production routes to be
frozen and requalified, ADR-0004 to remain synchronized with that result, final
exact-head strict CI, and explicit overall owner `N2 PASS`.

Until then ADR-0004 remains **Proposed**, PR #4 remains draft/open/unmerged, and N3 remains **Not started**.

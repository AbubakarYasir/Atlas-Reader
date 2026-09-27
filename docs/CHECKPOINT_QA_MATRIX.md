# Atlas Reader Native — Checkpoint QA and Acceptance Matrix

This file is the binding test map for `CHECKPOINTS.md`. It prevents a feature,
known limitation, or safety obligation from being postponed without a named
checkpoint, evidence requirement, and owner decision.

## Acceptance rule for every checkpoint

A checkpoint advances only when all seven conditions are true:

1. its written scope is implemented and its exclusions were respected;
2. required automated gates pass on the exact proposed branch head;
3. required manual, visual, accessibility, or hardware checks are recorded;
4. every known limitation is either fixed or transferred to a named later
   checkpoint with a blocking test; and
5. every changed interface has been inspected as a rendered product in its relevant populated, empty, failure, narrow, scaled and RTL states—not approved from code or a single happy-path screenshot; and
6. the owner explicitly records `N# PASS`.
7. changes to shared book identity, Library, metadata, search, progress, backup or UI models have been checked for accidental PDF-only assumptions; format-specific PDF behavior stays behind explicit capabilities/adapters, without implementing deferred formats prematurely.

Green CI proves only the automated portion. It never substitutes for the owner
gate. A skipped required test is a failure unless the checkpoint contract marks
it not applicable and records why.

Each accepted checkpoint must publish the evidence fields defined in
`QUALITY_AND_TESTING.md`, including the commit, artifact/checksum, tests,
fixtures, performance, Arabic/RTL/accessibility results, known limitations and
owner result.

## Program matrix

| Checkpoint | Product proof required | Automated QA required | Manual/owner proof required | Hard blockers |
|---|---|---|---|---|
| N0 Bootstrap | Repository, architecture, scope and build path exist without premature product code | clean configure, Release build, CTest and source/version checks | owner accepts architecture, scope and checkpoint program | missing build path, contradictory scope or undocumented architecture |
| N1 Windows baseline | Reproducible empty shell and named hardware/toolchain baseline | Debug/Release build and tests, lifecycle diagnostics, packaged smoke | startup series, Arabic/RTL, keyboard, theme, scale and lifecycle on physical Windows hardware | unreproducible toolchain, unacceptable shell regression or failed owner gate |
| N2 PDF qualification | PDF responsibilities are selected through comparable correctness, fidelity, security, preservation, stress and distribution evidence | normalized engine contract/fixture tests, rendering/text/search/navigation/security/mutation suites, stress, selected-route package/provenance/SBOM gate, exact-head strict CI | owner inspects image-only and real-font Arabic/Urdu artifacts and reviews the responsibility decision | unresolved engine-selection, safety, licensing or distribution blocker; missing `N2 PASS` |
| N3 Library/index | A real multi-root library is useful without Flutter and never destroys state because a root is unavailable | schema migration/rollback, repositories/FTS, scan cancellation, overlapping/reparse roots, identity/reconciliation, offline/corrupt/encrypted/Unicode fixtures, performance smoke; watcher coalescing becomes mandatory if an automatic watcher is introduced | representative Arabic/English library on SSD plus removable/offline or slower storage; rename/move/copy and recovery checklist; keyboard, 200%, narrow and Arabic rendered review | destructive reconciliation, UI-thread scanning/SQL, wrong-copy identity, failed migration, unusable search, generic/broken UI or inaccessible primary action |
| N4 Reader | Daily PDF reading is accurate, responsive and bounded in memory | open/error/session contracts, viewport/cache/eviction/cancellation, page geometry/rotation, links/destinations including duplicate-row normalization and rotated targets, text/search, multi-tab restore opt-in, long-PDF performance | read image-only, Arabic/Urdu, long, mixed-size/rotation and link-heavy PDFs at 100%/200%; keyboard and Narrator route; repeated tab/open/close soak | wrong navigation, broken shaping/direction, unbounded cache/memory, repeated UI stalls, inaccessible core reading path |
| N5 Resilience/bookmarks | Deep research navigation survives protected, read-only, offline and conflicting documents without false embed claims | capability preflight, overlay persistence, full tree editing, 10k-node search, reconciliation/conflicts, safe-save failure injection, encrypted-write preservation, versioned import/export | normal/restricted/read-only/locked/signed/certified/external-change/offline fixtures; Arabic/English duplicate/deep trees; independent-reader round trip | research loss, restriction bypass, blind overwrite, false saved/embedded state, wrong hierarchy/destination or non-recoverable conflict |
| N6 Ink/annotations | Writing feels direct and round-trips through standard PDF structures where permitted | stroke/page coordinate, undo/redo, gesture separation, overlay persistence, annotation serialization and safe-save regression suites | real pen/touch latency, zoom/pan/draw behavior, restricted/signed fallback and independent-reader reopen of ink/markup | dropped/corrupt strokes, unsafe original mutation, reader collapse while drawing, failed independent round trip |
| N7 Desktop utilities | Printing, covers, metadata, Windows integration, settings and diagnostics work without weakening core workflows | cover queue/invalidation, metadata preservation, print-range/layout model, launch/open-with/drag-drop/clipboard/long-path/session-close behavior, settings persistence/defaults, log privacy/redaction tests | physical print/preview, picker/open-with/context/drag-drop/clipboard, settings review, diagnostic-bundle preview, source-hash confirmation after print | source mutation by print, metadata collateral loss, private content in logs, dirty-work loss on close or core Index/Reader/Bookmark regression |
| N8 Migration/backup | Existing users can move safely and restore app-local research on a clean profile | versioned backup validation, clean restore, legacy read-only import, unresolved identity, idempotency, corrupted/partial input and interruption tests | copied representative legacy profile, side-by-side rollback and clean-machine/profile restore | legacy source mutation, filename-only relink, missing research, non-repeatable import or unverified restore |
| N9 Arabic/RTL/accessibility | Every implemented core workflow is qualified, not merely translated | localization completeness, bidi/Unicode round trips, keyboard/focus/UIA assertions, scale/high-contrast regression suite | Arabic UI; Arabic/English/Urdu mixed data; keyboard-only; Narrator; Accessibility Insights; 100%/200%; high contrast; reduced motion | any untested/failed core workflow, broken joining/diacritics/direction, inaccessible command or clipped/scaled core UI |
| N10 RC qualification | Feature-complete Windows build meets safety, performance, hardware, packaging and legal gates | full regression, hostile/fuzz targets available by then, prolonged soak, installer/upgrade dry runs, dependency/security/license/SBOM and reproducible package checks | large library, long PDFs, 10k bookmarks, SSD/HDD/external, pen/touch, multi-DPI/monitor, printer, interruption/conflict/offline and prolonged-session matrix | missing required hardware evidence, failed P0 threshold, unbounded memory, safety/security/licensing blocker or unexplained skipped core test |
| N11 Stable | The accepted RC is promoted without changing its behavior or damaging user data | byte/source equivalence, clean install/upgrade/uninstall, checksum/tag/artifact/notices verification | final owner acceptance of the exact RC and uninstall/user-data safety | payload drift after RC, missing archive/evidence, deleted user books/research, failed final owner gate |

## Known N2 limitations and their binding owners

These are not vague “later” tasks. Each item has an implementation checkpoint
and a final release checkpoint.

| N2 observation | Implementation owner | Required regression evidence | Final blocker |
|---|---|---|---|
| Qt PDF exposes duplicate raw URI rows | N4.4 navigation adapter | semantic link de-duplication contract test plus link-heavy fixture | duplicate user-visible action or wrong target blocks N4/N10 |
| Rotated explicit destinations need structural rotation metadata | N4.4 navigation adapter | normal/cropped/90°/180°/270° target fixtures with exact destination assertions | wrong page/point blocks N4/N10 |
| Synthetic Qt search was slower than PDFium | N4.4 reader search | long-document p50/p95/p99, cancellation and result correctness | normal-use search stall or missed result blocks N4/N10 |
| Qt working-set trend needs real-reader proof | N4.2 viewport/cache and N10 soak | 2,000-page, multi-tab and repeated open/close memory series with cache budget/eviction evidence | unbounded or page-count-proportional raster memory blocks N4/N10 |
| Fit/FitH/FitV tokens were not required for engine selection | N4.3/N4.4 navigation | representative destination-mode fixtures and documented normalized behavior | wrong core navigation blocks N4 |
| Broader graphics and annotation rendering corpus remains | N4 rendering and N6 annotations | image/gradient/transparency plus supported annotation-subtype corpus | user-visible corruption in supported workflows blocks owning checkpoint |
| Security-handler breadth and unsupported-security state are bounded | N5.1 capability preflight | modern encrypted/unsupported fixtures with explicit non-bypass states | ambiguous authorization or bypass blocks N5/N10 |
| Cryptographic signature validity is not verified by Atlas 2.0 | N5.1 policy and N5.4 safe save | signed/certified detection, local-only/default working-copy behavior and no validity claim | silent mutation or false verification claim blocks N5/N10; signature validation remains explicitly out of scope |
| A014 proves one corrected Arabic/Urdu page, not the whole corpus | N4 continuous reader fixtures and N9 final matrix | multiple real-font Arabic/Urdu PDFs with joining, diacritics, RTL alignment, search and mixed direction | broken core Arabic/Urdu workflow blocks N4/N9/N10 |

## Defect and carry-forward rule

Every owner/beta defect receives one of these before the checkpoint can close:

- an automated regression test linked to the fix;
- a numbered manual checklist item when automation cannot prove it; or
- a documented scope decision naming the later checkpoint and its blocking
  evidence.

“Test later,” “known limitation,” and “future hardening” without those details
are invalid handoff language.

## Competitive proof

For N3 through N10, the handoff includes one same-fixture workflow comparison
against relevant mature products. Record product/version/date, the shared task,
measured or observed result, and what Atlas must improve. Comparisons are for
learning and regression prevention; they do not authorize unsupported “best” or
“faster than” marketing claims.

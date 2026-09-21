# Atlas Reader Native — Success Metrics

Atlas is competing with mature PDF products, so progress cannot be measured by commit count, screenshots, or feature count. This document defines how we measure whether the native rewrite is actually becoming a better product.

## 1. Product scorecard

Windows 2.0 is successful only if the core workflows are dependable enough for daily research use.

### Library / Index

Measure:

- time to first useful library state after launch;
- progressive scan throughput;
- UI responsiveness during scan;
- search latency;
- correctness under offline roots, moved files, copies, encrypted/corrupt files;
- zero destructive purge from transient root failure;
- reliable Arabic/Unicode search and display.

Release target examples are defined in `PERFORMANCE.md`; checkpoint reports record actual measurements.

### Reader

Measure:

- cold/warm open time for representative PDFs;
- first visible page latency;
- scrolling frame pacing;
- zoom responsiveness;
- page-jump latency;
- search result navigation latency;
- memory usage versus document length;
- stability over long sessions and multiple tabs;
- mixed page size/rotation correctness.

A 2,000-page PDF must not consume memory proportional to 2,000 full-resolution rendered pages.

### Bookmarks / Outlines

Measure:

- correctness of deep trees;
- 10,000-node search/navigation latency;
- create/edit/move/reorder/nest responsiveness;
- round-trip fidelity in writable PDFs;
- no research loss in restricted/read-only/signed/offline/conflict scenarios;
- import/export fidelity;
- Arabic/mixed-direction correctness;
- independent-reader interoperability.

### Ink / annotations

Measure:

- input-to-visible-stroke latency on real pen hardware;
- stroke fidelity after save/reopen;
- standard-reader interoperability;
- undo/redo correctness;
- no reader frame collapse while drawing;
- safe local fallback on non-writable documents.

## 2. Reliability metrics

P0 acceptance requires:

- no known silent data-loss path;
- no false "saved/embedded" success;
- no blind overwrite of a changed external PDF;
- no destructive deletion because a drive/root is temporarily unavailable;
- no crash that corrupts the source document in tested save-failure scenarios;
- local research survives forced termination/restart in tested persistence scenarios;
- migrations/backups have deterministic restore evidence.

Crash-free percentage may be tracked later, but a high aggregate crash-free rate does not excuse a reproducible data-loss bug.

## 3. Performance budget metrics

Canonical benchmark reports should record at minimum:

- startup p50/p95;
- search p50/p95/p99 where meaningful;
- page render/open p50/p95;
- UI frame-time distribution under scroll/zoom;
- working set/private bytes;
- CPU usage during idle/scan/render;
- storage I/O during index and save;
- pen latency when hardware qualification begins.

Every metric includes hardware, OS, display refresh, storage type, build mode, fixture identity, compiler/Qt/dependency versions, and commit SHA.

## 4. Quality metrics

Do not chase test-count vanity. Track coverage by risk/contract.

Each accepted checkpoint must show:

- required unit/domain contracts covered;
- boundary/integration tests present;
- representative fixture classes covered;
- failure/recovery scenarios covered;
- Arabic/RTL/accessibility matrix updated for user-facing work;
- performance regression comparison to previous accepted checkpoint;
- unresolved known issues explicitly listed.

For PDF mutation, preservation tests are more important than generic line coverage.

## 5. Competitive evaluation

Atlas does not need to "beat" Adobe/Foxit in total features. We compare the workflows Atlas chooses to own.

For each beta, perform a structured manual comparison against relevant current versions of mature readers/editors using the same fixture where practical.

Compare:

- launch/open responsiveness;
- navigation/zoom/scroll usability;
- outline readability/editing depth;
- bookmark search and organization;
- protected/read-only behavior;
- data portability;
- Arabic/RTL behavior;
- accessibility/keyboard behavior;
- memory footprint on long documents;
- clarity of failure/recovery states.

Do not publish unsupported superiority claims. Record observations and screenshots/measurements internally as evidence.

## 6. Beta exit metrics

### `2.0.0-beta.1` — Index

Success means a real library can be managed/searchable daily without Flutter and without destructive state mistakes.

### `2.0.0-beta.2` — Reader

Success means normal daily PDF reading is comfortable, responsive, and bounded in memory on the baseline machine.

### `2.0.0-beta.3` — Bookmarks

Success means Atlas has a credible signature research workflow: deep outlines/bookmarks, local fallback, reconciliation, portability, and safe embedding.

### `2.0.0-beta.4+`

Each later beta must preserve or improve the core metrics. New features are not allowed to make Index/Reader/Bookmarks materially worse without an explicit accepted tradeoff.

## 7. Release blocker classes

A stable 2.0 release is blocked by any unresolved P0 issue in these classes:

- data loss/corruption;
- security/restriction bypass;
- external-change overwrite;
- unusable core accessibility path;
- broken Arabic/RTL core workflow;
- reproducible severe UI stall in normal baseline use;
- unbounded memory growth in defined reader scenarios;
- migration/restore corruption;
- licensing/distribution blocker;
- missing required installer/uninstall safety.

## 8. Regression rule

Every accepted checkpoint becomes a baseline.

If a later checkpoint regresses a core metric materially:

1. reproduce and measure;
2. identify the cause;
3. fix it, or document the tradeoff;
4. obtain explicit owner acceptance before moving the gate.

"More features" is not sufficient justification for making the primary workflows slower or less safe.

## 9. Success hierarchy

When priorities conflict, use this order:

1. **Research data safety and integrity**
2. **Correctness/interoperability**
3. **Core workflow responsiveness**
4. **Accessibility/Arabic/UX clarity**
5. **Resource efficiency**
6. **Feature breadth**
7. **Visual polish**

Visual polish still matters, but it cannot hide failures above it.
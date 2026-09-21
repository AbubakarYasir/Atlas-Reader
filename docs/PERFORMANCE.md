# Performance Contract

Atlas Reader Native is being rewritten partly to gain tighter control over latency and rendering. That does **not** justify claims such as “cannot lag.” Performance is a measured product requirement.

## Principles

1. UI/render thread stays bounded.
2. Work is prioritized by what the user can currently see/do.
3. Memory use scales with viewport/cache policy, not total document page count.
4. Background indexing never gets to make foreground reading feel broken.
5. Benchmarks record hardware, OS, build, fixture, cache state, and percentiles—not only best-case averages.
6. A performance win that breaks correctness/preservation is a regression, not an optimization.

## Baseline hardware policy

N1 records one named **baseline Windows machine** and one weaker/secondary machine when available. Final absolute gates are tied to named hardware rather than pretending every device should have identical latency.

Every benchmark report includes:

- CPU/GPU;
- RAM;
- SSD/HDD type;
- Windows build;
- display resolution/scale/refresh;
- power mode;
- compiler/Qt/dependency versions.

## Provisional user-experience targets

These are engineering targets to guide design before measured N1/N4 thresholds are finalized. Missing a target triggers investigation; final release gates are recorded after representative fixtures/hardware exist.

| Scenario | Provisional target |
|---|---|
| GUI callbacks during normal interaction | usually <2 ms; no normal synchronous app task >4 ms |
| 60 Hz frame budget | 16.67 ms total; Atlas CPU-side UI work leaves margin |
| Warm empty-shell first frame | target <800 ms p95 on baseline SSD |
| Cold empty-shell first frame | target <1.5 s p95 on baseline SSD |
| Cached library usable after shell | target <1 s for representative indexed profile |
| Command Center common query | target <50 ms p95 for representative library |
| 10,000-bookmark search | **<100 ms p95** qualified gate |
| Reader page jump feedback | selection/viewport response within one frame; render may complete async |
| Normal visible-page render after cached/open session | target <250 ms p95 at reading resolution on baseline hardware |
| Reader scrolling | no repeated long UI-thread stalls; 60 Hz target on baseline hardware |
| Background scan while reading | no meaningful foreground frame/input regression beyond agreed tolerance |

Do not advertise these externally until Verified on published hardware/fixtures.

## Frame budget

At 60 Hz a frame is 16.67 ms. Atlas keeps application-side UI work comfortably below that to leave compositor/GPU margin.

- target most GUI callbacks <2 ms;
- no normal synchronous app task >4 ms;
- no directory/PDF/database serialization work on GUI thread;
- smooth resize/scroll at 60 Hz on baseline hardware;
- 120 Hz/pen devices qualified separately.

Any repeated >16.67 ms application-side frame requires profiling and either a fix or explicit documented exception.

## Startup

Measure separately:

1. process start → first visible frame;
2. first frame → cached library usable;
3. background refresh completion.

The app shows useful shell/cached state before scan/PDF work completes. Startup never waits for full library enumeration.

N1 owns the empty-shell baseline; later checkpoints must report regression against it.

## Library/index

Architecture:

- directory enumeration off UI thread;
- bounded metadata workers;
- progressive commits/results;
- FTS for global search;
- writer batching;
- unavailable roots skip destructive reconciliation;
- watcher events coalesced/debounced;
- foreground reader/search tasks outrank scan/prefetch.

Benchmark fixtures:

- 1k books;
- 10k books;
- stress fixture larger than expected normal collection;
- SSD and representative HDD/external storage;
- Arabic/Unicode long paths;
- overlapping roots;
- encrypted/corrupt files;
- disconnected root;
- rename/move/copy reconciliation.

Record:

- time to first 50/100 visible indexed results;
- full scan time;
- UI frame/input responsiveness during scan;
- peak worker count/CPU;
- DB size/write rate;
- rescan/no-change cost.

A full scan may take time; **time to useful progressive results and foreground responsiveness matter more than one total-duration number.**

## Search

- common queries should return first useful results near-instantly;
- 10,000-bookmark search remains under **100 ms p95** on qualified baseline hardware;
- cancellation/new query supersedes stale expensive search;
- ranking/context construction must not push work onto UI thread;
- Arabic normalization/tokenization benchmarks use representative queries.

## Reader rendering

Never eagerly raster all pages.

Priority:

1. visible page(s);
2. immediate next/previous;
3. user-requested thumbnails;
4. predictive prefetch;
5. background cache.

Cache keys include document revision, page, scale bucket, rotation/crop/theme inputs as necessary.

Do not keep duplicate CPU and GPU full-resolution copies without a measured reason.

N4 reports at minimum:

- time to first readable page;
- page-jump feedback/render latency;
- sustained scroll frame pacing;
- cache hit/miss behavior;
- peak memory on long PDF;
- tab-switch latency;
- resize/zoom behavior;
- cancellation effectiveness after fast scrolling.

## Zoom/scroll

Zoom gestures reuse available textures temporarily and request sharper renders asynchronously. Do not synchronously re-raster every gesture delta.

Fast scrolling may display a lower-resolution/placeholder page briefly rather than block input. Quality catches up according to priority.

## Ink latency

Pointer samples flow to the live stroke overlay directly. Per-sample paths do not touch PDF serialization, SQLite, full-page rasterization, or heavyweight allocation.

Measure input-to-visible-stroke latency with real pen hardware in N6/N10. Mouse simulation is useful for correctness but cannot substantiate stylus-latency claims.

Record:

- p50/p95 visible stroke latency where measurement setup permits;
- dropped/coalesced samples;
- frame time during long strokes;
- behavior while page rendering/background scan is active.

## PDF save

Save may be expensive but never freezes the shell.

Measure phases separately where possible:

- serialize/temp write;
- validation;
- backup/replace;
- reopen/re-index.

Long phases provide clear progress/state. Cancellation is offered only where safe; cancel must never leave false committed state.

## Memory

Track at minimum:

- idle shell;
- indexed representative library;
- one 2,000-page PDF;
- multiple open tabs;
- page texture cache;
- thumbnail/cover cache;
- large 10k bookmark tree;
- temporary save buffers;
- prolonged session open/close cycles.

Caches have explicit budgets/eviction. “RAM is available” is not an eviction policy.

N4/N10 must demonstrate that opening a 2,000-page PDF does not allocate memory proportional to 2,000 full-resolution page images.

## I/O and HDD behavior

N3/N10 test representative HDD/external storage because random metadata access and aggressive concurrency can destroy responsiveness on slow media.

Worker concurrency may adapt to storage/observed queue pressure rather than using one fixed “maximum threads” value everywhere.

## Benchmark hygiene

Every recorded benchmark includes:

- commit SHA/version;
- Release configuration;
- compiler/Qt/dependency versions;
- CPU/GPU/RAM/storage;
- Windows version/display refresh/scale;
- fixture identity/checksum/size;
- warm/cold cache state;
- p50/p95/p99 when meaningful;
- iteration/sample count;
- profiler trace/reference when investigating a regression.

Do not compare Debug against Release or different machines without saying so.

## Regression policy

A regression is investigated when:

- it crosses an accepted threshold;
- repeated measurements show material degradation in a core workflow;
- memory becomes unbounded/proportional to total document/library size unexpectedly;
- background work measurably degrades foreground interaction;
- startup/reader hot path drifts materially from prior accepted checkpoint.

A performance regression may be accepted only with a documented tradeoff and owner decision. Accepted tradeoffs belong in checkpoint evidence/changelog so they are not rediscovered later.
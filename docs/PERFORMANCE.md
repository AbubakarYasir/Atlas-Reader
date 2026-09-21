# Performance Contract

Atlas Reader Native is being rewritten partly to gain tighter control over latency and rendering. That does **not** justify claims such as "cannot lag." Performance is a measured product requirement.

## Principles

1. UI/render thread stays bounded.
2. Work is prioritized by what the user can currently see/do.
3. Memory use scales with viewport/cache policy, not total document page count.
4. Background indexing never gets to make foreground reading feel broken.
5. Benchmarks record hardware, OS, build type, document/library fixture, and percentile—not only best-case averages.

## Frame budget

At 60 Hz a frame is 16.67 ms. Atlas should keep application-side UI work comfortably below that to leave room for compositor/GPU variation.

Targets are initially aspirational until N1/N4 benchmarks establish realistic thresholds:

- no synchronous application task > 4 ms on GUI thread in normal interaction;
- target most GUI callbacks < 2 ms;
- no directory/PDF/database serialization work on GUI thread;
- smooth resizing/scrolling at 60 Hz on baseline hardware;
- qualify 120 Hz/pen devices separately rather than assuming 60 Hz success is enough.

## Startup

Measure separately:

- process start → first window frame;
- first window → library state from local DB;
- background refresh completion.

The app should show useful shell/state before scanning or PDF work completes.

## Library/index

- directory enumeration isolated from UI;
- bounded metadata workers;
- progressive commits/results;
- FTS queries used for global search;
- writer batching to reduce transaction overhead;
- unavailable roots skip destructive reconciliation;
- watcher events coalesced/debounced.

Important benchmark fixtures:

- 1k, 10k, and larger libraries;
- SSD and representative HDD;
- Arabic/Unicode long paths;
- overlapping roots;
- encrypted/corrupt books;
- disconnected root.

## Search

Existing product target retained as an eventual gate: 10,000-bookmark search should remain under **100 ms p95** on qualified baseline hardware.

Results should stream/rank progressively if larger datasets make full completion slower.

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

## Zoom/scroll

A zoom gesture should reuse available textures temporarily and request sharper renders asynchronously. It should not synchronously re-raster the entire visible set for every gesture delta.

## Ink latency

Pointer samples flow to the live stroke overlay directly. Per-sample paths must not touch PDF serialization, SQLite, or full-page re-rendering.

Measure input-to-visible-stroke latency with suitable hardware in N6/N10. Do not publish a latency claim based only on mouse simulation.

## PDF save

Saving may be expensive but must not freeze the reader shell. Display progress/cancellable phases where technically safe. Source replacement remains atomic/recoverable according to core workflow rules.

## Memory

Track:

- idle shell;
- library loaded;
- one 2,000-page document;
- multiple open tabs;
- page texture cache;
- thumbnail cache;
- temporary save buffers.

Caches have explicit budgets and eviction policies. "RAM is available" is not an eviction policy.

## Benchmark hygiene

Every recorded benchmark should include:

- commit SHA;
- release/debug build;
- compiler/Qt version;
- CPU/GPU/RAM/storage;
- Windows version/display refresh;
- fixture identity/size;
- warm/cold cache state;
- p50/p95/p99 when meaningful.

A regression may be accepted only with a documented tradeoff and owner decision.
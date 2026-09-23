# N2 PDF Stress / Concurrency Baseline

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Planned — evidence pending**  
**Opened:** 2026-09-23  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This file defines the stress/concurrency qualification gate before any result is recorded. It must be updated with exact implementation SHA, workflow run, artifact identity and measured results before the slice can be treated as canonical evidence.

N2 remains Open. This file does not select a read engine and does not constitute `N2 PASS`.

## 1. Questions this slice must answer

### A. Lifetime-symmetric resource behavior

For both Qt PDF and PDFium, repeatedly perform the same high-level lifetime:

1. open a deterministic PDF;
2. load page 0;
3. render a bounded raster;
4. extract page text;
5. release page/document/render/text resources;
6. record process working-set checkpoints periodically.

The purpose is to detect crashes, resource-lifetime failures and obvious monotonic growth patterns under repeated complete document lifetimes.

The test does **not** invent a universal leak threshold from one GitHub-hosted Windows runner. Working-set start/end/peak/checkpoints and observed growth are evidence. A memory difference by itself is not called a leak without stronger evidence.

### B. PDFium serialization under concurrent application producers

PDFium's public API contract is not thread-safe. N2 must therefore never qualify or benchmark simultaneous PDFium API calls.

Instead, qualification will model the intended Atlas execution boundary:

- four application producer threads;
- each producer submits 250 PDF work requests;
- exactly one PDFium worker/execution lane owns all PDFium calls;
- 1,000 requests total;
- each request performs bounded open/page/render/text work and closes its resources;
- every request must complete successfully;
- the worker must not deadlock or drop tasks;
- measured maximum simultaneous PDFium API executions must equal **1**;
- all engine API work must execute on the single worker thread.

Queue wait/completion distributions may be recorded, but they are not compared directly with Qt timing as an engine-speed verdict.

## 2. Initial deterministic corpus

Use existing, already-qualified fixtures rather than inventing a new stress-only PDF unless evidence requires one:

- A003 — small ordinary English/navigation PDF, SHA-256 `77aec987979e995b940efabc2faf34da62b19be8b7acb800b95f993a720f42d5`;
- A011 — deterministic visual/geometry fixture, SHA-256 `b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29`.

The first stress pass may use A003 for lifetime/open/text work and A011 for render/geometry work. These remain small synthetic documents; large-document scaling is not inferred.

## 3. Proposed protocol

### Lifetime loop

Default qualification target:

- 500 complete document lifetimes per engine;
- render target approximately 306 × 396 pixels or another explicitly fixed small raster;
- working-set checkpoint every 50 iterations plus start/end;
- failures recorded with iteration index and operation;
- all candidate-native objects must be released before the next iteration begins.

Qt and PDFium probes should use equivalent high-level work. Candidate-specific initialization that necessarily lives for process lifetime must be documented separately rather than hidden.

### PDFium serialized queue

Default qualification target:

- producer threads: 4;
- requests per producer: 250;
- total: 1,000;
- one worker thread calling PDFium;
- bounded fixture/render workload per task;
- maximum active PDFium API execution: exactly 1;
- completed = submitted = 1,000;
- failures = 0;
- queue exits cleanly after all producers finish.

## 4. Evidence to capture

Each engine lifetime probe should emit machine-readable JSON containing at least:

- engine/version/pin;
- fixture identity;
- requested/completed iteration counts;
- failures;
- start/end/peak working set;
- periodic working-set checkpoints;
- first and last checkpoint deltas;
- total elapsed time;
- render/text/open success counts.

The PDFium queue probe should additionally emit:

- producer count;
- requests per producer;
- submitted/completed/failed totals;
- worker thread count actually used for PDFium calls;
- maximum simultaneous active PDFium execution count;
- queue wait p50/p95 if measured;
- task completion p50/p95 if measured;
- clean shutdown state.

## 5. Pass / interpretation rules

The functional gate passes only if:

- every requested lifetime iteration completes;
- every requested PDFium queue task completes;
- no crash/deadlock/timeout occurs;
- engine resource close/destruction paths complete;
- PDFium maximum active API execution is exactly 1;
- no PDFium API calls occur from producer threads;
- the artifact contains the full working-set/checkpoint evidence.

Memory interpretation is separate from functional pass/fail:

- a stable/noisy bounded series may be recorded as **PASS WITH LIMITATION** under this synthetic workload;
- clear sustained monotonic growth should trigger investigation before responsibility selection;
- one start/end delta on a hosted runner must not be labeled a leak by itself;
- no absolute memory threshold is invented solely to make this gate pass.

## 6. Architectural contract under test

If PDFium remains a production candidate, Atlas must own a serialization boundary. Engine handles/types must not escape that lane into arbitrary concurrent application code.

The likely production pattern is an Atlas-owned worker/executor with concurrent callers submitting immutable work requests and receiving normalized results. This qualification tests that pattern without prematurely implementing N3 Reader architecture.

Qt PDF is not assumed to require the same serialization policy; this slice's Qt lifetime loop is resource-lifetime evidence, not a forced queue architecture.

## 7. Non-goals

This slice does not:

- select Qt PDF or PDFium;
- implement the production reader/task scheduler;
- test simultaneous unsupported PDFium API calls;
- establish large-document performance scaling;
- establish a universal leak threshold;
- start N3.

## 8. Evidence record

Implementation SHA: **PENDING**  
Workflow run: **PENDING**  
Artifact: **PENDING**  
Qt lifetime result: **PENDING**  
PDFium lifetime result: **PENDING**  
PDFium serialized queue result: **PENDING**

N2 remains **Open**.

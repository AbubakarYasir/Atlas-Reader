# N2 PDF Stress / Concurrency Baseline

**Checkpoint:** N2 — PDF engine qualification spike  
**Status:** **Evidence captured — functional stress PASS; memory trends recorded with limitation**  
**Opened:** 2026-09-23  
**Evidence date:** 2026-09-23  
**Branch:** `native-v2-n2-pdf-engine-qualification`

This file records the canonical stress/concurrency evidence for N2. It does not select a read engine and does not constitute `N2 PASS`.

## 1. Canonical implementation and artifact

Physically tested implementation SHA:

`72109de91aad5496a9e2fe16d5741581243a649f`

Exact-head workflow results:

- `N2 PDF Stress` run `35801464463` — **PASS**;
- `Windows CI` run `35801464476` — **PASS**;
- `N2 PDF Coordinates` run `35801464469` — **PASS**;
- `N2 PDF Fidelity` run `35801464486` — **PASS**;
- `N2 PDF Performance` run `35801464462` — **PASS**;
- `N2 qpdf Qualification` run `35801464470` — **PASS**.

Canonical stress artifact:

- artifact ID `10726420090`;
- name `atlas-reader-n2-pdf-stress-72109de91aad5496a9e2fe16d5741581243a649f`;
- digest `sha256:6fa983cc89745950b7d73c75d8aea9061dfa0723a348724c6063dac12058f34a`.

## 2. Protocol

Existing deterministic fixtures were reused:

- A003 SHA-256 `77aec987979e995b940efabc2faf34da62b19be8b7acb800b95f993a720f42d5`;
- A011 SHA-256 `b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29`.

Lifetime stress per read engine:

1. open the deterministic document;
2. load/render bounded page content;
3. extract text;
4. release candidate-native page/document/render/text resources;
5. repeat for **500 complete document lifetimes**;
6. record process working set every 50 iterations plus start/end/OS peak.

PDFium serialized-queue stress:

- producers: **4**;
- requests per producer: **250**;
- submitted requests: **1,000**;
- execution lanes allowed to call PDFium: **1**;
- PDFium initialization, queued work and shutdown all occur on the dedicated worker lane;
- producer-side PDFium API calls are forbidden.

## 3. Functional lifetime result

| Check | Qt PDF 6.10.3 | PDFium `chromium/8066` |
|---|---:|---:|
| requested lifetimes | 500 | 500 |
| completed lifetimes | 500 | 500 |
| render successes | 500 | 500 |
| text successes | 500 | 500 |
| operation failures | 0 | 0 |
| result | **PASS** | **PASS** |

Qt completed the 500-lifetime workload in about `1629.95 ms` on the hosted runner. PDFium completed its 500-lifetime workload in about `623.47 ms`. These totals are stress-run observations, not a new performance ranking; the canonical repeated performance comparison remains `N2 PDF Performance`.

## 4. PDFium serialized execution result

The queue result is **PASS**:

- submitted: `1000`;
- completed: `1000`;
- failed: `0`;
- actual PDFium worker thread count: `1`;
- producer-side PDFium API calls: `0`;
- maximum simultaneously active PDFium API executions: `1`;
- clean shutdown: `true`.

Observed queue latency under this intentionally serialized burst workload:

- queue-wait p50: `447.7567 ms`;
- queue-wait p95: `878.7624 ms`;
- total-latency p50: `448.4745 ms`;
- total-latency p95: `879.4784 ms`.

These queue latencies describe a 1,000-request synthetic serialized workload and are **not** a Qt-vs-PDFium speed comparison or a proposed UI latency budget.

## 5. Working-set evidence

The validator intentionally does not invent a universal leak threshold from one hosted-runner series.

### Qt PDF

- start: `10.03125 MiB`;
- first checkpoint at iteration 50: `14.26172 MiB`;
- last checkpoint at iteration 500: `18.56641 MiB`;
- end: `18.56641 MiB`;
- start → end delta: `+8.53516 MiB`;
- first → last checkpoint delta: `+4.30469 MiB`;
- observed checkpoint range: `14.26172–18.56641 MiB`;
- linear fitted slope: about `9.94 KiB/iteration`;
- increasing checkpoint steps: `8/9`.

### PDFium

- start: `8.71484 MiB`;
- first checkpoint at iteration 50: `11.25 MiB`;
- last checkpoint at iteration 500: `11.84375 MiB`;
- end: `11.84375 MiB`;
- start → end delta: `+3.12891 MiB`;
- first → last checkpoint delta: `+0.59375 MiB`;
- observed checkpoint range: `11.25–11.86719 MiB`;
- linear fitted slope: about `1.23 KiB/iteration`;
- increasing checkpoint steps: `7/9`.

Interpretation:

- there were **no crashes, failed closes, dropped operations or resource-lifetime failures** across the 500-cycle runs;
- PDFium's working-set series is nearly flat after initial process/library growth under this tiny synthetic workload;
- Qt's working set shows a measurable upward trend across this single 500-cycle hosted-runner series;
- that trend is recorded as a **limitation / follow-up signal**, not automatically labeled a memory leak from one synthetic process-level working-set run;
- if Qt PDF is selected for a production responsibility involving repeated document churn, a later real-document soak test should retain this series as the baseline comparator.

Therefore lifetime/resource stress is **PASS WITH LIMITATION** for both candidates, with the stronger memory-growth caveat attached to Qt PDF.

## 6. Architectural conclusion for PDFium

The qualification proves that the intended Atlas execution boundary is viable:

> concurrent Atlas callers may submit immutable PDF work concurrently, but all PDFium public API execution is serialized through one Atlas-owned worker/executor lane.

If PDFium is selected for a production responsibility:

- PDFium handles/types must not escape the serialized adapter boundary;
- producer/application threads must not call PDFium directly;
- initialization and shutdown ownership must remain explicit;
- queue cancellation/back-pressure policy belongs to the Atlas adapter/application layer, not the domain model.

This is architecture evidence only; N2 does not implement the N3 Reader scheduler.

## 7. What this slice does not prove

This evidence does not establish:

- arbitrary large-document memory stability;
- multi-hour desktop soak stability;
- image-heavy or pathological-PDF memory behavior;
- a universal acceptable RSS/working-set threshold;
- that simultaneous PDFium API calls are safe — they were deliberately **not** attempted;
- production queue fairness/cancellation/back-pressure UX;
- a final read-engine selection.

## 8. Result

- Qt PDF repeated lifetime correctness: **PASS**;
- PDFium repeated lifetime correctness: **PASS**;
- PDFium serialized multi-producer queue: **PASS**;
- PDFium non-overlap contract: **PASS**;
- memory-growth interpretation: **PASS WITH LIMITATION / measured evidence**, with Qt showing the larger upward trend.

N2 remains **Open**. Production acquisition/licensing, scope review, responsibility assignment, final exact-head CI and explicit owner `N2 PASS` remain outstanding.

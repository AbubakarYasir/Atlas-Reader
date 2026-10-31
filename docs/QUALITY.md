# Atlas Reader — Quality, Testing and Speed

Correctness here means more than "doesn't crash". A fast build is still unacceptable if it corrupts a PDF, attaches research to the wrong copy, breaks Arabic, blocks a screen-reader user, or silently drops local work. Stage-by-stage obligations are in [CHECKPOINTS.md](../CHECKPOINTS.md); this file defines **how** we prove them.

## 1. Test layers

| Layer | What | Examples |
|---|---|---|
| Domain/unit | Pure C++ with no GUI | Identity decisions, capability transitions, bookmark tree ops, overlay states, reconciliation plans, page/label conversion, migration mapping, backup versioning |
| Component | One adapter on controlled resources | SQLite repositories/migrations/FTS, enumeration/watch, scheduler/cancellation, PDF adapter, serializers, backup/restore |
| Contract | Every implementation of a port meets shared expectations | PDF adapters normalize pages, rotation, boxes and destinations identically |
| Integration | Real subsystem chains | Folder → scan → SQLite → search; open → render → navigate; overlay → reconcile → safe save → reopen; external change → conflict; backup → clean profile → restore |
| GUI | Qt Test / QML tests on behavior, focus, state and semantics | Not brittle pixel positions |
| Manual / hardware | Only where automation can't prove it | Pen latency, Narrator flow, printing, high-DPI/multi-monitor, HDD/external drives, Arabic visual correctness, installer |

## 2. PDF fixtures

Never mutate irreplaceable originals. Committed fixtures need a stable ID, provenance, redistribution permission, SHA-256, expected behavior and mutation permission ([fixture contract](../tests/fixtures/pdf/README.md)). Private owner PDFs stay local and out of Git and CI logs. Test passwords are synthetic.

Required by N5/N6: simple writable · mixed Arabic/English outline · duplicate names under different parents · deep outline · no outline · malformed outline with readable pages · image-only · mixed page sizes · rotated · unusual crop/media boxes · page labels ≠ indices · open-password · permission-restricted · read-only copy · signed/certified · externally modified revision pair · sharing-locked · 2,000+ pages · 10,000+ outline nodes · XMP-rich · existing third-party annotations.

## 3. Preservation and interoperability

Every PDF write test compares, before and after: page count, size and rotation; outline hierarchy, order and destinations; unrelated annotations; document info and XMP; encryption and security; signature/certification behavior; attachments and other structures; content streams where they should not change. "It opens" is not a pass.

Embedded data is reopened by (1) Atlas in a fresh session and (2) an **independent** reader or tool. Atlas never validates itself only with the code that wrote the file.

## 4. Safety: failure injection and hostile input

Before stable: source locked · permission removed after open · disk full · temporary-file validation failure · source moved/deleted while open · external modification · removable folder disconnected · process killed at each safe-save step · corrupted or partial local database · restore with ambiguous books. The expected outcome is **preservation and recoverability**, not just an error dialog.

Fuzz Atlas-owned parsers as they land (bookmark JSON in N5, backup manifest and migration input in N8). Engine errors must be contained and never lead to an unsafe write or a UI crash. Imports enforce size, depth and node limits that are high enough for 10,000+ real bookmarks.

## 5. Speed budgets

Engineering targets for baseline hardware (named in the [N1 baseline](baselines/N1_WINDOWS_BASELINE.md)). Missing one triggers investigation; final gates are frozen in N10 with real fixtures. They are not advertised until Verified.

| Scenario | Target |
|---|---|
| GUI callback | Usually < 2 ms; no normal synchronous task > 4 ms |
| Frame (60 Hz) | 16.67 ms total, with Atlas UI work leaving clear margin; 120 Hz and pen devices qualified separately |
| Warm / cold shell first frame | < 800 ms / < 1.5 s p95 on SSD |
| Cached library usable after shell | < 1 s |
| Command Center query | < 50 ms p95 |
| **10,000-bookmark search** | **< 100 ms p95 (gate)** |
| Page jump feedback | Within one frame; sharp render may follow asynchronously |
| Visible-page render (open session) | < 250 ms p95 at reading resolution |
| First readable page after open | Measured in N4; target < 500 ms p95 for typical books |
| Scrolling | 60 Hz on baseline hardware; no repeated stalls |
| Document search | First results quickly; p50/p95/p99 recorded on a 1,000+ page book; cancellable |
| Memory, 2,000-page PDF | Bounded by cache budget, **not** proportional to page count |
| Background scan while reading | No noticeable input or frame regression |
| Ink | Input-to-visible-stroke p50/p95 on real pen hardware (N6/N10); mouse simulation doesn't count |

**Library:** time to the first 50/100 visible results matters more than total scan time. Test 1k and 10k books, a larger stress set, SSD and HDD/external storage, Unicode paths, overlapping folders, encrypted/corrupt files and disconnected folders. Worker concurrency adapts to the storage.

**Saving** never freezes the shell. Measure serialize, validate, replace and reopen separately, and show progress for long phases. Cancel only where it's safe; cancelling never leaves a false "committed" state.

## 6. How to make it fast

1. Reproduce in a **Release** build with a minimal scenario.
2. Measure: QML Profiler (bindings, frames), Tracy (C++ zones, queues, locks), RenderDoc (GPU), WPR/WPA (startup, I/O, scheduling), Visual Studio profiler.
3. Fix the actual cause. It is usually eager rendering, synchronous I/O, unbounded tasks, wrong caching or binding storms, not micro-optimizations.
4. Record before/after with hardware, build, fixture, warm/cold state, p50/p95/p99 and sample count.
5. Add a regression threshold once the numbers are stable.

Never compare Debug with Release, or different machines, without saying so. A faster result that breaks correctness is a regression.

**Benchmarks:** `atlas_n3_library_benchmark` (enumeration, metadata/FTS insert, warmed search; copied data only) exists. N4 adds a reader benchmark for first page, page jump, scroll pacing, search and the memory series.

## 7. Accessibility and Arabic gates (every UI stage)

- Complete keyboard route, logical focus order, visible focus.
- UI Automation role, name and state for custom controls; Narrator by hand.
- 100% and 200% (spot-check 125% and 150%), Windows high contrast, reduced motion, never color alone.
- Arabic UI direction and announcements; error and recovery dialogs usable without a pointer.
- Mixed fixtures: Arabic, English, Urdu, mixed titles, both digit systems, punctuation and brackets, diacritics, long Unicode/RTL+LTR paths. Round trips preserve Unicode exactly unless a documented normalization is intended.

## 8. Static and build gates

Now: clean configure, Debug + Release builds with `/W4 /permissive- /WX` on Atlas targets, CTest, Markdown governance check, PowerShell syntax check. Before the first stage that adds significant new C++ (N4.2): clang-format check and a curated clang-tidy set. Before RC: CodeQL C/C++, secrets scanning, dependency/license inventory and SBOM, ASan build where supported, coverage report for domain code (a diagnostic, not a target to game).

**CI lanes:** a fast lane on every push (build, unit/component tests, docs, formatting); a full lane on PDF-adapter changes, nightly and before reviews (N2 regression workflows, broad fixtures, benchmark smoke, package dry run); a release lane from tags only (clean build, full tests, package, notices/SBOM, checksum, smoke test).

## 9. Security and privacy gates

Passwords never in logs, DB, exports, crash bundles or snapshots. Ordinary logs never contain document text or bookmark notes. The core runtime needs no network. Dependencies are reviewed for security updates before RC. Signed/certified originals never silently lose integrity. Paths are data, never commands. Full model: [DATA_AND_SAFETY.md](DATA_AND_SAFETY.md#security-model).

## 10. Evidence record

Each sub-stage and stage handoff in the evidence ledger records:

```text
Stage / sub-stage:        Commit:              Toolchain:
Dependencies changed:     Automated tests (+ CI run IDs):
Fixtures:                 Performance (hardware, build, p50/p95/p99):
Design review (§10 of DESIGN.md, screenshot set):
Accessibility / RTL:      Manual hardware:
Known limitations → owning stage + blocking test:
Data migrations:          Artifacts + SHA-256:      Owner result:
```

Include a requirement-to-evidence table: every requirement maps to a test, a numbered manual check, a measurement or an explicit "not applicable, because". A green test count does not close an unmapped requirement. Unexplained skipped core tests block acceptance.

## 11. Defects

Every defect found in an owner review or a beta gets a fix plus an automated regression test, or a numbered manual check if automation can't prove it. A defect can also be explicitly moved to a named later stage with a blocking test. "Later hardening", "known limitation" or "future work" without a stage and a test is not allowed. The point is that Atlas gets **harder to break** as it grows.

## 12. Reviews are short

Owner review guides are plain language, about 10 checks and about 45 minutes, with a numbered PASS/FAIL report. Every skip is stated, because a skip is not a pass. Agents do the heavy verification first, so the owner's time goes to judgment: comfort, clarity, Arabic quality and trust.

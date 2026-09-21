# ADR-0001 — Native application stack

**Status:** Accepted for bootstrap; production dependency choices remain checkpoint-qualified.

**Date:** 2026-09-22

## Context

Atlas Reader's roadmap includes a large local library, virtualized PDF rendering, deeply nested bookmarks, pen/touch input, printing, safe structural PDF edits, Arabic/RTL, accessibility, and future Android/Linux/macOS/iOS support.

The existing Flutter implementation demonstrated product workflows but the successor seeks tighter control over graphics, native libraries, memory, threading, and input latency.

## Decision

Use:

- C++23 for shared domain/application/performance-sensitive code;
- Qt 6.11.x for cross-platform application infrastructure;
- Qt Quick/QML for presentation;
- CMake for builds;
- SQLite/FTS5 for local index/search after its checkpoint;
- an Atlas-owned PDF facade so rendering/transformation engines remain replaceable.

## Alternatives considered

### Rust + Slint/other Rust GUI

Strong language safety/performance, but higher ecosystem/product risk for this combination of mature desktop PDF, printing, accessibility, mobile, stylus, and platform integration.

### C# + Avalonia

Strong productivity/cross-platform option. Rejected for this iteration because Atlas's hot path and PDF/graphics integration are heavily native and the team wants direct control without making managed/native boundaries foundational.

### Kotlin Compose Multiplatform

Compelling for Android-first software, but Atlas is Windows-first with deep native desktop/PDF requirements.

### Tauri/Electron

Excellent for many applications, but a web rendering layer is not preferred as the foundation of Atlas's central document canvas/stylus workload.

### Continue Flutter

Lowest migration effort, but does not satisfy the motivation for a native successor and would preserve the architectural constraints this project intends to reevaluate.

## Consequences

Positive:

- mature multi-platform GUI stack;
- native library interoperability;
- direct performance/memory control;
- credible Windows-first and mobile-later path;
- custom scene-graph/rendering options.

Costs:

- C++ complexity and memory-safety burden;
- Qt licensing/deployment obligations;
- slower implementation than high-level managed frameworks if architecture/testing discipline is weak;
- need for explicit async/task ownership;
- platform qualification still required; Qt does not make platform differences disappear.

## Guardrail

This ADR chooses the architecture, not every backend library. N2 must still prove PDF engine choices with fixtures, benchmarks, preservation, and licensing evidence.
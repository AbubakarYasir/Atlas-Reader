#!/usr/bin/env python3
"""Validate Atlas N2 PDF stress evidence without inventing a leak threshold."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any


def mib(value: int | float) -> float:
    return float(value) / 1048576.0


def memory_summary(start: int, end: int, checkpoints: list[dict[str, Any]]) -> dict[str, Any]:
    points = [
        (int(item["iteration"]), int(item["working_set_bytes"]))
        for item in checkpoints
        if int(item.get("iteration", 0)) > 0
    ]

    slope_bytes_per_iteration = 0.0
    if len(points) >= 2:
        mean_x = sum(x for x, _ in points) / len(points)
        mean_y = sum(y for _, y in points) / len(points)
        denominator = sum((x - mean_x) ** 2 for x, _ in points)
        if denominator:
            slope_bytes_per_iteration = sum(
                (x - mean_x) * (y - mean_y) for x, y in points
            ) / denominator

    checkpoint_values = [value for _, value in points]
    monotonic_increases = sum(
        1 for left, right in zip(checkpoint_values, checkpoint_values[1:]) if right > left
    )

    return {
        "start_mib": mib(start),
        "end_mib": mib(end),
        "end_minus_start_mib": mib(end - start),
        "checkpoint_count": len(points),
        "checkpoint_first_mib": mib(checkpoint_values[0]) if checkpoint_values else None,
        "checkpoint_last_mib": mib(checkpoint_values[-1]) if checkpoint_values else None,
        "checkpoint_min_mib": mib(min(checkpoint_values)) if checkpoint_values else None,
        "checkpoint_max_mib": mib(max(checkpoint_values)) if checkpoint_values else None,
        "checkpoint_last_minus_first_mib": (
            mib(checkpoint_values[-1] - checkpoint_values[0]) if len(checkpoint_values) >= 2 else None
        ),
        "linear_slope_kib_per_iteration": slope_bytes_per_iteration / 1024.0,
        "monotonic_increase_steps": monotonic_increases,
        "interpretation": "measurement-only; no universal leak threshold inferred from one hosted-runner series",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qt", type=Path, required=True)
    parser.add_argument("--pdfium", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    qt = json.loads(args.qt.read_text(encoding="utf-8-sig"))
    pdfium = json.loads(args.pdfium.read_text(encoding="utf-8-sig"))
    failures: list[str] = []

    qt_requested = int(qt.get("iterations_requested", 0))
    qt_completed = int(qt.get("iterations_completed", -1))
    if not qt.get("passed") or qt_requested <= 0 or qt_completed != qt_requested:
        failures.append("qt-lifetime-incomplete")
    if int(qt.get("render_successes", -1)) != qt_requested:
        failures.append("qt-render-count-mismatch")
    if int(qt.get("text_successes", -1)) != qt_requested:
        failures.append("qt-text-count-mismatch")
    if qt.get("failures"):
        failures.append("qt-failures-present")

    lifetime = pdfium.get("lifetime", {})
    pdf_requested = int(lifetime.get("iterations_requested", 0))
    pdf_completed = int(lifetime.get("iterations_completed", -1))
    if not lifetime.get("passed") or pdf_requested <= 0 or pdf_completed != pdf_requested:
        failures.append("pdfium-lifetime-incomplete")
    if int(lifetime.get("render_successes", -1)) != pdf_requested:
        failures.append("pdfium-render-count-mismatch")
    if int(lifetime.get("text_successes", -1)) != pdf_requested:
        failures.append("pdfium-text-count-mismatch")
    if lifetime.get("failures"):
        failures.append("pdfium-lifetime-failures-present")

    queue = pdfium.get("serialized_queue", {})
    producers = int(queue.get("producer_count", 0))
    requests_per_producer = int(queue.get("requests_per_producer", 0))
    expected_tasks = producers * requests_per_producer
    if not queue.get("passed"):
        failures.append("pdfium-queue-not-passed")
    if int(queue.get("submitted", -1)) != expected_tasks:
        failures.append("pdfium-submitted-count-mismatch")
    if int(queue.get("completed", -1)) != expected_tasks:
        failures.append("pdfium-completed-count-mismatch")
    if int(queue.get("failed", -1)) != 0:
        failures.append("pdfium-queue-task-failures")
    if int(queue.get("pdfium_worker_thread_count", -1)) != 1:
        failures.append("pdfium-worker-thread-count-not-one")
    if int(queue.get("producer_pdfium_api_calls", -1)) != 0:
        failures.append("pdfium-api-called-from-producer")
    if int(queue.get("max_active_pdfium_api_executions", -1)) != 1:
        failures.append("pdfium-api-overlap-detected")
    if not queue.get("clean_shutdown"):
        failures.append("pdfium-queue-shutdown-not-clean")

    qt_memory = memory_summary(
        int(qt.get("working_set_start_bytes", 0)),
        int(qt.get("working_set_end_bytes", 0)),
        list(qt.get("checkpoints", [])),
    )
    pdfium_memory = memory_summary(
        int(lifetime.get("working_set_start_bytes", 0)),
        int(lifetime.get("working_set_end_bytes", 0)),
        list(lifetime.get("checkpoints", [])),
    )

    result = {
        "schema": "atlas.n2.pdf-stress-validation.v1",
        "functional_gate": {
            "qt_lifetime_iterations": qt_requested,
            "pdfium_lifetime_iterations": pdf_requested,
            "pdfium_producers": producers,
            "pdfium_requests_per_producer": requests_per_producer,
            "pdfium_total_requests": expected_tasks,
            "pdfium_max_active_api_executions": queue.get("max_active_pdfium_api_executions"),
            "pdfium_worker_thread_count": queue.get("pdfium_worker_thread_count"),
            "producer_pdfium_api_calls": queue.get("producer_pdfium_api_calls"),
            "clean_shutdown": queue.get("clean_shutdown"),
        },
        "queue_observation": {
            "wait_p50_ms": queue.get("queue_wait_p50_ms"),
            "wait_p95_ms": queue.get("queue_wait_p95_ms"),
            "total_latency_p50_ms": queue.get("total_latency_p50_ms"),
            "total_latency_p95_ms": queue.get("total_latency_p95_ms"),
            "note": "queue/latency values describe the serialized executor workload and are not a direct Qt-vs-PDFium speed comparison",
        },
        "qt_memory": qt_memory,
        "pdfium_memory": pdfium_memory,
        "memory_policy": "working-set series is evidence only; this validator does not infer leak/no-leak from a fixed absolute threshold",
        "failures": failures,
        "passed": not failures,
    }

    text = json.dumps(result, ensure_ascii=False, indent=2) + "\n"
    print(text, end="")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    return 0 if not failures else 2


if __name__ == "__main__":
    raise SystemExit(main())

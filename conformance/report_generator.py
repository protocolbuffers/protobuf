#!/usr/bin/env python3
# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Generates Markdown conformance reports from ConformanceRunResult textprotos.

The inputs are the files the gtest conformance runner writes with
--output_result_file (see README.md, "Coordinates and reports").

Supports three modes:
1. single: Generates a detailed <impl>_report.md for a single implementation.
2. master: Ingests multiple implementation results and produces a master
   conformance_matrix.md grid.
3. index:  Emits conformance_test_index.md, the catalog of every test case the
   harness knows about, with its coordinate, description and the set of
   variants it runs under.  This is a property of the suite itself, not of any
   implementation, so it only needs a single run result as its input.
"""

import argparse
import collections
import html
import os
import re
import sys
import time
from typing import Any, Dict, List, Tuple

try:
  from google.protobuf import text_format
  from conformance import conformance_result_pb2
except ImportError:
  from google.protobuf import text_format
  from third_party.protobuf.conformance import conformance_result_pb2

KNOWN_DOMAINS = {
    "01": "Wire",
    "02": "TextFormat",
    "03": "JSON",
    "04": "Semantics",
    "05": "Editions",
}

KNOWN_SECTIONS = {
    # Wire (01)
    "01.01": "Varint",
    "01.02": "Fixed",
    "01.03": "Length-Delimited",
    "01.04": "Groups & Unknown Fields",
    "01.05": "Message Sets",
    # TextFormat (02)
    "02.01": "Lexer",
    "02.02": "Fields",
    "02.03": "Extensions",
    "02.04": "Performance",
    # JSON (03)
    "03.01": "Field Names",
    "03.02": "Primitives",
    "03.03": "Well-Known Types",
    "03.04": "Enums",
    "03.05": "Parsing",
    # Semantics (04)
    "04.01": "Oneofs",
    "04.02": "Oneofs",
    "04.03": "Maps",
    # Editions (05)
    "05.01": "Delimited Fields",
    "05.02": "UTF-8 Validation",
    "05.03": "Features Unstable",
    # Informational (999)
    "999.01": "Wire",
    "999.02": "TextFormat",
    "999.03": "Unknown Field Order",
    "999.04": "Performance Benchmarks",
}


def resolve_path(rel_path: str) -> str:
  if not rel_path or os.path.isabs(rel_path):
    return rel_path
  for env_var in ("BUILD_WORKING_DIRECTORY", "BUILD_WORKSPACE_DIRECTORY"):
    ws = os.environ.get(env_var)
    if ws:
      return os.path.abspath(os.path.join(ws, rel_path))
  return os.path.abspath(rel_path)


def load_run_result(path: str) -> conformance_result_pb2.ConformanceRunResult:
  resolved = resolve_path(path)
  run_result = conformance_result_pb2.ConformanceRunResult()
  with open(resolved, "r", encoding="utf-8") as f:
    text_format.Parse(f.read(), run_result)
  return run_result


def get_status_str(status_val: int) -> str:
  try:
    return conformance_result_pb2.ConformanceTestCaseResult.Status.Name(
        status_val
    )
  except ValueError:
    return "UNKNOWN"


# Status badges.  Plain ASCII tokens (no emoji) so that the reports render the
# same everywhere and stay greppable.
BADGE_PASS = "PASS"
BADGE_ALTERNATE = "ALT"
BADGE_OTHER = "OTHER"
BADGE_FAIL = "FAIL"
BADGE_CRASH = "CRASH"
BADGE_SKIP = "SKIP"
BADGE_MISSING = "-"

PASSING_STATUSES = ("PASS_CANONICAL", "PASS_ALTERNATE", "PASS_OTHER")


def format_badge_md(
    status_str: str,
    matched_behavior_id: str = "",
    is_informational: bool = False,
) -> str:
  """The Markdown badge for a status.

  Informational tests never pass or fail on their own; a pass reports which
  legal behavior (matched_behavior_id) the implementation chose.
  """
  if is_informational:
    if status_str == "FAILURE":
      return BADGE_FAIL
    elif status_str == "CRASH":
      return BADGE_CRASH
    elif status_str == "SKIPPED":
      return BADGE_SKIP
    elif status_str in PASSING_STATUSES:
      label = (
          matched_behavior_id
          if matched_behavior_id
          else ("canonical" if status_str == "PASS_CANONICAL" else "alternate")
      )
      return f"{BADGE_ALTERNATE} `{label}`"
    else:
      return BADGE_MISSING
  else:
    if status_str == "PASS_CANONICAL":
      return BADGE_PASS
    elif status_str == "PASS_ALTERNATE":
      label = matched_behavior_id if matched_behavior_id else "alternate"
      return f"{BADGE_ALTERNATE} `{label}`"
    elif status_str == "PASS_OTHER":
      return BADGE_OTHER
    elif status_str == "FAILURE":
      return BADGE_FAIL
    elif status_str == "CRASH":
      return BADGE_CRASH
    elif status_str == "SKIPPED":
      return BADGE_SKIP
    else:
      return BADGE_MISSING


def format_badge_html(
    status_str: str,
    matched_behavior_id: str = "",
    is_informational: bool = False,
) -> str:
  """The HTML badge for a status; see format_badge_md()."""
  if is_informational:
    if status_str == "FAILURE":
      return '<span class="badge badge-failure">FAIL Failed</span>'
    elif status_str == "CRASH":
      return '<span class="badge badge-crash">CRASH Crash</span>'
    elif status_str == "SKIPPED":
      return '<span class="badge badge-skipped">SKIP Skipped</span>'
    elif status_str in PASSING_STATUSES:
      label = (
          matched_behavior_id
          if matched_behavior_id
          else ("canonical" if status_str == "PASS_CANONICAL" else "alternate")
      )
      return (
          f'<span class="badge badge-alternate">ALT {html.escape(label)}</span>'
      )
    else:
      return '<span class="badge badge-missing">-</span>'
  else:
    if status_str == "PASS_CANONICAL":
      return '<span class="badge badge-canonical">PASS Canonical</span>'
    elif status_str == "PASS_ALTERNATE":
      label = matched_behavior_id if matched_behavior_id else "Alternate"
      return (
          f'<span class="badge badge-alternate">ALT {html.escape(label)}</span>'
      )
    elif status_str == "PASS_OTHER":
      return '<span class="badge badge-other">OTHER Other</span>'
    elif status_str == "FAILURE":
      return '<span class="badge badge-failure">FAIL Failed</span>'
    elif status_str == "CRASH":
      return '<span class="badge badge-crash">CRASH Crash</span>'
    elif status_str == "SKIPPED":
      return '<span class="badge badge-skipped">SKIP Skipped</span>'
    else:
      return '<span class="badge badge-missing">-</span>'


def calculate_metrics(
    test_names: List[str],
    test_catalog: Dict[str, conformance_result_pb2.ConformanceTestCaseResult],
    impl_results: Dict[str, Dict[str, Any]],
) -> Dict[str, Any]:
  total = len(test_names)
  required_tests = [
      t for t in test_names if not test_catalog[t].is_informational
  ]
  info_tests = [t for t in test_names if test_catalog[t].is_informational]

  req_passed = sum(
      1
      for t in required_tests
      if impl_results.get(t, {}).get("status") == "PASS_CANONICAL"
  )
  req_pass_pct = (
      (req_passed / len(required_tests) * 100.0) if required_tests else 0.0
  )

  info_alternate = sum(
      1
      for t in info_tests
      if impl_results.get(t, {}).get("status") == "PASS_ALTERNATE"
  )
  info_alt_pct = (
      (info_alternate / len(info_tests) * 100.0) if info_tests else 0.0
  )

  canonical = sum(
      1
      for t in test_names
      if impl_results.get(t, {}).get("status") == "PASS_CANONICAL"
  )
  alternate = sum(
      1
      for t in test_names
      if impl_results.get(t, {}).get("status") == "PASS_ALTERNATE"
  )
  other = sum(
      1
      for t in test_names
      if impl_results.get(t, {}).get("status") == "PASS_OTHER"
  )
  failures = sum(
      1
      for t in test_names
      if impl_results.get(t, {}).get("status") == "FAILURE"
  )
  crashes = sum(
      1 for t in test_names if impl_results.get(t, {}).get("status") == "CRASH"
  )
  skipped = sum(
      1
      for t in test_names
      if impl_results.get(t, {}).get("status") == "SKIPPED"
  )

  return {
      "total": total,
      "required_total": len(required_tests),
      "required_passed": req_passed,
      "required_pass_pct": req_pass_pct,
      "info_total": len(info_tests),
      "info_alternate": info_alternate,
      "info_alt_pct": info_alt_pct,
      "canonical": canonical,
      "alternate": alternate,
      "other": other,
      "failures": failures,
      "crashes": crashes,
      "skipped": skipped,
  }


def priority_of(case: conformance_result_pb2.ConformanceTestCaseResult) -> str:
  """The priority ("P0".."P3") recorded for a test, "-" if it has none."""
  return case.priority or "-"


def priority_summary(priorities: List[str]) -> str:
  """Summarizes the priorities of a test case's variants: "P0" or "P0, P3"."""
  distinct = sorted(p for p in set(priorities) if p and p != "-")
  return ", ".join(distinct) or "-"


def coordinate_sort_key(
    name: str, case: conformance_result_pb2.ConformanceTestCaseResult
):
  coord = case.coordinate or "00.00.000.00"
  parts = []
  for p in coord.split("."):
    try:
      parts.append(int(p))
    except ValueError:
      parts.append(p)
  return (parts, name)


def domain_title_for(domain_key: str, raw_domain: str) -> str:
  """Human readable title for a numeric domain key ("01" -> "Wire")."""
  if domain_key in KNOWN_DOMAINS:
    return KNOWN_DOMAINS[domain_key]
  clean = re.sub(r"^\d+\.\s*", "", raw_domain or "General").strip()
  if clean.lower() == "api":
    return "API"
  return clean.replace("_", " ").title()


def section_title_for(section_key: str, raw_section: str) -> str:
  """Human readable title for a section key ("01.03" -> "Length-Delimited")."""
  if section_key in KNOWN_SECTIONS:
    return KNOWN_SECTIONS[section_key]
  clean = re.sub(r"^\d+\.\d+\s*", "", raw_section or "General").strip()
  if clean.lower() == "length_delimited":
    return "Length-Delimited"
  return clean.replace("_", " ").title()


def group_by_section(
    test_catalog: Dict[str, conformance_result_pb2.ConformanceTestCaseResult],
    sorted_test_names: List[str],
):
  """Buckets tests into the coordinate taxonomy.

  Returns (grouped_required, grouped_informational, domain_names,
  section_names) where grouped_required is {domain_key: {section_key:
  [test_name]}} and grouped_informational is {"999.NN": [test_name]}.
  """
  grouped_required: Dict[str, Dict[str, List[str]]] = collections.OrderedDict()
  grouped_informational: Dict[str, List[str]] = collections.OrderedDict()
  domain_names: Dict[str, str] = {}
  section_names: Dict[str, str] = {}

  for test_name in sorted_test_names:
    meta = test_catalog[test_name]
    coord = meta.coordinate or "00.00.000.00"
    parts = coord.split(".")
    domain_key = parts[0] if len(parts) > 0 else "01"
    section_num = parts[1] if len(parts) > 1 else "01"
    section_key = f"{domain_key}.{section_num}"

    domain_names[domain_key] = meta.domain
    section_names[section_key] = meta.section

    if meta.is_informational or domain_key == "999":
      info_sec_key = f"999.{section_num}"
      grouped_informational.setdefault(info_sec_key, []).append(test_name)
    else:
      grouped_required.setdefault(
          domain_key, collections.OrderedDict()
      ).setdefault(section_key, []).append(test_name)

  return grouped_required, grouped_informational, domain_names, section_names


def section_heading_number(section_key: str) -> str:
  parts = section_key.split(".")
  if len(parts) > 1:
    return f"{int(parts[0])}.{parts[1]}"
  return section_key


def strip_variant_from_description(description: str) -> str:
  """Drops the per-variant "[Proto3, pb2pb]" tag from a test description.

  The index is keyed on the test case, which is shared by every variant, so the
  variant-specific annotation would be misleading there.
  """
  return re.sub(r"\s*\[[^\]]*\]", "", description or "")


def collapse_to_test_cases(
    test_catalog: Dict[str, conformance_result_pb2.ConformanceTestCaseResult],
    test_names: List[str],
) -> List[Dict[str, Any]]:
  """Collapses executed tests into the distinct test cases they instantiate.

  A test case is run once per variant, so the executed tests are a cross
  product of cases and variants. Folding the variant axis back up turns a long
  list of near-identical rows into one row per case, which is what makes
  coverage gaps (a case that only runs under one syntax, say) visible.

  Preserves the incoming order, which callers sort by coordinate.
  """
  cases: Dict[Tuple[str, int], Dict[str, Any]] = collections.OrderedDict()
  for test_name in test_names:
    meta = test_catalog[test_name]
    key = (meta.section_coordinate, meta.test_number)
    entry = cases.get(key)
    if entry is None:
      entry = {
          "coordinate": f"{meta.section_coordinate}.{meta.test_number:03d}",
          "name": f"{meta.domain}.{meta.section}.{meta.test_case}",
          "description": strip_variant_from_description(meta.description),
          "syntaxes": [],
          "payloads": [],
          "priorities": [],
          "count": 0,
      }
      cases[key] = entry
    entry["count"] += 1
    entry["priorities"].append(priority_of(meta))
    if meta.syntax and meta.syntax not in entry["syntaxes"]:
      entry["syntaxes"].append(meta.syntax)
    if meta.payloads and meta.payloads not in entry["payloads"]:
      entry["payloads"].append(meta.payloads)
  return list(cases.values())


def generate_test_index(
    run_result: conformance_result_pb2.ConformanceRunResult,
) -> str:
  """Renders the catalog of every test case the harness defines.

  The index describes the suite, not a testee: it lists each test's coordinate,
  identifier and description so that coverage gaps are reviewable without
  running anything.
  """
  test_catalog = {c.test_name: c for c in run_result.results}
  sorted_test_names = sorted(
      test_catalog.keys(),
      key=lambda name: coordinate_sort_key(name, test_catalog[name]),
  )

  grouped_required, grouped_informational, domain_names, section_names = (
      group_by_section(test_catalog, sorted_test_names)
  )

  time_str = time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime())
  total = len(sorted_test_names)

  # Per-section test case lists, computed once and reused by the contents
  # listing and the section tables.
  section_cases: Dict[str, List[Dict[str, Any]]] = {}
  for sections in grouped_required.values():
    for section_key, tests in sections.items():
      section_cases[section_key] = collapse_to_test_cases(test_catalog, tests)
  for info_sec_key, tests in grouped_informational.items():
    section_cases[info_sec_key] = collapse_to_test_cases(test_catalog, tests)

  info_case_total = sum(len(section_cases[k]) for k in grouped_informational)
  case_total = sum(len(v) for v in section_cases.values())

  lines = [
      "# Protobuf Conformance Test Index",
      "",
      "> **Catalog of every test case defined by the conformance suite**",
      "> ",
      f"> Generated on {time_str}",
      "",
      (
          f"This index covers **{case_total}** test cases"
          f" ({case_total - info_case_total} specification, {info_case_total}"
          f" informational), expanded into **{total}** executed tests across"
          f" {len(grouped_required) + (1 if grouped_informational else 0)}"
          " domains. Every test has two parallel identifiers with a 1:1"
          " component mapping: the coordinate"
          " `<domain>.<section>.<case>.<variant>` (e.g. `01.01.001.31`) and"
          " the greppable name `<domain>.<section>.<case>.<variant>` (e.g."
          " `wire.varint.eof_before_unknown_value_double_parsefails"
          ".ed_proto2_pb2pb`). This index is keyed on the first three"
          " components; the variant axis is summarized in the Syntaxes and"
          " Payloads columns."
      ),
      "",
      "## Contents",
      "",
  ]

  # Table of contents.
  for domain_key, sections in grouped_required.items():
    title = domain_title_for(domain_key, domain_names.get(domain_key, ""))
    domain_idx = int(domain_key) if domain_key.isdigit() else domain_key
    count = sum(len(section_cases[k]) for k in sections)
    lines.append(
        f"- [{domain_idx}. {title}](#{domain_idx}-{title.lower()}) "
        f"- {count} test cases"
    )
  if grouped_informational:
    lines.append(
        "- [999. Informational](#999-informational) "
        f"- {info_case_total} test cases"
    )

  def render_section(section_key: str, heading: str):
    cases = section_cases[section_key]
    runs = sum(c["count"] for c in cases)
    lines.extend([
        "",
        f"### {heading}",
        "",
        f"{len(cases)} test cases, {runs} executed tests.",
        "",
        (
            "| Coordinate | Test Case | Priority | Syntaxes | Payloads |"
            " Description |"
        ),
        "|:---|:---|:---:|:---|:---|:---|",
    ])
    for case in cases:
      description = case["description"].replace("|", "\\|")
      syntaxes = ", ".join(case["syntaxes"]) or "-"
      payloads = ", ".join(case["payloads"]) or "-"
      priority = priority_summary(case["priorities"])
      lines.append(
          f"| `{case['coordinate']}` | `{case['name']}` | {priority} |"
          f" {syntaxes} | {payloads} | {description} |"
      )

  for domain_key, sections in grouped_required.items():
    title = domain_title_for(domain_key, domain_names.get(domain_key, ""))
    domain_idx = int(domain_key) if domain_key.isdigit() else domain_key
    count = sum(len(section_cases[k]) for k in sections)
    lines.extend([
        "",
        f"## {domain_idx}. {title}",
        "",
        f"{count} test cases across {len(sections)} sections.",
    ])
    for section_key in sections:
      sec_title = section_title_for(
          section_key, section_names.get(section_key, "")
      )
      render_section(
          section_key,
          f"{section_heading_number(section_key)} {sec_title}",
      )

  if grouped_informational:
    lines.extend([
        "",
        "## 999. Informational",
        "",
        (
            f"{info_case_total} test cases. Informational tests record which"
            " legal behavior an implementation chose; they are never pass/fail"
            " on their own."
        ),
    ])
    for info_sec_key in grouped_informational:
      sec_title = KNOWN_SECTIONS.get(info_sec_key, "General")
      render_section(info_sec_key, f"{info_sec_key} {sec_title}")

  lines.append("")
  return "\n".join(lines)


def generate_single_report(
    run_result: conformance_result_pb2.ConformanceRunResult,
) -> str:
  impl = run_result.implementation_name or "Unknown"

  test_catalog = {c.test_name: c for c in run_result.results}
  sorted_test_names = sorted(
      test_catalog.keys(),
      key=lambda name: coordinate_sort_key(name, test_catalog[name]),
  )

  impl_results = {
      c.test_name: {
          "status": get_status_str(c.status),
          "matched_behavior_id": c.matched_behavior_id,
          "error_message": c.error_message,
      }
      for c in run_result.results
  }

  m = calculate_metrics(sorted_test_names, test_catalog, impl_results)
  time_str = time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime())

  lines = [
      f"# Protobuf Conformance Report: {impl}",
      "",
      f"> **Specification Conformance Results for {impl}**",
      "> ",
      f"> Generated on {time_str}",
      "",
      "## Executive Summary",
      "",
      "| Metric | Value |",
      "|:---|:---:|",
      f"| **Total Tests** | {m['total']} |",
      (
          f"| **Required Pass Rate** | {m['required_pass_pct']:.1f}%"
          f" ({m['required_passed']}/{m['required_total']}) |"
      ),
      (
          f"| **Informational Alternate Rate** | {m['info_alt_pct']:.1f}%"
          f" ({m['info_alternate']}/{m['info_total']}) |"
      ),
      f"| **Pass Canonical** | {m['canonical']} |",
      f"| **Pass Alternate** | {m['alternate']} |",
      f"| **Pass Other** | {m['other']} |",
      f"| **Failures** | {m['failures']} |",
      f"| **Crashes** | {m['crashes']} |",
      f"| **Skipped** | {m['skipped']} |",
      "",
  ]

  # Priority Rollup
  priority_stats: Dict[str, Dict[str, int]] = {}
  for name in sorted_test_names:
    priority = priority_of(test_catalog[name])
    stats = priority_stats.setdefault(
        priority, {"total": 0, "pass": 0, "fail": 0, "skip": 0, "crash": 0}
    )
    stats["total"] += 1
    st = impl_results[name]["status"]
    if st in PASSING_STATUSES:
      stats["pass"] += 1
    elif st == "FAILURE":
      stats["fail"] += 1
    elif st == "SKIPPED":
      stats["skip"] += 1
    elif st == "CRASH":
      stats["crash"] += 1

  lines.extend([
      "## Priority Rollup",
      "",
      "| Priority | Total | Pass | Fail | Skip | Crash | Pass Rate |",
      "|:---|:---:|:---:|:---:|:---:|:---:|:---:|",
  ])
  for priority in sorted(priority_stats.keys()):
    st = priority_stats[priority]
    rate = (st["pass"] / st["total"] * 100.0) if st["total"] > 0 else 0.0
    lines.append(
        f"| **{priority}** | {st['total']} | {st['pass']} | {st['fail']} |"
        f" {st['skip']} | {st['crash']} | {rate:.1f}% |"
    )
  lines.append("")

  # Domain Rollup
  domain_stats: Dict[str, Dict[str, int]] = {}
  for name in sorted_test_names:
    c = test_catalog[name]
    coord_parts = (c.coordinate or "00.00.000.00").split(".")
    domain_key = coord_parts[0] if coord_parts else "00"
    dom_title = KNOWN_DOMAINS.get(
        domain_key, (c.domain or "General").replace("_", " ").title()
    )
    full_dom_title = (
        f"{int(domain_key) if domain_key.isdigit() else domain_key}."
        f" {dom_title}"
    )

    if full_dom_title not in domain_stats:
      domain_stats[full_dom_title] = {
          "total": 0,
          "pass": 0,
          "fail": 0,
          "skip": 0,
          "crash": 0,
      }
    domain_stats[full_dom_title]["total"] += 1
    st = impl_results[name]["status"]
    if st in PASSING_STATUSES:
      domain_stats[full_dom_title]["pass"] += 1
    elif st == "FAILURE":
      domain_stats[full_dom_title]["fail"] += 1
    elif st == "SKIPPED":
      domain_stats[full_dom_title]["skip"] += 1
    elif st == "CRASH":
      domain_stats[full_dom_title]["crash"] += 1

  lines.extend([
      "## Domain Rollup",
      "",
      "| Domain | Total | Pass | Fail | Skip | Crash | Pass Rate |",
      "|:---|:---:|:---:|:---:|:---:|:---:|:---:|",
  ])
  for dom in sorted(domain_stats.keys()):
    st = domain_stats[dom]
    dom_rate = (st["pass"] / st["total"] * 100.0) if st["total"] > 0 else 0.0
    lines.append(
        f"| **{dom}** | {st['total']} | {st['pass']} | {st['fail']} |"
        f" {st['skip']} | {st['crash']} | {dom_rate:.1f}% |"
    )
  lines.append("")

  # Failures / Crashes Section
  failures = [
      name
      for name in sorted_test_names
      if impl_results[name]["status"] in ("FAILURE", "CRASH")
  ]
  if failures:
    lines.extend([
        f"## Failures & Crashes ({len(failures)})",
        "",
        (
            "| Coordinate | Test Case | Priority | Status | Section |"
            " Error Message |"
        ),
        "|:---|:---|:---:|:---:|:---|:---|",
    ])
    for name in failures:
      c = test_catalog[name]
      st = impl_results[name]["status"]
      badge = format_badge_md(st, is_informational=c.is_informational)
      sec = (
          f"{c.domain.replace('_', ' ').title()} >"
          f" {c.section.replace('_', ' ').title()}"
      )
      err = (
          (impl_results[name]["error_message"] or "").replace("\n", " ").strip()
      )
      err = err.replace("|", "\\|")
      if len(err) > 90:
        err = err[:87] + "..."
      lines.append(
          f"| `{c.coordinate or '-'}` | `{name}` | {priority_of(c)} | {badge} |"
          f" {sec} | {err} |"
      )
    lines.append("")

  # Informational Section
  info_cases = [
      name for name in sorted_test_names if test_catalog[name].is_informational
  ]
  if info_cases:
    lines.extend([
        f"## Informational Behaviors ({len(info_cases)})",
        "",
        (
            "| Coordinate | Test Case | Priority | Section | Matched Behavior |"
            " Status |"
        ),
        "|:---|:---|:---:|:---|:---:|:---:|",
    ])
    for name in info_cases:
      c = test_catalog[name]
      st = impl_results[name]["status"]
      matched = impl_results[name]["matched_behavior_id"] or "canonical"
      badge = format_badge_md(st, matched, is_informational=True)
      sec = (
          f"{c.domain.replace('_', ' ').title()} >"
          f" {c.section.replace('_', ' ').title()}"
      )
      lines.append(
          f"| `{c.coordinate or '-'}` | `{name}` | {priority_of(c)} | {sec} |"
          f" `{matched}` | {badge} |"
      )
    lines.append("")

  return "\n".join(lines)


def generate_master_matrix(
    runs: List[conformance_result_pb2.ConformanceRunResult],
) -> str:
  implementations = [
      r.implementation_name or f"Impl_{i}" for i, r in enumerate(runs)
  ]

  # Index all unique tests
  test_catalog: Dict[str, conformance_result_pb2.ConformanceTestCaseResult] = {}
  run_results: Dict[str, Dict[str, Dict[str, Any]]] = {
      impl: {} for impl in implementations
  }

  for r, impl in zip(runs, implementations):
    for c in r.results:
      if c.test_name not in test_catalog:
        test_catalog[c.test_name] = c
      run_results[impl][c.test_name] = {
          "status": get_status_str(c.status),
          "matched_behavior_id": c.matched_behavior_id,
          "error_message": c.error_message,
          "execution_time_us": c.execution_time_us,
      }

  sorted_test_names = sorted(
      test_catalog.keys(),
      key=lambda name: coordinate_sort_key(name, test_catalog[name]),
  )

  # Calculate metrics per implementation
  metrics: Dict[str, Dict[str, Any]] = {}
  for impl in implementations:
    metrics[impl] = calculate_metrics(
        sorted_test_names, test_catalog, run_results[impl]
    )

  time_str = time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime())
  lines = [
      "# Protobuf Conformance Parity Matrix",
      "",
      "> **Specification Conformance and Behavioral Parity Results**",
      "> ",
      f"> Generated on {time_str}",
      "",
      "## Executive Summary",
      "",
      "| Metric | " + " | ".join(impl for impl in implementations) + " |",
      "|:---| " + " | ".join(":---:" for _ in implementations) + " |",
  ]

  lines.append(
      "| **Total Tests** | "
      + " | ".join(str(metrics[impl]["total"]) for impl in implementations)
      + " |"
  )
  lines.append(
      "| **Required Pass Rate** | "
      + " | ".join(
          f"{metrics[impl]['required_pass_pct']:.1f}%"
          f" ({metrics[impl]['required_passed']}/{metrics[impl]['required_total']})"
          for impl in implementations
      )
      + " |"
  )
  lines.append(
      "| **Informational Alternate Rate** | "
      + " | ".join(
          f"{metrics[impl]['info_alt_pct']:.1f}%"
          f" ({metrics[impl]['info_alternate']}/{metrics[impl]['info_total']})"
          for impl in implementations
      )
      + " |"
  )
  lines.append(
      "| **Pass Canonical** | "
      + " | ".join(str(metrics[impl]["canonical"]) for impl in implementations)
      + " |"
  )
  lines.append(
      "| **Pass Alternate** | "
      + " | ".join(str(metrics[impl]["alternate"]) for impl in implementations)
      + " |"
  )
  lines.append(
      "| **Pass Other** | "
      + " | ".join(str(metrics[impl]["other"]) for impl in implementations)
      + " |"
  )
  lines.append(
      "| **Failures** | "
      + " | ".join(str(metrics[impl]["failures"]) for impl in implementations)
      + " |"
  )
  lines.append(
      "| **Crashes** | "
      + " | ".join(str(metrics[impl]["crashes"]) for impl in implementations)
      + " |"
  )
  lines.append(
      "| **Skipped** | "
      + " | ".join(str(metrics[impl]["skipped"]) for impl in implementations)
      + " |"
  )

  # Group test cases by domain, then by section.
  grouped_required, grouped_informational, domain_names, section_names = (
      group_by_section(test_catalog, sorted_test_names)
  )

  # 1. Render all REQUIRED domains and sections
  for domain_key, sections in grouped_required.items():
    if domain_key in KNOWN_DOMAINS:
      domain_title = KNOWN_DOMAINS[domain_key]
    else:
      domain_raw = domain_names.get(domain_key, "General")
      clean_dom = re.sub(r"^\d+\.\s*", "", domain_raw).strip()
      domain_title = (
          "API"
          if clean_dom.lower() == "api"
          else clean_dom.replace("_", " ").title()
      )

    domain_idx = int(domain_key) if domain_key.isdigit() else domain_key
    lines.extend([
        "",
        f"## {domain_idx}. {domain_title}",
    ])

    for section_key, tests in sections.items():
      if section_key in KNOWN_SECTIONS:
        sec_title = KNOWN_SECTIONS[section_key]
      else:
        sec_raw = section_names.get(section_key, "General")
        clean_sec = re.sub(r"^\d+\.\d+\s*", "", sec_raw).strip()
        sec_title = (
            "Length-Delimited"
            if clean_sec.lower() == "length_delimited"
            else clean_sec.replace("_", " ").title()
        )

      sec_parts = section_key.split(".")
      sec_heading_num = (
          f"{int(sec_parts[0])}.{sec_parts[1]}"
          if len(sec_parts) > 1
          else section_key
      )

      lines.extend([
          "",
          f"### {sec_heading_num} {sec_title}",
          "",
          "| Coordinate | Test Case | Priority | "
          + " | ".join(impl for impl in implementations)
          + " |",
          "|:---|:---|:---:| "
          + " | ".join(":---:" for _ in implementations)
          + " |",
      ])

      for test_name in tests:
        impl_cells = []
        for impl in implementations:
          res = run_results.get(impl, {}).get(test_name, {})
          st = res.get("status", "MISSING")
          matched_id = res.get("matched_behavior_id", "")
          impl_cells.append(
              format_badge_md(st, matched_id, is_informational=False)
          )

        coord = test_catalog[test_name].coordinate or "-"
        priority = priority_of(test_catalog[test_name])
        lines.append(
            f"| `{coord}` | `{test_name}` | {priority} | "
            + " | ".join(impl_cells)
            + " |"
        )

  # 2. Render INFORMATIONAL domain: 999. Informational with 999.x sections
  if grouped_informational:
    lines.extend([
        "",
        "## 999. Informational",
    ])

    for info_sec_key, tests in grouped_informational.items():
      sec_title = KNOWN_SECTIONS.get(info_sec_key, "General")
      lines.extend([
          "",
          f"### {info_sec_key} {sec_title}",
          "",
          "| Coordinate | Test Case | Priority | "
          + " | ".join(impl for impl in implementations)
          + " |",
          "|:---|:---|:---:| "
          + " | ".join(":---:" for _ in implementations)
          + " |",
      ])

      for test_name in tests:
        impl_cells = []
        for impl in implementations:
          res = run_results.get(impl, {}).get(test_name, {})
          st = res.get("status", "MISSING")
          matched_id = res.get("matched_behavior_id", "")
          impl_cells.append(
              format_badge_md(st, matched_id, is_informational=True)
          )

        coord = test_catalog[test_name].coordinate or "-"
        priority = priority_of(test_catalog[test_name])
        lines.append(
            f"| `{coord}` | `{test_name}` | {priority} | "
            + " | ".join(impl_cells)
            + " |"
        )

  lines.append("")
  return "\n".join(lines)


def render_html_matrix(
    runs: List[conformance_result_pb2.ConformanceRunResult],
) -> str:
  implementations = [
      r.implementation_name or f"Impl_{i}" for i, r in enumerate(runs)
  ]
  test_catalog: Dict[str, conformance_result_pb2.ConformanceTestCaseResult] = {}
  run_results: Dict[str, Dict[str, Dict[str, Any]]] = {
      impl: {} for impl in implementations
  }

  for r, impl in zip(runs, implementations):
    for c in r.results:
      if c.test_name not in test_catalog:
        test_catalog[c.test_name] = c
      run_results[impl][c.test_name] = {
          "status": get_status_str(c.status),
          "matched_behavior_id": c.matched_behavior_id,
          "error_message": c.error_message,
          "execution_time_us": c.execution_time_us,
      }

  sorted_test_names = sorted(
      test_catalog.keys(),
      key=lambda name: coordinate_sort_key(name, test_catalog[name]),
  )

  metrics: Dict[str, Dict[str, Any]] = {}
  for impl in implementations:
    metrics[impl] = calculate_metrics(
        sorted_test_names, test_catalog, run_results[impl]
    )

  primary_impl = implementations[0] if implementations else "default"
  m = metrics.get(
      primary_impl,
      {
          "total": len(sorted_test_names),
          "required_total": 0,
          "required_passed": 0,
          "required_pass_pct": 0.0,
          "info_total": 0,
          "info_alternate": 0,
          "info_alt_pct": 0.0,
          "failures": 0,
          "crashes": 0,
          "canonical": 0,
          "alternate": 0,
          "other": 0,
      },
  )

  domains = sorted(
      list({
          test_catalog[t].domain
          for t in sorted_test_names
          if test_catalog[t].domain
      })
  )

  time_str = time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime())

  html_lines = [
      "<!DOCTYPE html>",
      '<html lang="en">',
      "<head>",
      '  <meta charset="UTF-8">',
      (
          '  <meta name="viewport" content="width=device-width,'
          ' initial-scale=1.0">'
      ),
      "  <title>Protobuf Conformance Parity Matrix</title>",
      "  <style>",
      "    :root {",
      "      --bg-primary: #0f172a;",
      "      --bg-secondary: #1e293b;",
      "      --bg-tertiary: #334155;",
      "      --text-primary: #f8fafc;",
      "      --text-secondary: #94a3b8;",
      "      --text-muted: #64748b;",
      "      --accent: #3b82f6;",
      "      --accent-hover: #2563eb;",
      "      --success: #10b981;",
      "      --success-bg: rgba(16, 185, 129, 0.15);",
      "      --warning: #f59e0b;",
      "      --warning-bg: rgba(245, 158, 11, 0.15);",
      "      --danger: #ef4444;",
      "      --danger-bg: rgba(239, 68, 68, 0.15);",
      "      --cyan: #06b6d4;",
      "      --cyan-bg: rgba(6, 182, 212, 0.15);",
      "      --border: #334155;",
      "      --border-light: #475569;",
      "    }",
      "    * { box-sizing: border-box; margin: 0; padding: 0; }",
      "    body {",
      (
          "      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI',"
          " Roboto, 'Helvetica Neue', Arial, sans-serif;"
      ),
      "      background-color: var(--bg-primary);",
      "      color: var(--text-primary);",
      "      line-height: 1.5;",
      "      padding: 2rem;",
      "    }",
      "    .container { max-width: 1400px; margin: 0 auto; }",
      (
          "    header { margin-bottom: 2rem; border-bottom: 1px solid"
          " var(--border); padding-bottom: 1.5rem; }"
      ),
      (
          "    .header-top { display: flex; justify-content: space-between;"
          " align-items: flex-start; }"
      ),
      (
          "    h1 { font-size: 1.875rem; font-weight: 700; color: #fff;"
          " display: flex; align-items: center; gap: 0.75rem; }"
      ),
      (
          "    h1 .version-badge { font-size: 0.75rem; background:"
          " var(--accent); color: white; padding: 0.2rem 0.6rem; border-radius:"
          " 9999px; font-weight: 600; text-transform: uppercase;"
          " letter-spacing: 0.05em; }"
      ),
      (
          "    .subtitle { color: var(--text-secondary); margin-top: 0.5rem;"
          " font-size: 0.95rem; }"
      ),
      "    .timestamp { font-size: 0.8rem; color: var(--text-muted); }",
      (
          "    .metrics-grid { display: grid; grid-template-columns:"
          " repeat(auto-fit, minmax(200px, 1fr)); gap: 1.25rem; margin-bottom:"
          " 2rem; }"
      ),
      (
          "    .metric-card { background: var(--bg-secondary); border: 1px"
          " solid var(--border); border-radius: 0.75rem; padding: 1.25rem;"
          " position: relative; overflow: hidden; }"
      ),
      (
          "    .metric-card::before { content: ''; position: absolute; top: 0;"
          " left: 0; right: 0; height: 3px; background: var(--accent); }"
      ),
      "    .metric-card.success::before { background: var(--success); }",
      "    .metric-card.warning::before { background: var(--warning); }",
      "    .metric-card.cyan::before { background: var(--cyan); }",
      "    .metric-card.danger::before { background: var(--danger); }",
      (
          "    .metric-title { font-size: 0.85rem; font-weight: 500; color:"
          " var(--text-secondary); text-transform: uppercase; letter-spacing:"
          " 0.05em; }"
      ),
      (
          "    .metric-value { font-size: 2rem; font-weight: 700; margin-top:"
          " 0.25rem; color: #fff; }"
      ),
      (
          "    .metric-sub { font-size: 0.8rem; color: var(--text-muted);"
          " margin-top: 0.25rem; }"
      ),
      (
          "    .progress-bar { width: 100%; height: 6px; background:"
          " var(--border); border-radius: 9999px; margin-top: 0.75rem;"
          " overflow: hidden; }"
      ),
      (
          "    .progress-fill { height: 100%; background: var(--success);"
          " border-radius: 9999px; }"
      ),
      (
          "    .toolbar { background: var(--bg-secondary); border: 1px solid"
          " var(--border); border-radius: 0.75rem; padding: 1rem 1.25rem;"
          " display: flex; flex-wrap: wrap; gap: 1rem; align-items: center;"
          " justify-content: space-between; margin-bottom: 1.5rem; }"
      ),
      (
          "    .toolbar-group { display: flex; align-items: center; gap:"
          " 0.75rem; flex-wrap: wrap; }"
      ),
      (
          "    .search-input { background: var(--bg-primary); border: 1px solid"
          " var(--border); color: var(--text-primary); padding: 0.5rem 1rem;"
          " border-radius: 0.5rem; font-size: 0.9rem; min-width: 280px;"
          " outline: none; }"
      ),
      (
          "    .filter-btn { background: var(--bg-primary); border: 1px solid"
          " var(--border); color: var(--text-secondary); padding: 0.45rem"
          " 0.85rem; border-radius: 0.5rem; font-size: 0.85rem; font-weight:"
          " 500; cursor: pointer; }"
      ),
      (
          "    .filter-btn.active { background: var(--accent); border-color:"
          " var(--accent); color: white; }"
      ),
      (
          "    .select-filter { background: var(--bg-primary); border: 1px"
          " solid var(--border); color: var(--text-primary); padding: 0.45rem"
          " 0.85rem; border-radius: 0.5rem; font-size: 0.85rem; outline:"
          " none; }"
      ),
      (
          "    .table-container { background: var(--bg-secondary); border: 1px"
          " solid var(--border); border-radius: 0.75rem; overflow-x: auto; }"
      ),
      "    table { width: 100%; border-collapse: collapse; text-align: left; }",
      (
          "    th { background: #182234; padding: 0.85rem 1rem; font-size:"
          " 0.8rem; font-weight: 600; text-transform: uppercase;"
          " letter-spacing: 0.05em; color: var(--text-secondary);"
          " border-bottom: 1px solid var(--border); }"
      ),
      (
          "    td { padding: 0.85rem 1rem; font-size: 0.875rem; border-bottom:"
          " 1px solid rgba(51, 65, 85, 0.4); }"
      ),
      "    tr:hover td { background: rgba(255, 255, 255, 0.02); }",
      (
          "    .badge { display: inline-flex; align-items: center; gap:"
          " 0.35rem; padding: 0.35rem 0.75rem; border-radius: 9999px;"
          " font-size: 0.8rem; font-weight: 600; white-space: nowrap; }"
      ),
      (
          "    .badge-canonical { background: var(--success-bg); color:"
          " #34d399; border: 1px solid rgba(52, 211, 153, 0.3); }"
      ),
      (
          "    .badge-alternate { background: var(--cyan-bg); color: #38bdf8;"
          " border: 1px solid rgba(56, 189, 248, 0.3); }"
      ),
      (
          "    .badge-other { background: var(--warning-bg); color: #fbbf24;"
          " border: 1px solid rgba(251, 191, 36, 0.3); }"
      ),
      (
          "    .badge-failure { background: var(--danger-bg); color: #f87171;"
          " border: 1px solid rgba(248, 113, 113, 0.3); }"
      ),
      (
          "    .badge-crash { background: rgba(239, 68, 68, 0.3); color:"
          " #fca5a5; border: 1px solid #ef4444; }"
      ),
      (
          "    .badge-skipped { background: rgba(148, 163, 184, 0.15); color:"
          " #94a3b8; border: 1px solid rgba(148, 163, 184, 0.3); }"
      ),
      (
          "    .badge-missing { background: transparent; color:"
          " var(--text-muted); border: 1px dashed var(--border); }"
      ),
      (
          "    .test-name { font-family: ui-monospace, SFMono-Regular, Menlo,"
          " Monaco, Consolas, monospace; font-size: 0.825rem; color:"
          " var(--text-primary); }"
      ),
      "  </style>",
      "</head>",
      "<body>",
      '  <div class="container">',
      "    <header>",
      '      <div class="header-top">',
      "        <div>",
      (
          "          <h1>Protobuf Conformance Parity Matrix <span"
          ' class="version-badge">Live</span></h1>'
      ),
      (
          '          <div class="subtitle">Hierarchical coordinate taxonomy'
          " &amp; multi-implementation behavioral matrix.</div>"
      ),
      "        </div>",
      f'        <div class="timestamp">Generated: {time_str}</div>',
      "      </div>",
      "    </header>",
      '    <div class="metrics-grid">',
      '      <div class="metric-card">',
      '        <div class="metric-title">Total Tests</div>',
      f'        <div class="metric-value">{m["total"]}</div>',
      (
          f'        <div class="metric-sub">{len(implementations)}'
          " implementations active</div>"
      ),
      "      </div>",
      '      <div class="metric-card success">',
      '        <div class="metric-title">Required Pass Rate</div>',
      f'        <div class="metric-value">{m["required_pass_pct"]:.1f}%</div>',
      (
          f'        <div class="metric-sub">{m["required_passed"]} /'
          f' {m["required_total"]} specifications passed</div>'
      ),
      '        <div class="progress-bar">',
      (
          '          <div class="progress-fill" style="width:'
          f' {m["required_pass_pct"]}%;"></div>'
      ),
      "        </div>",
      "      </div>",
      '      <div class="metric-card cyan">',
      '        <div class="metric-title">Informational Alternate</div>',
      f'        <div class="metric-value">{m["info_alt_pct"]:.1f}%</div>',
      (
          f'        <div class="metric-sub">{m["info_alternate"]} /'
          f' {m["info_total"]} accepted variants</div>'
      ),
      "      </div>",
      (
          '      <div class="metric-card'
          f' {"danger" if m["failures"] > 0 else "success"}">'
      ),
      '        <div class="metric-title">Failures</div>',
      f'        <div class="metric-value">{m["failures"]}</div>',
      '        <div class="metric-sub">Specification non-compliances</div>',
      "      </div>",
      (
          '      <div class="metric-card'
          f' {"danger" if m["crashes"] > 0 else "success"}">'
      ),
      '        <div class="metric-title">Crashes</div>',
      f'        <div class="metric-value">{m["crashes"]}</div>',
      '        <div class="metric-sub">Process terminations or timeouts</div>',
      "      </div>",
      "    </div>",
      '    <div class="toolbar">',
      '      <div class="toolbar-group">',
      (
          '        <input type="text" id="searchInput" class="search-input"'
          ' placeholder="Search coordinate, name, section...">'
      ),
      '        <button class="filter-btn active" data-filter="ALL">All ('
      + str(len(sorted_test_names))
      + ")</button>",
      '        <button class="filter-btn" data-filter="CANONICAL">Canonical ('
      + str(m["canonical"])
      + ")</button>",
      '        <button class="filter-btn" data-filter="FAILURE">Failures ('
      + str(m["failures"])
      + ")</button>",
      '        <button class="filter-btn" data-filter="SKIPPED">Skipped ('
      + str(m["skipped"])
      + ")</button>",
      "      </div>",
      '      <div class="toolbar-group">',
      '        <select id="domainSelect" class="select-filter">',
      '          <option value="ALL">All Domains</option>',
  ]

  for d in domains:
    html_lines.append(
        f'          <option value="{html.escape(d)}">Domain:'
        f" {html.escape(d)}</option>"
    )

  html_lines.extend([
      "        </select>",
      "      </div>",
      "    </div>",
      '    <div class="table-container">',
      "      <table>",
      "        <thead>",
      "          <tr>",
      "            <th>Coordinate</th>",
      "            <th>Domain / Section</th>",
      "            <th>Test Identifier</th>",
      "            <th>Priority</th>",
  ])

  for impl in implementations:
    html_lines.append(f"            <th>{html.escape(impl)}</th>")

  html_lines.extend([
      "          </tr>",
      "        </thead>",
      "        <tbody>",
  ])

  for name in sorted_test_names:
    c = test_catalog[name]
    coord = c.coordinate or "00.00.000.00"
    dom_sec = f"{c.domain} / {c.section}"

    html_lines.append(f'          <tr data-domain="{html.escape(c.domain)}">')
    html_lines.append(f"            <td><code>{html.escape(coord)}</code></td>")
    html_lines.append(f"            <td>{html.escape(dom_sec)}</td>")
    html_lines.append(
        f'            <td class="test-name">{html.escape(name)}</td>'
    )
    html_lines.append(f"            <td>{html.escape(priority_of(c))}</td>")

    for impl in implementations:
      res = run_results.get(impl, {}).get(name, {})
      st = res.get("status", "MISSING")
      matched_id = res.get("matched_behavior_id", "")
      badge_html = format_badge_html(
          st, matched_id, is_informational=c.is_informational
      )
      html_lines.append(f"            <td>{badge_html}</td>")

    html_lines.append("          </tr>")

  html_lines.extend([
      "        </tbody>",
      "      </table>",
      "    </div>",
      "  </div>",
      "  <script>",
      "    document.addEventListener('DOMContentLoaded', () => {",
      "      const searchInput = document.getElementById('searchInput');",
      "      const domainSelect = document.getElementById('domainSelect');",
      "      const filterButtons = document.querySelectorAll('.filter-btn');",
      "      const rows = document.querySelectorAll('tbody tr');",
      "      let currentSearch = '';",
      "      let currentDomain = 'ALL';",
      "      let currentStatus = 'ALL';",
      "      function applyFilters() {",
      "        rows.forEach(row => {",
      "          const text = row.innerText.toLowerCase();",
      "          const domain = row.getAttribute('data-domain');",
      (
          "          const matchesSearch = !currentSearch ||"
          " text.includes(currentSearch);"
      ),
      (
          "          const matchesDomain = currentDomain === 'ALL' || domain"
          " === currentDomain;"
      ),
      (
          "          row.style.display = matchesSearch && matchesDomain ? '' :"
          " 'none';"
      ),
      "        });",
      "      }",
      (
          "      searchInput.addEventListener('input', (e) => { currentSearch ="
          " e.target.value.toLowerCase(); applyFilters(); });"
      ),
      (
          "      domainSelect.addEventListener('change', (e) => { currentDomain"
          " = e.target.value; applyFilters(); });"
      ),
      "      filterButtons.forEach(btn => {",
      "        btn.addEventListener('click', () => {",
      "          filterButtons.forEach(b => b.classList.remove('active'));",
      "          btn.classList.add('active');",
      "          currentStatus = btn.getAttribute('data-filter');",
      "          applyFilters();",
      "        });",
      "      });",
      "    });",
      "  </script>",
      "</body>",
      "</html>",
  ])

  return "\n".join(html_lines)


def main(argv=None):
  parser = argparse.ArgumentParser(
      description="Generate Conformance Markdown and HTML Reports."
  )
  parser.add_argument(
      "--mode",
      choices=["single", "master", "index"],
      default="single",
      help=(
          "Report generation mode: 'single' for one testee, 'master' for a"
          " cross-implementation matrix, or 'index' for the test catalog."
      ),
  )
  parser.add_argument(
      "--input_file",
      action="append",
      dest="input_files",
      help=(
          "Path to ConformanceRunResult textproto file(s). Can be specified"
          " multiple times."
      ),
  )
  parser.add_argument(
      "--output_file",
      type=str,
      default="",
      help="Path to output Markdown file. If omitted, writes to stdout.",
  )
  parser.add_argument(
      "--output_html",
      type=str,
      default="",
      help="Optional path to output interactive HTML matrix (in master mode).",
  )
  parser.add_argument(
      "positional_inputs",
      nargs="*",
      help="Positional input textproto files.",
  )

  args = parser.parse_args(argv)
  all_inputs = (args.input_files or []) + args.positional_inputs
  if not all_inputs:
    print("Error: No input files specified.", file=sys.stderr)
    sys.exit(1)

  if args.mode == "single":
    if len(all_inputs) > 1:
      print(
          "Warning: Multiple inputs provided for 'single' mode. Using the first"
          " one.",
          file=sys.stderr,
      )
    run_result = load_run_result(all_inputs[0])
    report_md = generate_single_report(run_result)
  elif args.mode == "index":
    if len(all_inputs) > 1:
      print(
          "Warning: Multiple inputs provided for 'index' mode. Using the first"
          " one.",
          file=sys.stderr,
      )
    report_md = generate_test_index(load_run_result(all_inputs[0]))
  else:
    runs = [load_run_result(f) for f in all_inputs]
    report_md = generate_master_matrix(runs)
    if args.output_html:
      out_html = resolve_path(args.output_html)
      os.makedirs(os.path.dirname(out_html), exist_ok=True)
      html_content = render_html_matrix(runs)
      with open(out_html, "w", encoding="utf-8") as f:
        f.write(html_content)
      print(f"HTML matrix written to {out_html}")

  if args.output_file:
    out_file = resolve_path(args.output_file)
    os.makedirs(os.path.dirname(out_file), exist_ok=True)
    with open(out_file, "w", encoding="utf-8") as f:
      f.write(report_md)
    print(f"Report written to {out_file}")
  else:
    print(report_md)


if __name__ == "__main__":
  main()

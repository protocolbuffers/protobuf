# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Microbenchmarks for JSON format parsing and custom enum names."""

import statistics
import timeit
import unittest

from google.protobuf import json_format

from google.protobuf.json import json_enumval_custom_string_pb2


class JsonFormatBenchmark(unittest.TestCase):
  """Microbenchmarks for JSON format parsing."""

  def test_benchmark_parse_json(self):
    """Benchmarks parsing of default, custom, and unknown enum names in JSON."""
    default_payloads = [
        '{"armor":"ARMOR_GREAT_HELM"}',
        '{"armor":"ARMOR_GAUNTLET"}',
        '{"armor":"ARMOR_PLATE"}',
        '{"armor":"ARMOR_COIF"}',
        '{"armor":"ARMOR_PAULDRON"}',
        '{"armor":"ARMOR_SABATON"}',
        '{"armor":"ARMOR_HACHI_MAI_DO"}',
    ]
    custom_payloads = [
        '{"armor":"gr8 helm"}',
        '{"armor":"a\\"b"}',
        '{"armor":"\\"plate\\""}',
        '{"armor":""}',
        '{"armor":"p\\taul\\ndron"}',
        '{"armor":"sabaton"}',
        '{"armor":"8"}',
    ]
    unknown_payloads = [
        '{"armor":"UNKNOWN_1"}',
        '{"armor":"UNKNOWN_2"}',
        '{"armor":"UNKNOWN_3"}',
        '{"armor":"UNKNOWN_4"}',
        '{"armor":"UNKNOWN_5"}',
        '{"armor":"UNKNOWN_6"}',
        '{"armor":"UNKNOWN_7"}',
    ]

    repeated_default_payload = (
        '{"armors":['
        + ','.join(
            [
                '"ARMOR_GREAT_HELM"',
                '"ARMOR_GAUNTLET"',
                '"ARMOR_PLATE"',
                '"ARMOR_COIF"',
                '"ARMOR_PAULDRON"',
                '"ARMOR_SABATON"',
                '"ARMOR_HACHI_MAI_DO"',
            ]
            * 14
        )
        + ']}'
    )
    repeated_custom_payload = (
        '{"armors":['
        + ','.join(
            [
                '"gr8 helm"',
                '"a\\"b"',
                '"\\"plate\\""',
                '""',
                '"p\\taul\\ndron"',
                '"sabaton"',
                '"8"',
            ]
            * 14
        )
        + ']}'
    )
    repeated_unknown_payload = (
        '{"armors":['
        + ','.join(
            [
                '"UNKNOWN_1"',
                '"UNKNOWN_2"',
                '"UNKNOWN_3"',
                '"UNKNOWN_4"',
                '"UNKNOWN_5"',
                '"UNKNOWN_6"',
                '"UNKNOWN_7"',
            ]
            * 14
        )
        + ']}'
    )

    iterations = 10
    trials = 100
    num_parses = 10
    num_items = 98

    msg = json_enumval_custom_string_pb2.Knight()

    # Warmup phase to eliminate initial compilation/import overhead
    for _ in range(200):
      for p in custom_payloads:
        msg.Clear()
        json_format.Parse(p, msg)

    def run_def():
      for p in default_payloads:
        for _ in range(num_parses):
          msg.Clear()
          json_format.Parse(p, msg)

    def run_cust():
      for p in custom_payloads:
        for _ in range(num_parses):
          msg.Clear()
          json_format.Parse(p, msg)

    def run_unk():
      for p in unknown_payloads:
        for _ in range(num_parses):
          msg.Clear()
          json_format.Parse(p, msg, ignore_unknown_fields=True)

    def run_rep_def():
      for _ in range(num_parses):
        msg.Clear()
        json_format.Parse(repeated_default_payload, msg)

    def run_rep_cust():
      for _ in range(num_parses):
        msg.Clear()
        json_format.Parse(repeated_custom_payload, msg)

    def run_rep_unk():
      for _ in range(num_parses):
        msg.Clear()
        json_format.Parse(
            repeated_unknown_payload, msg, ignore_unknown_fields=True
        )

    benchmarks = [
        (
            'run_def',
            timeit.Timer(run_def, setup=msg.Clear),
            len(default_payloads) * num_parses,
        ),
        (
            'run_cust',
            timeit.Timer(run_cust, setup=msg.Clear),
            len(custom_payloads) * num_parses,
        ),
        (
            'run_unk',
            timeit.Timer(run_unk, setup=msg.Clear),
            len(unknown_payloads) * num_parses,
        ),
        (
            'run_rep_def',
            timeit.Timer(run_rep_def, setup=msg.Clear),
            num_items * num_parses,
        ),
        (
            'run_rep_cust',
            timeit.Timer(run_rep_cust, setup=msg.Clear),
            num_items * num_parses,
        ),
        (
            'run_rep_unk',
            timeit.Timer(run_rep_unk, setup=msg.Clear),
            num_items * num_parses,
        ),
    ]

    # Interleaved trials to minimize CPU frequency scaling / ordering bias
    benchmark_samples = {name: [] for name, _, _ in benchmarks}
    for _ in range(trials):
      for name, timer, num_ops_per_batch in benchmarks:
        total_ops = iterations * num_ops_per_batch
        elapsed = timer.timeit(number=iterations)
        benchmark_samples[name].append(elapsed * 1e6 / total_ops)

    def stats(samples):
      if len(samples) < 100:
        raise ValueError(
            f'Insufficient samples ({len(samples)}) for statistical'
            ' calculations. At least 100 samples are required.'
        )
      med_val = statistics.median(samples)
      mean_val = statistics.mean(samples)
      stdev_val = statistics.stdev(samples)
      p99_val = statistics.quantiles(samples, n=100)[98]
      return p99_val, med_val, mean_val, stdev_val

    p99_def, med_def, mean_def, std_def = stats(benchmark_samples['run_def'])
    p99_cust, med_cust, mean_cust, std_cust = stats(
        benchmark_samples['run_cust']
    )
    p99_unk, med_unk, mean_unk, std_unk = stats(benchmark_samples['run_unk'])
    p99_rep_def, med_rep_def, mean_rep_def, std_rep_def = stats(
        benchmark_samples['run_rep_def']
    )
    p99_rep_cust, med_rep_cust, mean_rep_cust, std_rep_cust = stats(
        benchmark_samples['run_rep_cust']
    )
    p99_rep_unk, med_rep_unk, mean_rep_unk, std_rep_unk = stats(
        benchmark_samples['run_rep_unk']
    )

    print(
        f'\n[BENCHMARK] ParseJsonDefault:                med {med_def:.2f}'
        f' us/op | p99 {p99_def:.2f} us/op | mean {mean_def:.2f} ±'
        f' {std_def:.2f} us/op'
    )
    print(
        f'[BENCHMARK] ParseJsonCustom:                 med {med_cust:.2f} us/op'
        f' | p99 {p99_cust:.2f} us/op | mean {mean_cust:.2f} ± {std_cust:.2f}'
        ' us/op'
    )
    print(
        '[BENCHMARK] ParseJsonUnknownIgnored:        '
        f' med {med_unk:.2f} us/op | p99 {p99_unk:.2f} us/op | mean'
        f' {mean_unk:.2f} ± {std_unk:.2f} us/op'
    )
    print(
        '[BENCHMARK] ParseRepeatedJsonDefault:       '
        f' med {med_rep_def:.2f} us/item | p99 {p99_rep_def:.2f} us/item |'
        f' mean {mean_rep_def:.2f} ± {std_rep_def:.2f} us/item'
    )
    print(
        '[BENCHMARK] ParseRepeatedJsonCustom:        '
        f' med {med_rep_cust:.2f} us/item | p99 {p99_rep_cust:.2f} us/item |'
        f' mean {mean_rep_cust:.2f} ± {std_rep_cust:.2f} us/item'
    )
    print(
        '[BENCHMARK] ParseRepeatedJsonUnknownIgnored:'
        f' med {med_rep_unk:.2f} us/item | p99 {p99_rep_unk:.2f} us/item |'
        f' mean {mean_rep_unk:.2f} ± {std_rep_unk:.2f} us/item'
    )


if __name__ == '__main__':
  unittest.main()

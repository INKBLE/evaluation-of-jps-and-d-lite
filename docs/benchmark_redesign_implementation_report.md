# Benchmark redesign implementation report

## Implemented files

* `/home/inkble/MAMMwork/PathSearchFix.cpp`: legacy benchmark code remains
  disabled and includes the new runner.
* `/home/inkble/MAMMwork/BenchmarkRunner.inc`: CLI, reproducible map/pair
  generation, local octile parsing, validation, manifests, raw records, and
  static/dynamic execution.
* `/home/inkble/MAMMwork/tools/analyze_benchmarks.py`: standard-library
  two-stage summaries and configuration-scoped paired bootstrap comparisons.
* `/home/inkble/MAMMwork/README_BENCHMARKS.md`,
  `/home/inkble/MAMMwork/data/movingai/`, and
  `/home/inkble/MAMMwork/results/statistical_methods.md`: operational and
  statistical documentation.

## Validation performed on 2026-08-07

Build command:

```bash
cd /home/inkble/MAMMwork
g++ -std=c++17 -O2 -Wall -Wextra -pedantic PathSearchFix.cpp -o path_benchmark
python3 -m py_compile tools/analyze_benchmarks.py
```

The executable was created, `./path_benchmark --help` returned zero, and the
Python bytecode check succeeded.

Executed smoke tests (all used master seed `20260807`, two measured repetitions,
and one recorded warm-up repetition):

```bash
./path_benchmark --mode random-static --target-solvable 1 --max-candidates 50 \
  --timing-repetitions 2 --warmup-runs 1 --output-dir results/smoke_static
./path_benchmark --mode random-dynamic --target-solvable 1 --max-candidates 50 \
  --timing-repetitions 2 --warmup-runs 1 --dynamic-update-mode both \
  --output-dir results/smoke_dynamic
./path_benchmark --mode movingai-static --pairs-per-map 1 --min-start-goal-distance 1 \
  --timing-repetitions 2 --warmup-runs 1 --output-dir results/smoke_movingai_static
python3 tools/analyze_benchmarks.py --input-dir results/smoke_static
python3 tools/analyze_benchmarks.py --input-dir results/smoke_dynamic
python3 tools/analyze_benchmarks.py --input-dir results/smoke_movingai_static
```

Audited outcomes:

| Run | Raw records | Warm-up records | Correct cost groups | Analysis output |
|---|---:|---:|---:|---|
| random static | 81 | 27 | 18 / 18 | 27 static summary rows, paired rows |
| random dynamic | 324 | 108 | 36 / 36 final-update groups | 54 dynamic summary rows, paired rows |
| Moving AI static | 18 | 6 | 4 / 4 | 6 static summary rows, paired rows |

The dynamic manifest had 18 rows and contained both `local_nonpath` and
`path_blocking`. The runner stores rejected events rather than silently removing
them; no rejection occurred in this small smoke configuration.

## Limitations

Smoke tests validate the pipeline, not a publication-scale performance claim.
The defaults are intentionally much larger and must be run on the target
hardware under a documented environment. Moving AI input remains local-only;
the example manifest references only maps already present in this repository.
No paper table or numerical result has been updated from smoke-test data.
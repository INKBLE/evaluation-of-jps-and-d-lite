# Path-planning benchmark pipeline

Build from `/home/inkble/MAMMwork` (there is no build system):

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic PathSearchFix.cpp -o path_benchmark
```

Small static smoke test and analysis:

```bash
./path_benchmark --mode random-static --target-solvable 3 --max-candidates 200 --timing-repetitions 2 --warmup-runs 1 --output-dir results/smoke_static
python3 tools/analyze_benchmarks.py --input-dir results/smoke_static
```

All planners use 8-neighbor motion, cost 10/14, and the existing rule allowing
a diagonal when at least one adjacent cardinal cell is free. Random instances
are derived reproducibly from `--master-seed`; every candidate is retained in
the manifest. Warm-ups are written with `warmup=1` and excluded by analysis.

Dynamic `local_nonpath` chooses outside the union of the three original paths;
`path_blocking` chooses a common internal path cell outside the endpoint margin.
The update is identical for all planners. A*/JPS restart; D* Lite times only
`updateCell + replan` after unmeasured initialization. Rejected/unreachable
updates remain in `dynamic_manifest.csv` and incomplete targets return nonzero.

Moving AI maps are local-only and configured in `data/movingai/manifest.csv`.
Do not use this pipeline to claim numerical results until actual raw CSV output
has been produced and reviewed.
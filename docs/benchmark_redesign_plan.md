# Submission-Quality Benchmark Redesign Plan

## Scope and source files

The current C++17 entry point is `/home/inkble/MAMMwork/PathSearchFix.cpp`.
It contains the grid model, A*, JPS, D* Lite, random-map generation, the
existing dynamic update logic, CSV writers, and `main`.  It will be refactored
into a configurable experiment runner while preserving the implementations and
movement semantics of A*, JPS, and D* Lite.  Algorithm code will only be
changed if a validation failure demonstrates a bug that prevents a fair
comparison; such a change will be documented explicitly.

Files to be created or changed:

* `PathSearchFix.cpp`: CLI parsing, reproducible instance generation, Moving
  AI parsing, planner adapters, validation, manifests, raw-record writers, and
  C++ summary generation.
* `tools/analyze_benchmarks.py`: standard-library-only two-stage aggregation,
  paired comparisons, and deterministic bootstrap confidence intervals.
* `README_BENCHMARKS.md`: build/run instructions, metric definitions, and
  reproducibility policy.
* `data/movingai/manifest.csv`: editable example dataset manifest; it contains
  local paths only and never downloads maps.
* `data/movingai/README.md`: local Moving AI map placement and terrain-policy
  documentation.
* `results/statistical_methods.md`: statistical definitions and exclusion
  policy.
* `docs/benchmark_redesign_implementation_report.md`: final implementation
  report after build and smoke-test validation.

Legacy CSV files are left untouched.  New runs write the stable schema below
to the selected `--output-dir` (default `results/benchmark_redesign`).

## Common movement model and planner timing boundary

All planners receive copies of the same occupancy grid, identical start and
goal coordinates, and the same update cell in dynamic experiments.  The
existing model is retained: 8-connected movement, cardinal cost 10, diagonal
cost 14, and a diagonal is forbidden only if *both* adjacent cardinal cells
are blocked (`allow_single_corner_contact`).  Costs are integers and therefore
must agree exactly.

Timed intervals use `std::chrono::steady_clock` and contain only the planner
operation:

* static A*/JPS: solver construction plus `solve`/search, because each is a
  from-scratch planner;
* static D* Lite: D* Lite construction plus initial `solve`, because that is
  its required initial-planning operation;
* dynamic initial phase: the same initial operation as static;
* dynamic restart phase for A*/JPS: construction on the already-created
  updated grid plus search;
* dynamic repair phase for D* Lite: `updateCell` plus `replan`, including its
  incremental state repair and path extraction.

Grid generation/copying performed solely for test setup, map parsing,
manifest/CSV I/O, validation, and console output are outside timed intervals.
Warm-up runs are written with `warmup=1` and excluded from measurements.

## Reproducible random-map instances

Random static configurations are 50x50, 100x100, and 500x500 at densities
0.10, 0.20, and 0.30.  The default target is 100 valid solvable maps per
configuration, configurable with `--target-solvable`; `--max-candidates`
limits attempts.  Fixed corner endpoints `(0,0)` and `(width-1,height-1)` are
forced free by the existing generator and are documented as the random-map
endpoint policy.

For each configuration, sequential candidate indexes are deterministically
mixed with `--master-seed`, width, height, and density to obtain unique map
seeds.  Every candidate—solvable or not—is written to
`random_map_manifest.csv`.  Its seed, dimensions, density, endpoints, and
stable FNV-1a 64-bit grid hash exactly reconstruct and identify the map; map
files need not be duplicated.  A* is used only as an untimed feasibility
oracle before inclusion, not as a performance result.  Generation stops at the
target or maximum.  A missed target is written as an incomplete result and
causes a nonzero runner status after outputs are flushed.

All three planners run every included static map with the same repetitions
(default 20) and warmups (default 3).  Deterministic fields are captured from
the first non-warm-up run and cross-repetition changes are recorded as a
correctness/exclusion reason rather than treated as independent maps.

## Correctness handling

Each completed planner result is checked for: reported success, a nonempty
path with the configured endpoints, every cardinal/diagonal step legality,
the configured diagonal-collision rule, and exact equality between the
reported and independently recomputed integer path cost.  For a solvable
group, each planner must succeed with a valid path and equal cost.  Raw rows
carry `correctness_group`, `path_cost_matches_group`, and `exclusion_reason`.
Failures and disagreements remain in raw files.  Only fully correct groups
are eligible for performance aggregates; this is conditional-on-solvability
and conditional-on-correctness, not a claim about all generated maps.

## Dynamic instances and event fairness

Dynamic tests use the selected static solvable maps and have two explicit
modes, selected with `--dynamic-update-mode local_nonpath|path_blocking|both`.
For every update attempt, all three planners first run on the unmodified map;
the initial results are validated before an event can be accepted.

The event selection is algorithm-neutral and deterministic from the map seed,
mode, and attempt index:

* `local_nonpath`: candidates are free, non-endpoint cells outside the union
  of A*, JPS, and D* Lite initial path cells.  A deterministic shuffled order
  selects one.  Thus the event is not on any planner's selected original path.
* `path_blocking`: candidates are interior cells in the intersection of all
  three initial path cell sets.  A configurable prefix/suffix is excluded from
  each path.  The first deterministically ordered candidate is used.  Thus the
  identical update blocks every planner's selected initial path rather than
  favoring one planner.

The selected cell, path-membership flag, solvability after the update, every
rejection (no candidate, failed initial correctness, or updated map
unreachable), and inclusion status are saved in `dynamic_manifest.csv`.
For a valid dynamic timing instance, A* and JPS restart from the updated grid;
D* Lite is initialized on the original grid and receives exactly one
`updateCell(cell, true)` followed by `replan`.  Initial and repair/restart
phases are recorded separately.  `path_blocking` accepts only updates that
remain solvable; the same conditional policy is applied to `local_nonpath` for
comparable repair-time results, while all unreachable updates remain visible in
the manifest and are summarized as an unreachable fraction.

The default target is 100 valid dynamic instances per configuration and mode.
If a static set cannot provide that many accepted events, the run is explicitly
incomplete rather than silently dropping attempts.

## Moving AI Lab integration

The runner parses standard local `type octile` files with exactly:

```
type octile
height <H>
width <W>
map
```

Terrain policy: `.` and `G` are free; `@`, `O`, `T`, `S`, and `W` are blocked;
all other terrain symbols cause a descriptive parse error.  Local repository
maps currently include `/home/inkble/MAMMwork/room-map/*.map` and
`/home/inkble/MAMMwork/maze-map/*.map`; a supplied manifest will refer to them
without downloading any data.  Output records map family, manifest identifier,
filename/source, dimensions, and FNV-1a hash.

The manifest format supports `map_family,map_path,map_id,pairs_per_map,notes`.
For each selected map, deterministic free-cell pair sampling uses the master
seed and map identity, enforces `--min-start-goal-distance`, checks
solvability, records unsuccessful samples, and retains the requested number of
valid pairs (default 100) or reports incomplete.  Per-map results precede
family-level aggregates; map identifiers are never discarded by pooling.

## Stable output schema

`random_map_manifest.csv` columns:

```
experiment_id,map_family,map_source,grid_width,grid_height,obstacle_density,
master_seed,map_seed,candidate_index,map_id,map_hash,start_x,start_y,goal_x,
goal_y,initially_solvable,included_in_static_set,exclusion_reason
```

`static_raw.csv` has one map/planner/timing-repetition row:

```
experiment_id,benchmark_type,map_family,map_id,map_hash,grid_width,grid_height,
obstacle_density,start_x,start_y,goal_x,goal_y,planner,repetition,warmup,
success,path_cost,expanded_nodes,elapsed_us,correctness_group,
path_cost_matches_group,exclusion_reason
```

`dynamic_manifest.csv` columns:

```
experiment_id,map_family,map_id,map_hash,update_mode,update_x,update_y,
update_on_original_path,updated_map_solvable,included_in_dynamic_set,
exclusion_reason
```

`dynamic_raw.csv` columns:

```
experiment_id,map_family,map_id,map_hash,update_mode,planner,phase,repetition,
success,path_cost,expanded_nodes,elapsed_us,path_cost_matches_group,
exclusion_reason
```

`static_summary.csv` and `dynamic_summary.csv` contain each requested field:

```
experiment_id,benchmark_type,map_family,grid_width,grid_height,
obstacle_density,update_mode,planner,independent_map_count,candidate_map_count,
solvable_map_count,solvability_rate,timing_repetitions,
timing_statistic_per_map,median_elapsed_us,mean_elapsed_us,stddev_elapsed_us,
q1_elapsed_us,q3_elapsed_us,p95_elapsed_us,median_expanded_nodes,
mean_expanded_nodes,success_rate,notes
```

Fields not applicable to an input family or static experiment are empty, not
invented.  CSV string fields are correctly quoted.

## Analysis and documentation

`tools/analyze_benchmarks.py` uses Python 3 standard library only.  It first
computes each map/planner/phase median from non-warm-up timing rows, then
computes configuration statistics across maps.  It pairs JPS-vs-A* and D* Lite
vs-A* by `(map_id)` for static and `(map_id, update_mode, update coordinates)`
for dynamic data.  It reports per-instance baseline/comparison speedups,
median/mean/Q1/Q3 speedup, and a percentile bootstrap 95% CI using a documented
fixed seed and resample count.  No significance claim or unimplemented test is
reported.

`results/statistical_methods.md` documents the independent unit (map instance,
not repetition), timing aggregation, conditional interpretation, bootstrap
method, exclusions, and failures.  `README_BENCHMARKS.md` includes build and
example commands, map installation policy, and the prohibition on updating
paper numbers until real raw CSV files exist.

## Quality gates

After implementation, compile with the existing direct C++17 command (there
is currently no build system), run a small random smoke test only after a
successful build, inspect headers and row relationships, run the analysis
script, and verify static/dynamic cost agreement and manifest retention of
failed/rejected instances.  The final implementation report lists changed
files, exact commands, outcomes, limitations, and locally required Moving AI
maps.  No LaTeX paper file or numerical claim will be modified.
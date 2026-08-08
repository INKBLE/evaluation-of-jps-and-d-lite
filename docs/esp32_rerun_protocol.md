# ESP32 controlled rerun protocol

## Audit of the pre-rerun firmware

The audited source was `/home/inkble/MAMMwork/esp_benchmark/main/path_benchmark.cpp`
before the controlled-rerun changes.

* `app_main()` registered `heap_caps_register_failed_alloc_callback()` and then
  ran all six cases in one process, in this exact order: `initial` 50x50 d10,
  `initial` 100x100 d20, `initial` 500x500 d20, `replanning` 50x50 d10,
  `replanning` 100x100 d20, and `replanning` 500x500 d30.  It printed
  `benchmark_complete` afterwards.  It did not call `esp_restart()`.
* In each case the loop order was **instance, AStar, JPS, DStarLite**, with one
  non-emitted warm-up and three emitted repetitions for each planner.  Thus all
  cases and all planners shared the same boot and allocator history.
* `Grid` was a local object in `runOne()`, holding `vector<int> cells`; it and
  the solver temporary were destroyed on return from `runOne()`.  Solver
  containers were consequently destroyed after every run, subject to normal
  allocator caching/fragmentation.  No explicit `shrink_to_fit()` was used.
  A D* Lite solver was local to `runDStarLite()` and retained state from its
  initial solve through `updateCell(...); replan()` within that one run only.
* The old timer started immediately before the solve lambda and stopped on its
  return.  Grid construction and normal solver construction were outside the
  interval.  In the dynamic D* Lite path, solver construction was before the
  initial interval; the repair interval included `updateCell` and `replan`.
  UART CSV logging was after the interval.  The old runner did include path
  result construction/destruction performed by the solve call, but did not
  separately document that policy.
* The ESP timer was `esp_timer_get_time()`.  Free heap was sampled before and
  after the solve; `heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)` was
  emitted as `heap_minimum`.  That API is a **since-boot global low-water
  mark**, not a per-sample peak-memory measurement.  The old continuous run
  therefore cannot assign it to an independent case.
* Allocation failure was captured with a heap failed-allocation callback that
  stored requested bytes/capabilities without logging, and `std::bad_alloc`
  was caught around `runOne()`.  The old record did not reliably identify the
  allocation stage and used `initial_or_replan` for dynamic failures.
* The audited `sdkconfig` targets ESP32, enables C++ exceptions and a 1024-byte
  emergency exception pool, disables PSRAM and Bluetooth, sets CPU frequency to
  160 MHz, and leaves `CONFIG_FREERTOS_UNICORE` unset (dual-core capable).  No
  application Wi-Fi initialization is present.  ESP-IDF is v5.2.2 and the
  existing build uses xtensa-esp-elf GCC 13.2.0.

## Controlled invocation and isolation policy

Each flash/reboot executes one generated profile: one planner and one scenario.
The firmware never advances to another planner or scenario and always ends with
the exact standalone marker `planner_scenario_complete`, including after a
captured allocation failure.  The host tool reflashes (which resets the MCU)
for every profile.  `instance_count=1` is the strict-isolation equivalent for
the D* Lite diagnostic; a fresh flash/reboot is used for each such invocation.

The selected profile is generated in
`/home/inkble/MAMMwork/esp_benchmark/main/benchmark_profile.h`.  It contains
planner, scenario, instance range, warm-up count, formal repetitions and an
experiment ID.  Its values are printed at boot and copied into every CSV row.

Within a profile, each instance is contained in a scope so Grid and all planner
objects are destroyed before the next instance.  Explicit vector clearing and
swapping releases result-path capacity before leaving the scope.  This does not
make the allocator's since-boot low-water mark per-instance; it only avoids
live algorithm objects crossing instance boundaries.

## Maps, dynamic events, and correctness

The prior ESP runner used deterministic seeds
`1000 + width*17 + height*31 + density_percent*101 + instance`.  The repository
also contains a newer desktop random-map manifest, but it has different 64-bit
seeds and map IDs.  It therefore cannot truthfully be described as the old ESP
map set.  The ESP rerun preserves the previous ESP seed formula, writes its
exact map ID and seed in every row, and documents it as a separate deterministic
ESP manifest.

For replanning, a deterministic reference A* initial path selects the first
middle-out interior cell whose blockage remains solvable.  The selected
coordinate is emitted as `update_x,update_y` and is identical for all planners
for a given map.  A*/JPS search the updated Grid from scratch; D* Lite retains
the solver constructed on the original Grid and executes one `updateCell` plus
`replan`.  A missing initial path or eligible update emits `SKIP` with
`NO_DYNAMIC_EVENT`; it is never counted as success or OOM.  This is a newly
documented deterministic ESP event manifest rule, not a claim that it equals a
different desktop-manifest event.

Movement remains 8-connected with cardinal cost 10 and diagonal cost 14.  The
existing diagonal collision rule is retained: a diagonal is disallowed only
when both orthogonal side cells are blocked.  Successful formal rows carry map
identity, found state, cost and expanded-node count.  Host-side postprocessing
checks duplicate sample keys and, when all three planners are available, checks
final path-cost agreement by map/repetition/phase.

## Measurement policy and limitations

Formal samples record free heap before and after the timed call and
`since_boot_minimum_free_heap_before/after`.  The latter is explicitly global
since boot.  No allocation tracker or allocator high-water mark is added,
because the STL allocations inside all existing solvers are not routed through
a common allocator; free-heap deltas and global low-water mark are only
implementation-level observations.

The timed interval excludes Grid construction, map generation, deterministic
event selection, planner construction/internal-array initialization, UART
logging/CSV serialization, and object destruction.  It includes the algorithm
call itself: initial search/path reconstruction; for dynamic A*/JPS the updated
map search/path reconstruction; and for D* Lite `updateCell` plus repair/path
reconstruction.  This policy is consistent across planners for each respective
phase, while deliberately measuring D* Lite repair rather than rebuilding it.

Captured `bad_alloc` records use `elapsed_us=-1` and `expanded_nodes=-1`, retain
the requested allocation bytes, and label the best-known stage.  An allocation
failure is not evidence of an intrinsic algorithmic memory bound.
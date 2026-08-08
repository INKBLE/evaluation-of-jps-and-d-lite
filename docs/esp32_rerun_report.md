# ESP32 controlled rerun report

## Run identity

This report is derived only from the captured full-matrix output in
`/home/inkble/MAMMwork/results/esp32_rerun/full_matrix`.

* Run ID: `esp32_20260808T132635Z`
* Device: ESP32-D0WD-V3 revision 3.1, MAC `8c:4f:00:3c:4b:d4`
* ESP-IDF: v5.2.2; CPU: 160 MHz; PSRAM: disabled
* Profiles: 18 (`AStar`, `JPS`, `DStarLite` x six scenarios), each isolated by
  build, flash, and hardware reset.
* Profile configuration: 10 deterministic ESP maps, one unreported warm-up and
  three reported repetitions per map. Every one of the 18 manifests recorded
  `completion_marker_seen=1`.

The raw samples, failure records, manifests, and aggregate phase summaries are
available in [full_matrix](/home/inkble/MAMMwork/results/esp32_rerun/full_matrix).

## Data integrity

The aggregate raw CSV contains 720 rows with no duplicate sample keys. Of these,
540 are formal rows and 180 are non-emitted warm-up rows:

* 360 successful measurements from 50x50 and 100x100 scenarios.
* 180 structured, safe OOM rows from 500x500 scenarios.
* No other failure status, no completion timeout, and no `Guru Meditation`,
  `abort`, or task-watchdog output in any captured raw serial log.
* All 120 comparable successful map/repetition/phase groups had identical final
  path cost across the three planners. D* Lite uses `repair` as its post-update
  phase; A* and JPS use `replan`.

The original dynamic D* Lite batch diagnostic initially exposed task-watchdog
messages because CPU0 did not yield between samples. Firmware now calls
`vTaskDelay(1)` only after each complete sample, outside the timed interval,
and the host treats task-watchdog output as fatal. The corrected batch had 60
successful rows (30 initial/repair pairs); ten subsequent single-instance
fresh-flash diagnostics also completed without fatal output.

## Formal timing results

Values below are median microseconds across the 30 successful formal samples in
each planner/scenario/phase. They are copied from
`esp32_summary.csv`; lower values are faster within a row.

| Scenario and phase | AStar | JPS | DStarLite |
| --- | ---: | ---: | ---: |
| Initial 50x50 d10 | 36,253.5 | 26,148.0 | 207,161.0 |
| Initial 100x100 d20 | 187,265.5 | 147,122.5 | 916,859.0 |
| Dynamic 50x50 d10 initial | 36,161.5 | 25,892.5 | 207,134.0 |
| Dynamic 50x50 d10 replan/repair | 36,085.5 | 24,981.0 | 7,226.5 |
| Dynamic 100x100 d20 initial | 187,177.0 | 144,807.0 | 916,800.5 |
| Dynamic 100x100 d20 replan/repair | 208,927.5 | 151,942.0 | 544,644.0 |

D* Lite initial solves are substantially slower in this implementation, while
its 50x50 repair median is much lower than from-scratch replanning. The 50x50
D* Lite repair distribution has a high p95 (281,474 us), so its median alone
does not describe all repair behavior. At 100x100 the D* Lite repair median is
544,644 us, compared with 208,927.5 us for A* replan and 151,942 us for JPS
replan.

## 500x500 memory boundary

All three planners, in both initial 500x500 d20 and dynamic 500x500 d30
scenarios, failed safely before search during `grid_allocation`. Each of the
180 rows records `OOM`, `bad_alloc_heap_callback`, and a failed allocation of
1,000,000 bytes. There are no elapsed-time or expanded-node values for these
rows because no planner invocation occurred. This establishes a configuration
boundary for this 160 MHz ESP32 build without PSRAM; it is not a comparative
algorithm memory limit.

## Memory metric limitation

`since_boot_minimum_free_heap` in the summary is a boot-global allocator
low-water mark. It must not be read as per-sample peak memory or used for a
cross-profile memory ranking. The raw CSV also records free heap immediately
before and after each timed operation as implementation observations only.

## Reproduction

Source the ESP-IDF environment and run:

```bash
. /home/inkble/esp/esp-idf/export.sh
python3 /home/inkble/MAMMwork/tools/run_esp32_matrix.py \
  --device /dev/ttyACM0 \
  --project /home/inkble/MAMMwork/esp_benchmark \
  --instances 10 --warmups 1 --repetitions 3 --timeout 240 \
  --output /home/inkble/MAMMwork/results/esp32_rerun/full_matrix
```

The runner persists raw rows, manifest, failure log, and summary even when a
later profile fails; failure is still returned to the caller. The deterministic
ESP map seeds and dynamic-event rule are defined in
[esp32_rerun_protocol.md](/home/inkble/MAMMwork/docs/esp32_rerun_protocol.md).
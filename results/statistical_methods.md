# Statistical methods

The independent unit is a map instance (or map/start-goal pair), not a timing
repetition. Analysis first takes each map/planner/phase median over non-warm-up
runs, then summarizes those per-map medians. Only successful rows with matching
validated group costs are aggregated; rejected maps and updates remain auditable
in their manifests.

Paired speedup is `AStar_median_time / compared_planner_median_time` for the
same map and, dynamically, update mode/event. Dynamic A*/JPS `replan` and D*
Lite `repair` are normalized to the shared `updated_solution` comparison phase.
The script reports descriptive
quantiles and a percentile-bootstrap 95% CI for median speedup. Defaults are a
fixed seed of 20260807 and 10,000 resamples. It makes no hypothesis-test claim.
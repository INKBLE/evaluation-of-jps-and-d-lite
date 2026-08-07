# Local Moving AI map input

The runner reads local paths from `manifest.csv` and never downloads maps.
It accepts standard `type octile` map headers. `.` and `G` are traversable;
`@`, `O`, `T`, `S`, and `W` are blocked. Any other terrain symbol is rejected.

Append approved local maps as `map_family,map_path,map_id,pairs_per_map,notes`.
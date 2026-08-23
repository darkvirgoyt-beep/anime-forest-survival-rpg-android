# AETHELGRAD 50-room co-op stress report

## Scenario

The harness started **50 concurrent co-op rooms** with **4 players per room**, for a total of **200 simulated players**. It exercised room creation, three friend joins per room, four heartbeats per room, one combat action per room, one gather action per room, and one craft action per room. The service ran locally over HTTP using the same route implementation as production, with a 1 ms synthetic delay on selected in-memory SQL operations.

> This is an in-memory SQL-pattern simulation. It measures route concurrency, JSON handling, transaction sequencing, and service behavior. It is **not** a PostgreSQL capacity benchmark.

## Results

| Metric | Result |
| --- | ---: |
| Total requests | 550 |
| Successful requests | 550 |
| Unexpected failures | 0 |
| Aggregate minimum latency | 16.193 ms |
| Aggregate median latency | 52.940 ms |
| Aggregate p95 latency | 100.560 ms |
| Aggregate maximum latency | 117.031 ms |
| Aggregate elapsed phase time | 353.263 ms |
| Simulated throughput | 1,556.91 requests/second |
| SQL-pattern operations | 5,400 |
| Simulated transactions | 150 |
| Rooms created | 50 |
| Active members | 200 |

## Phase breakdown

| Phase | Requests | Status | p50 | p95 | Max | Elapsed |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| Create rooms | 50 | 50 × 201 | 59.265 ms | 76.838 ms | 78.914 ms | 78.832 ms |
| Join members | 150 | 150 × 200 | 75.680 ms | 111.639 ms | 117.031 ms | 116.997 ms |
| Heartbeats | 200 | 200 × 200 | 57.594 ms | 84.534 ms | 90.012 ms | 89.911 ms |
| Combat | 50 | 50 × 200 | 24.349 ms | 24.466 ms | 25.015 ms | 24.985 ms |
| Gather | 50 | 50 × 200 | 16.802 ms | 22.380 ms | 22.418 ms | 22.405 ms |
| Craft | 50 | 50 × 200 | 18.823 ms | 19.454 ms | 20.144 ms | 20.133 ms |

## Validation coverage

Every room completed the full path: creation, three joins, four presence updates, a server-approved attack, a server-calculated resource reward, and server-validated crafting. Expected status codes were returned for all 550 requests: `201` for room creation and `200` for every subsequent operation.

## Interpretation and limitations

The route implementation remained stable under 50-room concurrency in the local simulation, and no unexpected status or transaction-path failure occurred. Join and heartbeat phases carry the largest request fan-out, so they are the most useful candidates for a future real transport and database benchmark.

The measured numbers must not be used as a PostgreSQL sizing claim. The harness uses a memory-backed SQL-pattern pool and a synthetic 1 ms delay. Before production capacity planning, repeat the scenario against a disposable PostgreSQL 16 instance using the exported schema, a documented connection-pool size, realistic network latency, and a real concurrent transaction isolation policy. Add tests for lock waits, deadlocks, connection exhaustion, reconnects, and sustained multi-minute load.

## Reproduction

```bash
cd server
node test/coop_stress_50_rooms.mjs
```

The machine-readable output is stored in `docs/aethelgard_50_room_stress_result.json`.

## References

[1]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/test/coop_stress_50_rooms.mjs "AETHELGRAD 50-room stress harness"

[2]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/src/server.mjs "AETHELGRAD online-service route implementation"

[3]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/sql/004_authoritative_gameplay.sql "AETHELGRAD authoritative gameplay schema"

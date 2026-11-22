# G1 R3-0: tolerance of the cost model against luna, written before the comparison (2026-11-15)

Committed alone (`G1_r3_design.md`, R2.7). Hypothesis, no source: the figures below are a first guess of what a search can
live with, not a measured need. They are not adjusted after seeing the comparison; a change is a new file with a new date.

The comparison: the 31 rows of R0, priced with the cost model of section 3 of the design note, against the master cycles
luna measures on the same rows, in SlowROM and FastROM.

Pass, for each speed:
1. Per row: the model within **3 %** of luna.
2. Mean absolute error over the rows under **1 %**.
3. Rank: the order of each row's variants (switch on / off) is the same in the model and in luna.

A row outside the tolerance: the model is corrected (the cause named, with the instruction or access it mispriced), the
comparison re-run on all rows. The search (R3-2) starts only after a pass at both speeds.

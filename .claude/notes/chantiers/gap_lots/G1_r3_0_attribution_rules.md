# G1 R3-0: attribution of the 0.32 of `path`, outcomes written before any measure (2026-11-15)

Decision of the conductor (thread `2026-11-15-g1-r3-quelle-ligne…`, message 005). Committed alone, before any attribution figure.

**Object**: the gap between the paper of `path` (`g1_r3_0/path_paper.asm`, 1.316 SlowROM / 1.319 FastROM of the hand) and the
hand (`path_hand.asm`): 25 800 master cycles per call SlowROM. The paper stays on file unchanged.

**Method**: at the frozen commit, post by post (32-bit composition and sign extension, mask M, accumulators in slots, loop
counter and index, table read, the rest), each cost in master cycles per call, summing to the measured gap; a post is
classified IR idiom (a rewrite of the IR that would also feed today's generator) or allocation itself (what a better choice of
register/slot/place would change). A post not attributable is its own line, not folded.

**Outcomes**:
- (a) more than half of the gap is IR idioms: a second paper of `path` is written, as a new dated file with revised rules frozen
  before its text, applied by another agent; the current paper stays.
- (b) the gap is mostly the allocation itself: the bet of chains in A is lost on leaf A; G1 stops at R3-0 and the conductor takes
  the report to the owner, with what goes to the current generator.
Limit: one day at most. R2.7 and the request to the games continue.

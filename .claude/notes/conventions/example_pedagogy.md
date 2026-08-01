# Example bundle — pedagogy charter

The example bundle is the SDK's front door and its teacher. This charter is
the **pedagogical** north star for every example we write or move; the
**mechanical** checklist (Doxygen tags, screenshot naming, init order, the
10 anchor systems a move touches) lives in `.claude/rules/new_example.md`.
Read that for *how to ship* an example; read this for *how to teach* with one.

Governs the greenfield reorg in
`.claude/notes/chantiers/examples_reorg_by_usecase.md`. The Text wave
(`print_string` / `scroll_message` / `fundamentals/text_glyphs`) is the
reference implementation — its quality is the floor, this charter is the
ceiling.

## The spine: the bundle teaches a *developer's journey*, not a feature list

We do not organise learning by PPU block. A learner wakes up with an
**internal question**, and it changes at each stage of building a game. The
curriculum's narrative spine is those seven stages:

| Stage | The learner's question | Families | The confidence it buys |
|-------|------------------------|----------|------------------------|
| 0 | "Does my setup even work?" | Getting started | the toolchain isn't scary |
| 1 | "Can I put something on screen?" | Text, Backgrounds, Sprites | I control the picture |
| 2 | "Can the player act?" | Input | it's a game, not a demo |
| 3 | "Can I build a world?" | Scrolling & maps | bigger than one screen |
| 4 | "Can I make it feel good?" | Colour & effects, Mode 7, Audio | it feels like a real game |
| 5 | "Can I hold it together?" | Framework, Game math | architecture, not spaghetti |
| 6 | "Can I finish and ship?" | Data & memory, Enhancement chips, Games | a complete cartridge |

**"Comfortable at each stage" is the whole goal**, and it has a precise
definition: at every step the learner (a) never hits a wall they can't climb
with what they *just* learned, (b) always sees a result that works, and (c)
always understands *why*. Design every example to protect those three.

## Core principles (instructional design, applied to the SNES)

1. **Time-to-first-success decides everything.** The first fifteen minutes —
   a colour on screen, then a word — determine whether they continue.
   Optimise the working result *before* the theory. Never open a family with
   its hardest rung.
2. **One new idea per rung.** Each example adds exactly one concept onto a
   mastered base (zone of proximal development). The jump must be climbable,
   never a cliff. If a rung needs two new ideas, it's two rungs.
3. **Concrete before abstract.** It works first; you explain second.
   Motivation precedes theory, never the reverse.
4. **Scaffolding that fades — consumer → author.** Early rungs give
   everything (`textModeInit()` does eight things in one call: "call it, it
   works"). Middle rungs remove scaffolding ("here's what it did — now do
   part yourself"). The `fundamentals/` tier is the deliberate reveal:
   *what the module hid*, shown when the learner is ready for it, not before.
   Late rungs fade to a challenge ("you know enough — go").
5. **End every topic on juice.** The last rung of a family is a showcase that
   makes them want the next family. Intrinsic motivation is the fuel of a
   step-by-step path.
6. **Debugging confidence is part of comfort.** The SNES fails *silently*
   (black screen, garbage tiles, VRAM writes dropped outside VBlank). A
   beginner facing an unexplained black screen quits. So every example
   defuses the silent traps up front — see the mandatory anatomy below.
7. **Always answer "why does this matter for *my* game?"** Every example
   whispers how you'd use it in a real game. That sentence is the difference
   between a hardware manual and a mentor.
8. **Identical structure everywhere.** Same sections, same order, every time.
   The learner's eye knows where to look; extraneous cognitive load drops.
9. **Games are capstones that force transfer.** They recombine earlier
   rungs, and they link back ("this game uses rung X") so the learner sees
   the payoff of what they learned.
10. **Permission to branch.** Not everyone builds an RPG. Use-case navigation
    lets a learner head straight for their goal (a shmup, a puzzle) without
    walking the whole tree.

## Mandatory example anatomy (the pedagogical layer)

Beyond being *correct*, every example README must carry, in this order:

1. **Title + rung** — `Family N · rung X.Y (short idea)`.
2. **Why it matters for your game** — one or two sentences connecting the
   lesson to a real game need. This opens the README, before the mechanics.
3. **Screenshot** — the visible result (or omit if there's genuinely no
   visual, e.g. audio).
4. **What you'll learn** — the *single* new idea, plus the concepts it rests
   on.
5. **What to observe / if it breaks** — what a correct run looks like, and
   the one or two silent-failure symptoms for *this* example with their
   cause ("solid colour, no text → the print never flushed"; "garbage tiles →
   VRAM written outside forced blank"). This is the debugging-confidence
   contract.
6. **Build & run** — the standard command, luna (not Mesen2) as the runner.
7. **Ladder links** — `← previous rung · → next rung`, and the
   `fundamentals/` cross-link where one exists. The learner must always know
   where they are and where they're going.

The `main.c` `@file` block mirrors this: `@brief` names the family+rung,
`@par What to Observe` states the correct result, and the prose explains
*why*, not just *what* — comments teach, they don't narrate the obvious.

## The scaffolding curve (calibrate the voice to the stage)

- **Stages 0–1 (worked-example mode):** full hand-holding, one-call helpers,
  every line explained. The learner is a *consumer* of a working recipe.
- **Stages 2–4 (assemble mode):** the helper is opened up; the learner wires
  a few pieces themselves; "Go Further" prompts invite small tweaks.
- **Stages 5–6 (author mode):** minimal scaffolding; the example is a
  starting point and a challenge, not a recipe. Cross-links to the modules'
  API docs replace step-by-step prose.

Match the README's density and tone to the stage. A stage-0 README over-
explains on purpose; a stage-6 README that over-explains is condescending.

## Anti-patterns (do NOT)

- Two examples that answer the *same* developer question (visually different
  ≠ pedagogically different — but *same question, same answer* = a duplicate;
  merge or drop it).
- A rung that introduces two new concepts at once (split it).
- Opening a family with its showcase (that's the reward, not the on-ramp).
- A README that explains *what* the code does line-by-line without ever
  saying *why* the learner should care or *how* they'd use it.
- Silent omission of the failure modes ("it just works" hides the exact
  moment the learner will get stuck).
- Numbered directory prefixes (`01_`) — they fossilise the ladder into paths
  and break on insertion; encode order in the family README, not the folder
  name.

## Relationship to other docs

- `.claude/rules/new_example.md` — the mechanical shipping checklist (init
  order, Doxygen tags, screenshot naming, testing). This charter sits *above*
  it: correctness is necessary, pedagogy is the point.
- `docs/API_INDEX.md` — the task vocabulary. Family and rung names bind to
  its "I want to…" column so the tree and the index reinforce one learning
  language.
- `docs/LEARNING_PATH.md` — the narrative spine, told as the seven-stage
  developer journey above (not a PPU tour).

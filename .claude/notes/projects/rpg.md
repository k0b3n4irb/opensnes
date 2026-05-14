---
name: RPG project — asset references and style choices
description: User's RPG project context. ClockworkRaven for menus/inventory pixel-art, HUD overlay-direct (Zelda ALttP standard). Used when working on projects/rpg/ specifically — not the SDK itself.
type: project
originSessionId: 7cf8aa6f-20b3-4e2f-81b4-25748e91b08f
---
The user's in-progress RPG project lives under `projects/rpg/` (when
present). Asset and style preferences:

- **ClockworkRaven** — pixel-art menus, GUI, HUD, UI kits.
  https://clockworkraven.itch.io/. Inventory kit:
  https://clockworkraven.itch.io/raven-fantasy-pixelart-menu-gui-hud-and-ui-kit-starter-set
  The user has explicitly preferred this style for menus / inventory in
  the RPG project. Apply when designing those UI surfaces.
- **HUD style — overlay-direct** (Zelda: A Link to the Past standard,
  not a separate panel below the play area). Apply this when designing
  hearts / magic / item HUD elements for the RPG.

## How to apply

- When working on RPG menus / inventory: prefer ClockworkRaven-style
  pixel-art conventions (border treatment, item slot grids, font
  weight) over generic placeholder UI.
- When sizing HUD elements: design for overlay-on-play-area, not for a
  bottom strip — the play area takes the full 256×224 frame.
- When deciding asset sources: respect MIT-compatible licensing and
  track new assets in `ATTRIBUTION.md`.

## When this rule does NOT apply

- SDK-internal work (compiler, lib, examples). Those don't touch the
  RPG project.
- Other user projects under `projects/` if any (none documented today).

## Cross-reference

- `memory/feedback_mesen2_redundancy.md` — `projects/rpg/` changes do
  NOT require Mesen2 validation of the 7 SDK reference examples; only
  the project being modified needs validation.

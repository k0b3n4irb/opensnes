# Input Examples

The SNES controller has 12 buttons: A, B, X, Y, L, R, Start, Select, and a D-pad
(Up, Down, Left, Right). The console reads them automatically every frame via
"auto-joypad read" and stores the result in hardware registers.

The key skill is **edge detection** — distinguishing "button is held" from "button was
just pressed this frame." Without it, a single tap of A fires 10+ times at 60fps.

| Example | What you'll learn |
|---------|------------------|
| [Button Test](button_test/) | Reading all buttons, edge detection pattern |
| [Two Players](two_players/) | Reading both controllers independently |

# Giuseppe RPG Engine — Documentation technique

> **Ce document décrit le moteur tel qu'il fonctionne aujourd'hui.**
> Pour la vision long terme (ring menu, inventaire, quêtes, sauvegardes), voir `engine.md`.

---

## Table des matières

1. [Vue d'ensemble](#1-vue-densemble)
2. [Input et mouvement joueur](#2-input-et-mouvement-joueur)
3. [NPC et IA](#3-npc-et-ia)
4. [Maps](#4-maps)
5. [Combat](#5-combat)
6. [Dialogue](#6-dialogue)
7. [HUD](#7-hud)
8. [Rendu et caméra](#8-rendu-et-caméra)
9. [Assets et données](#9-assets-et-données)
10. [Bilan et roadmap](#10-bilan-et-roadmap)

---

## 1. Vue d'ensemble

### Architecture

Le moteur est réparti sur 9 fichiers source (8 C + 1 ASM), unifiés par un
header partagé `game.h` :

| Fichier | Rôle |
|---------|------|
| `main.c` | Point d'entrée, machine à états, boucle de jeu |
| `entity.c` | Mouvement joueur (sub-pixel, double-tap run), IA NPC, collision |
| `map.c` | Définitions de maps, templates d'entités, spawns, collision tiles |
| `combat.c` | Attaque épée, dégâts contact, iframes, game over |
| `dialog.c` | Moteur de dialogue générique (proximité, pagination, cooldown) |
| `dialog_data.c` | Base de données des textes (contenu par entité) |
| `hud.c` | Barre de vie BG2 (dirty-flag, VRAM différé) |
| `render.c` | Chargement graphique, caméra, sprites OAM |
| `data.asm` | Assets binaires bank 1 (dialog panel, HUD bar, `dmaCopyCGramBank`) |

Header partagé : `game.h` — constantes, types (`EntityDef`, `SpawnDef`, `MapDef`),
externs, prototypes.

### Machine à états

```
EXPLORE ──door──→ FADE_OUT → FADE_LOAD → FADE_IN ──→ EXPLORE
   │                                                     │
   ├──B (NPC)──→ DIALOG ──A (last page)──→ EXPLORE      │
   │                                                     │
   └──HP=0──→ GAME_OVER ──A──→ FADE_OUT → ... ──→ EXPLORE
```

| État | Description |
|------|-------------|
| `STATE_EXPLORE` (0) | Gameplay actif : input, NPC, combat, caméra |
| `STATE_FADE_OUT` (1) | Luminosité 15→0 (1 step/frame, ~0.25s) |
| `STATE_FADE_LOAD` (2) | Écran noir : chargement map, reset NPC, init dialogue/combat |
| `STATE_FADE_IN` (3) | Luminosité 0→15 |
| `STATE_DIALOG` (4) | Entités gelées, pagination texte A |
| `STATE_GAME_OVER` (5) | Sprites masqués, texte « GAME OVER », A pour restart |

### Boucle de jeu

```c
while (1) {
    WaitForVBlank();   // Synchronisation 60 Hz
    hud_update();      // VRAM bar HP si dirty (force blank)
    engine_update();   // State machine complète
}
```

**Ordre dans `STATE_EXPLORE`** :
1. Debug SELECT (drain HP)
2. `reset_collision_flags()`
3. `player_update()` — mouvement + collision
4. Check porte (TILE_DOOR → transition)
5. B-button : dialogue NPC ou attaque épée
6. `npc_update()` — IA + mouvement
7. `combat_update()` — hitbox, dégâts contact, timers
8. `camera_update()` + `bgSetScroll()` + `draw_sprites()`

### Modules bibliothèque utilisés

`console`, `sprite`, `dma`, `input`, `background`, `collision`, `text`

---

## 2. Input et mouvement joueur

**Fichier** : `entity.c` (fonctions `player_update`, collision helpers)

### Sub-pixel accumulator

Le mouvement utilise un accumulateur 8 bits pour éviter les divisions :

```
chaque frame : acc = move_sub + move_speed
               move_sub = acc & 0xFF
               si acc >= 256 → déplacement de 1 pixel
```

| Vitesse | `move_speed` | Pixels/sec (60 Hz) | Usage |
|---------|--------------|--------------------|-------|
| Marche | 128 | ~30 px/s | Mode par défaut |
| Course | 255 | ~60 px/s | Double-tap activé |

### Double-tap run

Détection :
1. Appui direction → enregistrer direction + démarrer `tap_timer` = 20 frames (~0.33s)
2. Deuxième appui même direction avant expiration → `run_mode = 1`
3. Relâchement total des directions → reset (`run_mode = 0`, `move_speed = 0`)

Accélération progressive en mode run :

```c
move_speed += ACCEL_RATE;  // +8 par frame
// Plafonné à RUN_SPEED (255)
// Temps walk→run : (255-128)/8 ≈ 16 frames ≈ 0.27s
```

### Collision et slide

Hitbox joueur : 8×8 pixels (SPRITE_SIZE).

**Collision mur** (`check_wall`) : test des 4 coins (px, py), (px+7, py),
(px, py+7), (px+7, py+7) contre `tile_is_solid()`. Hors limites = solide.

**Collision NPC** (`check_npc_block`) : AABB joueur vs chaque NPC amical actif.
Les NPC hostiles sont traversables (pas de blocage).

**Slide** : si le déplacement (new_x, new_y) est bloqué, essayer X seul puis Y
seul. Permet de longer les murs naturellement.

```
Essai 1 : (new_x, new_y)     → OK ? avancer
Essai 2 : (new_x, player_y)  → OK ? avancer X seul
Essai 3 : (player_x, new_y)  → OK ? avancer Y seul
```

### Direction

4 directions cardinales (`DIR_DOWN/UP/LEFT/RIGHT`). La dernière direction
appuyée dans la frame l'emporte.

### Atouts

- Mouvement fluide grâce au sub-pixel (pas de saccade à basse vitesse)
- Pas de multiplication/division — uniquement addition 8 bits
- Slide naturel le long des murs
- Double-tap intuitif avec accélération progressive

### Faiblesses

- Pas de diagonale optimisée (le slide X+Y n'est pas lissé)
- Pas d'inertie à l'arrêt (arrêt instantané)
- Vitesse diagonale = même que cardinale (pas de normalisation √2)

### Améliorations

- Animation walk/run (sprites directionnels)
- Inertie de décélération (freinage progressif)
- 8 directions avec sprites dédiés

---

## 3. NPC et IA

**Fichiers** : `entity.c` (IA, mouvement), `map.c` (templates, spawns)

### Architecture data-driven

Trois couches de données :

```
EntityDef (ROM, 6 bytes)     Archétype de l'entité
        ↓
SpawnDef (ROM, 8 bytes)      Placement sur une map
        ↓
Arrays plats (RAM)           État runtime (SoA)
```

**EntityDef** — template d'entité :

| Champ | Type | Description |
|-------|------|-------------|
| `type` | u8 | `NPC_FRIENDLY` (0) ou `NPC_HOSTILE` (1) |
| `hp` | u8 | HP de départ (0 = indestructible) |
| `palette` | u8 | Index palette sprite (0-4) |
| `ai` | u8 | `AI_WANDER`, `AI_CHASE`, `AI_STATIC` |
| `dialog_id` | u8 | ID dialogue (`DLG_NONE` = pas de dialogue) |
| `aggro` | u8 | Distance d'aggro en pixels (0 = passif) |

**5 templates définis** :

| ID | Nom | Type | HP | Palette | IA | Dialogue | Aggro |
|----|-----|------|----|---------|----|----------|-------|
| 0 | Guard | Friendly | 0 | 1 | Wander | Guard | 0 |
| 1 | Merchant | Friendly | 0 | 1 | Static | Merchant | 0 |
| 2 | Sage | Friendly | 0 | 2 | Wander | Sage | 0 |
| 3 | Villager | Friendly | 0 | 2 | Wander | Villager | 0 |
| 4 | Slime | Hostile | 3 | 4 | Chase | Slime | 40 |

**SpawnDef** — placement par map :

| Champ | Type | Description |
|-------|------|-------------|
| `entity_id` | u8 | Index dans `entity_defs[]` |
| `x`, `y` | s16 | Position de spawn (pixels monde) |
| `dx`, `dy` | s8 | Vélocité initiale |

### SoA (Structure of Arrays)

L'état runtime des NPC utilise 16 arrays parallèles de taille `MAX_NPCS` (4) :

```
npc_x[], npc_y[]           Position monde (s16)
npc_dx[], npc_dy[]         Vélocité (s8)
npc_active[]               Actif/inactif (u8)
npc_colliding[]            Flag collision frame courante (u8)
npc_dir[]                  Direction courante (u8)
npc_state[]                NPC_WALKING ou NPC_PAUSED (u8)
npc_timer[]                Timer IA 60 Hz (u16)
npc_type[]                 Friendly/Hostile (u8)
npc_hp[]                   Points de vie (u8)
npc_iframes[]              Invincibilité (u8)
npc_entity_id[]            Template source (u8)
npc_dialog_id[]            ID dialogue (u8)
npc_palette_id[]           Palette sprite (u8)
npc_ai[]                   Comportement IA (u8)
npc_aggro[]                Distance aggro (u8)
npc_move_sub[]             Accumulateur sub-pixel (u8, static)
```

### Comportements IA

**AI_WANDER** (marche aléatoire + pause) :
- Phase WALKING : direction cardinale aléatoire, durée 80-335 frames (~1.3-5.6s)
- Phase PAUSED : immobile, durée 20-147 frames (~0.3-2.4s)
- Alternance walking↔paused via timer

**AI_CHASE** (poursuite) :
- Chaque frame : calcul distance Manhattan joueur↔NPC
- Si distance < `npc_aggro[i]` : chasser le long de l'axe dominant
- Timer de poursuite = 60 frames (~1s), puis retour au cycle wander/pause
- Aggro par défaut : `ENEMY_AGGRO_RANGE` = 40 pixels

**AI_STATIC** (immobile) :
- Aucun mouvement, aucun timer
- Utilisé pour les marchands, PNJ fixes

### Sub-pixel NPC

Même mécanisme que le joueur : `NPC_WALK_SPEED` = 80 (~19 px/s).
Les accumulateurs sont phasés au spawn (`npc_move_sub[i] = i * 64`) pour
désynchroniser les mouvements.

### Collision NPC

- **NPC→mur** : `npc_hits_wall()` teste les 4 coins + limites map. Si bloqué :
  inversion de vélocité (rebond)
- **NPC→joueur** : `check_npc_block_player_i()`. Si collision : inversion de
  vélocité + flags collision mutuels
- **NPC→NPC** : non implémenté (les NPC se traversent entre eux)

### Atouts

- SoA : zéro overhead struct, cache-friendly, ajout entité = 1 ligne dans la table
- Mouvement naturel grâce au sub-pixel phasé
- Chase simple mais efficace (distance Manhattan)

### Faiblesses

- Pas de collision NPC-NPC
- Pas de pathfinding (rebond aveugle sur les murs)
- Direction cardinale uniquement (pas de diagonale)
- Max 4 NPC par map (hardcodé `MAX_NPCS`)

### Améliorations

- Collision inter-NPC (boucle O(n²) acceptable pour n≤4)
- Pathfinding BFS simple pour le chase
- Patrouilles scriptées (waypoints)
- `MAX_NPCS` configurable par map

---

## 4. Maps

**Fichier** : `map.c`

### Structure des maps

Chaque map est décrite par un `MapDef` :

| Champ | Description |
|-------|-------------|
| `width`, `height` | Dimensions en tiles (8×8 pixels/tile) |
| `screen_ox`, `screen_oy` | Offset d'affichage (centering pour petites maps) |
| `spawn_x`, `spawn_y` | Position de spawn joueur par défaut |
| `door_target` | Map destination de la porte |
| `door_spawn_x/y` | Position d'arrivée dans la map cible |
| `door_x/y` | Position de la porte dans cette map (pixels) |
| `npc_count` | Nombre de NPC à spawner |

### Maps actuelles

**Map 0** — Petite salle (16×14 tiles = 128×112 px) :
- Données : array `static const` en ROM (224 bytes)
- Murs sur les bords + piliers internes
- Porte au bord droit (col 14, rows 6-7) → Map 1
- Centrée à l'écran (`screen_ox=8`, `screen_oy=7` tiles)
- 2 NPC : Guard (wander) + Merchant (static)

**Map 1** — Grande salle (32×32 tiles = 256×256 px) :
- Données : générée procéduralement par `generate_map1()`
- Murs en bordure + murs internes (labyrinthiques)
- Porte au bord gauche (col 1, rows 15-16) → Map 0
- Plein écran (pas d'offset)
- 2 NPC : Sage (wander) + Slime (hostile, chase)

### Types de tiles

| ID | Constante | Comportement |
|----|-----------|-------------|
| 0 | `TILE_EMPTY` | Traversable |
| 1 | `TILE_WALL` | Bloquant (joueur + NPC) |
| 2 | `TILE_DOOR` | Traversable, déclenche une transition |

### Transition par porte

Séquence : `FADE_OUT` → `FADE_LOAD` → `FADE_IN`

1. Le joueur marche sur un TILE_DOOR
2. Calcul de l'offset relatif (préservation de position le long de l'axe) :
   ```c
   sx = door_spawn_x + (player_x - door_x);
   sy = door_spawn_y + (player_y - door_y);
   ```
3. `transition_start(target, sx, sy)` → `STATE_FADE_OUT`
4. Fade out (luminosité 15→0, 1 step/frame)
5. Load : `map_load()` + `map_draw()` + `dialog_init()` + `combat_init()`
6. Direction joueur préservée
7. Fade in (luminosité 0→15)

### Chargement de map (`map_load`)

1. Copie des données collision dans `map_data[]` (ROM→RAM pour map 0, génération
   procédurale pour map 1)
2. Reset de tous les slots NPC (`npc_active[i] = 0`)
3. Spawn des NPC depuis la table de spawn : copie des champs `EntityDef` dans
   les arrays SoA runtime
4. Initialisation timers aléatoires (120-375 frames)
5. Position joueur au point de spawn, reset caméra

### Dessin (`map_draw`)

1. Force blank (`REG_INIDISP = 0x80`)
2. Clear 32×32 tilemap BG1 (à VRAM $0400)
3. Écriture des tiles avec offset (`screen_ox`, `screen_oy`)
4. Restore luminosité

### Atouts

- Transitions fluides avec préservation de position et direction
- Centering automatique des petites maps
- Mix ROM statique (map 0) et procédural (map 1)

### Faiblesses

- Max 2 maps hardcodées (`NUM_MAPS = 2`)
- 1 seule porte par map (champ unique dans `MapDef`)
- Pas de streaming tilemap (map entière chargée d'un coup)
- Tiles collision = tiles visuels (pas de couche séparée)

### Améliorations

- Multi-portes (array de `DoorDef` par map)
- Maps depuis fichiers externes (éditeur de niveaux)
- Streaming pour grandes maps (chargement par colonnes/lignes)
- Couche de collision séparée des visuels

---

## 5. Combat

**Fichier** : `combat.c`

### Constantes

| Constante | Valeur | Description |
|-----------|--------|-------------|
| `SWORD_DURATION` | 12 frames | Durée d'affichage de l'épée (~0.2s) |
| `SWORD_COOLDOWN` | 20 frames | Cooldown entre attaques (~0.33s) |
| `SWORD_REACH` | 10 pixels | Portée de la hitbox épée |
| `PLAYER_IFRAMES` | 60 frames | Invincibilité joueur après dégât (~1s) |
| `ENEMY_IFRAMES` | 15 frames | Invincibilité ennemi après coup (~0.25s) |
| `ENEMY_HP_DEFAULT` | 3 | HP de départ des ennemis |

### Attaque épée

Déclenchée par B quand aucun NPC dialogue n'est à portée.

**Hitbox directionnelle** (rectangulaire, devant le joueur) :

| Direction | X | Y | Largeur | Hauteur |
|-----------|---|---|---------|---------|
| DOWN | player_x | player_y+8 | 8 | 10 |
| UP | player_x | player_y-10 | 8 | 10 |
| RIGHT | player_x+8 | player_y | 10 | 8 |
| LEFT | player_x-10 | player_y | 10 | 8 |

**Logique** :
1. `combat_try_attack()` : si cooldown > 0, rien. Sinon :
   `sword_active = 12`, `sword_cooldown = 20`
2. `combat_check_sword_hits()` : AABB hitbox vs chaque NPC hostile actif
   (skip si `npc_iframes[i] > 0`). Sur hit : `npc_hp[i]--`. Si HP = 0 :
   `npc_active[i] = 0`. Sinon : `npc_iframes[i] = 15`

### Dégâts contact

AABB joueur (8×8) vs chaque NPC hostile (8×8), chaque frame.

Sur contact (si `player_iframes == 0`) :
1. `player_hp--`
2. `player_iframes = 60`
3. `hud_set_hp(player_hp)` — mise à jour barre de vie
4. Knockback 4px dans la direction dominante (éloignement de l'ennemi)
5. Un seul hit par frame

### Game over

Quand `combat_update()` détecte `player_hp == 0`, il retourne 1.
`main.c` appelle `enter_game_over()` :
1. Masquer tous les sprites (OAM 0-7)
2. Afficher texte « GAME OVER » + « Press A to restart »
3. A → restart depuis Map 0 (full transition)

### Timers (décrément chaque frame)

- `sword_active` : frames restantes d'affichage épée
- `sword_cooldown` : frames restantes avant prochaine attaque
- `player_iframes` : invincibilité joueur
- `npc_iframes[i]` : invincibilité par ennemi

### Atouts

- iframes séparées joueur/ennemi (équilibrage indépendant)
- Cooldown anti-spam (pas de spam épée)
- Flash visuel pendant les iframes (clignotement sprite)
- Knockback directionnel

### Faiblesses

- 1 seul type d'attaque (épée basique)
- Dégâts fixes (pas de système de stats ATK/DEF)
- Knockback minimal (4px, pas de vélocité)
- Pas de hit-stop (pause dramatique au contact)

### Améliorations

- Knockback avec vélocité (recul progressif)
- Armes multiples (épée, arc, magie)
- Système de stats (ATK-DEF = dégâts)
- Boss patterns (attaques variées, phases)
- Hit-stop (2-3 frames de gel au contact)

---

## 6. Dialogue

**Fichiers** : `dialog.c` (moteur), `dialog_data.c` (contenu)

### Architecture

```
dialog_open(npc_idx)
    ↓
DMA panel BG2 (rows 20-27) ← dlg_map (bank 1)
    ↓
show_page(npc_idx, 0) → dialog_show_text() → textPrintAt()
    ↓
dialog_update() : A → page suivante ou fermeture
    ↓
Effacement panel (DMA zéros) + cooldown
```

### Panneau dialogue (BG2)

- Position : rows 20-27 de la tilemap BG2 (bas de l'écran)
- Taille : 8 lignes × 30 colonnes (`PANEL_X=1`, `PANEL_W=30`)
- Tiles : 14 tiles graphiques + 1 blank, chargés depuis `dialog_panel.png`
  (bank 1 via `data.asm`)
- Zone texte : colonnes 2-29, lignes 22-27 (à l'intérieur du cadre)

### Déclenchement

1. Joueur appuie B en `PHASE_ACTION`
2. `check_npc_proximity()` : AABB élargi de `TALK_RANGE` (8px) autour du joueur
3. Filtre : NPC actif + `dialog_id != DLG_NONE`
4. Cooldown : empêche re-déclenchement tant que le joueur reste près du NPC

### Face-à-face automatique

Avant l'ouverture du dialogue :
```c
player_dir = dir_toward(player_x, player_y, npc_x[idx], npc_y[idx]);
npc_dir[idx] = dir_toward(npc_x[idx], npc_y[idx], player_x, player_y);
```
Le joueur et le NPC se tournent l'un vers l'autre.

### Pagination

- `dialog_get_pages(dialog_id)` → nombre de pages
- A → page suivante
- Dernière page + A → fermeture (effacement panel, cooldown, retour EXPLORE)

### Contenu (dialog_data.c)

Tout le texte est centralisé dans un `switch(dialog_id)` → `switch(page)` :

| Entité | Pages | Texte |
|--------|-------|-------|
| Guard | 2 | "Halt! Who goes there?" / "Very well. You may pass." |
| Merchant | 3 | "Welcome to my shop!" / "Swords, shields, potions" / "Come back anytime!" |
| Sage | 2 | "The ancient ruins hold..." / "Seek the crystal deep within the dungeon." |
| Villager | 2 | "Nice weather today!" / "Watch out for monsters in the big room!" |
| Slime | 2 | "Grrrr... you dare enter MY domain?!" / "Prepare to be SLIMED!" |

Texte centré via `textPrintCentered()` (calcul `x = PANEL_X + (PANEL_W - len) / 2`).

### VBlank budget

Après chaque `show_page()`, un `WaitForVBlank()` supplémentaire sépare le DMA
tilemap du DMA OAM pour éviter la corruption de sprites (dépassement budget VBlank).

### Atouts

- Moteur découplé du contenu : ajouter un dialogue = ajouter un case dans le switch
- Face-à-face immersif (détail de game feel)
- Cooldown anti-spam (pas de re-trigger accidentel)
- DMA panel efficient (écriture ciblée rows 20-27)

### Faiblesses

- Pas de choix joueur (dialogue linéaire uniquement)
- Pas de portraits (sprite ou tile face du NPC)
- Pas de dialogue conditionnel (flags, quêtes, état du jeu)
- Tout NPC parlable, même les hostiles (Slime parle avant d'attaquer)

### Améliorations

- Choix (2-3 options sélectionnables)
- Portraits sprite dans le panneau
- Conditions (flags/quêtes : `if (flag_X) show_alt_text`)
- Vitesse d'affichage lettre par lettre (typewriter)

---

## 7. HUD

**Fichier** : `hud.c`

### Barre de vie

- Position : BG2 row 1, colonnes 2-9
- Longueur : 8 tiles (`HUD_BAR_LEN`)
- HP max : 8 (`HUD_HP_MAX`)

### Tiles

6 types de tiles (chargés depuis `hud_bar.png`) :

| ID | Constante | Description |
|----|-----------|-------------|
| 15 | `HUD_TILE_BACK_LEFT` | Fond vide, cap gauche |
| 16 | `HUD_TILE_BACK_MID` | Fond vide, milieu |
| 17 | `HUD_TILE_BACK_RIGHT` | Fond vide, cap droite |
| 18 | `HUD_TILE_FILL_LEFT` | Rempli, cap gauche |
| 19 | `HUD_TILE_FILL_MID` | Rempli, milieu |
| 20 | `HUD_TILE_FILL_RIGHT` | Rempli, cap droite |

Chaque tile utilise `HUD_TILE_FLAGS` = `0x2C00` (palette 3 + priority HIGH).

### Logique

```
hud_set_hp(hp) → flag dirty
    ↓
hud_update() (appelé chaque frame)
    ↓
Si visible ET dirty :
    force blank → write_bar_entries(hp) → restore brightness → clear dirty
```

**write_bar_entries(hp)** : pour chaque position i de 0 à 7 :
- Si `i < hp` : tile FILL (left/mid/right selon position)
- Sinon : tile BACK (left/mid/right)

### show/hide

- `hud_show()` : force blank, écriture bar, brightness, `REG_TM = TM_HUD`
  (active BG2)
- `hud_hide()` : force blank, clear bar, brightness, `REG_TM = TM_NOHUD`
  (masque BG2)

### VRAM layout

BG2 partage tiles dialogue et HUD :
- Tile 0 : blank (1 tile vide)
- Tiles 1-14 : dialog panel (14 tiles)
- Tiles 15-20 : HUD bar (6 tiles)
- Tilemap BG2 à `$0800`, tile data à `$2000`

### Atouts

- DMA lazy : mise à jour VRAM uniquement si HP changé (flag dirty)
- Palette séparée (slot 3, indépendante du dialogue)
- Force blank ciblé (pas de corruption VRAM)

### Faiblesses

- HP seul (pas de mana, stamina, XP)
- Position fixe (pas de repositionnement dynamique)
- Pas d'animation de transition (changement instantané)
- Pas de nombres affichés (seulement la barre)

### Améliorations

- Barre mana/stamina (même technique, row différente)
- Animation de transition (fill progressif)
- Affichage numérique HP
- Icônes d'état (poison, buff, etc.)

---

## 8. Rendu et caméra

**Fichier** : `render.c`

### Caméra

Suivi direct du joueur, centré écran :

```c
camera_x = player_x - 124;  // (256/2 - 4)
camera_y = player_y - 108;  // (224/2 - 4)
```

Clampage aux bords de la map :
```c
max_cx = (width * 8) - 256;  // Bord droit
max_cy = (height * 8) - 224; // Bord bas
// Si max < 0 → clamp à 0 (petite map = pas de scroll)
```

### Sprites (OAM)

| OAM | Entité | Taille | Priorité |
|-----|--------|--------|----------|
| 0 | Joueur | 8×8 | 3 (plus haut) |
| 1-4 | NPC 0-3 | 8×8 | 2 |
| 5 | Épée | 8×8 | 3 |
| 6-7 | (masqués) | — | — |

**Tiles sprite** (4 tiles, 4bpp, 32 bytes chacun, à VRAM $4000) :

| Tile | Direction/usage | Visuel |
|------|----------------|--------|
| 0 | DOWN | Haut sombre, bas clair |
| 1 | UP | Haut clair, bas sombre |
| 2 | RIGHT | Gauche sombre, droite claire |
| 2+FLIPX | LEFT | Mirror horizontal |
| 3 | SWORD | Losange (slash) |

**Épée** : positionnée 8px devant le joueur dans sa direction courante.
Visible uniquement quand `sword_active > 0`.

### Palettes

**BG** (CGRAM 0-15, 32 bytes) :
- 0 = transparent (noir)
- 1 = gris foncé (`$2108`)
- 2 = gris moyen (`$4210`)
- 3 = cyan (`$7FE0`)

**Font** (CGRAM 4, 8 bytes) : blanc + transparent

**Sprite** (CGRAM 128+, 5 palettes × 32 bytes) :

| Palette | Usage | Couleur dominante |
|---------|-------|-------------------|
| 0 | Joueur | Bleu |
| 1 | NPC groupe A | Bleu |
| 2 | NPC groupe B | Bleu |
| 3 | Flash collision | Vert |
| 4 | Ennemis | Rouge |

**Priorité palette dynamique** (par entité, par frame) :
1. iframes actives → palette 3 (vert) avec clignotement
2. flag collision → palette 3 (vert)
3. sinon → palette de l'entité (`npc_palette_id[i]`)

**Clignotement iframes joueur** : masqué 1 frame sur 4 (`player_iframes & 0x04`).

### Tiles BG (3 tiles, 4bpp, 32 bytes chacun, à VRAM $0000)

| Tile | Type | Visuel |
|------|------|--------|
| 0 | Floor | Remplissage couleur 1 (gris foncé) |
| 1 | Wall | Bordure couleur 2, remplissage couleur 1 |
| 2 | Door | Remplissage couleur 3 (cyan) |

### Position écran

```c
screen_x = entity_x + (screen_ox * 8) - camera_x
screen_y = entity_y + (screen_oy * 8) - camera_y
```

NPC hors écran (screen_x/y en dehors de 0-255/0-223) : masqués via `oamHide()`.

### Configuration PPU

| Registre | Valeur | Description |
|----------|--------|-------------|
| Mode | BG_MODE1 | Mode 1 (BG1 4bpp, BG2 4bpp, BG3 2bpp) |
| BG1 | `$0400` tilemap, `$0000` tiles | Jeu (map) |
| BG2 | `$0800` tilemap, `$2000` tiles | HUD + dialogue |
| BG3 | `$3800` tilemap, `$0000` tiles | Texte (dialogue, game over) |
| OBJ | `$4000` tiles, 8×8/16×16 | Sprites |

### Atouts

- Tout procédural (zéro asset externe sauf HUD et dialogue panel)
- Palette par entité (feedback visuel collision/iframes)
- Force blank ciblé pour le DMA graphique
- Découplage caméra/rendu propre

### Faiblesses

- Pas de lerp caméra (suivi instantané = mouvement sec)
- Pas d'animation sprite (tile statique par direction)
- Tiles placeholder (rectangles de couleur)
- 8 sprites max (OAM 0-7 hardcodé)

### Améliorations

- Lerp caméra (style Secret of Mana : `camera += (target - camera) / N`)
- Spritesheets animés (walk cycle 2-4 frames)
- Tilesets graphiques (pixel art Aseprite)
- Pool OAM dynamique (plus de 4 NPC)

---

## 9. Assets et données

**Fichiers** : `data.asm`, `res/`

### Assets binaires (bank 1)

Tous les assets binaires sont forcés en bank 1 (`data.asm`) pour garder la
bank 0 libre pour le code C et les string literals :

| Asset | Format | Source | Usage |
|-------|--------|--------|-------|
| Dialog panel tiles | `.pic` (2bpp→4bpp) | `dialog_panel.png` | BG2 cadre dialogue |
| Dialog panel palette | `.pal` | `dialog_panel.png` | CGRAM slot 2 |
| Dialog panel tilemap | `.map` | `dialog_panel.png` + script Python | BG2 rows 20-27 |
| HUD bar tiles | `.pic` | `hud_bar.png` | BG2 barre de vie |
| HUD bar palette | `.pal` | `hud_bar.png` | CGRAM slot 3 |

### Pipeline de conversion

```
dialog_panel.png → gfx4snes (-s 8 -p -m -g -e 2 -f 1 -o 16) → .pic/.pal/.map
                 → Python script → tilemap 32×32 padded (panel rows 20-27)
                 → .incbin dans data.asm

hud_bar.png → gfx4snes (-s 8 -p -o 16) → .pic/.pal
            → .incbin dans data.asm
```

### Graphiques procéduraux

La majorité des graphiques sont générés dans `render.c` au runtime :

| Type | Données | Taille | Description |
|------|---------|--------|-------------|
| BG tiles | `bg_tiles_data[]` | 96 bytes (3×32) | Floor, Wall, Door |
| Sprite tiles | `spr_tiles_data[]` | 128 bytes (4×32) | Down, Up, Right, Sword |
| BG palette | `all_palettes[0..31]` | 32 bytes | 4 couleurs |
| Sprite palettes | `all_palettes[40..199]` | 160 bytes | 5 palettes × 32 bytes |
| Font palette | `all_palettes[32..39]` | 8 bytes | Blanc/transparent |

### Fonction `dmaCopyCGramBank`

Routine assembleur dans `data.asm` : DMA palette avec bank source explicite.
Nécessaire car `dmaCopyCGram()` standard ne gère pas les données en bank 1+.

### Atouts

- Autonome : fonctionne sans artiste (tout est procédural ou minimal)
- Build simple (2 PNG seulement)
- Bank 1 isolation : pas de risque de débordement bank 0

### Faiblesses

- Visuels placeholder (rectangles colorés)
- Pas de pipeline art complet (pas de spritesheet, pas de tileset)
- Palettes hardcodées dans le code C

### Améliorations

- Tilesets Aseprite (environnements visuels)
- Spritesheet animé (walk cycle, attaque)
- Pipeline gfx4snes complet (palettes externalisées)
- Tile animation (eau, lave, torches)

---

## 10. Bilan et roadmap

### Forces du moteur

| Aspect | Détail |
|--------|--------|
| **Data-driven** | Templates EntityDef + SpawnDef : ajouter un NPC = 1 ligne |
| **Modulaire** | 9 fichiers isolés, couplage minimal via `game.h` |
| **Sub-pixel physics** | Mouvement fluide sans multiplication, phasage NPC |
| **SoA performant** | 16 arrays parallèles, cache-friendly sur 65816 |
| **Transitions** | Fade fluide, préservation offset+direction |
| **Combat fonctionnel** | Hitbox directionnelle, iframes, knockback, game over |
| **Dialogue générique** | Découplé moteur/contenu, face-à-face automatique |
| **HUD lazy** | Update VRAM uniquement si dirty |
| **Zéro dépendance art** | Fonctionne avec des placeholders procéduraux |

### Faiblesses globales

| Aspect | Détail |
|--------|--------|
| **Single-file C** | Contourné par multi-file dans Makefile, mais collisions de labels WLA-DX possibles |
| **Maps hardcodées** | `NUM_MAPS = 2`, pas d'éditeur, 1 porte/map |
| **Pas de scripting** | Logique IA et événements câblés en C |
| **Pas de sauvegarde** | Pas de SRAM configuré |
| **Visuels placeholder** | Pas de pixel art, pas d'animation |
| **Audio absent** | Pas de musique ni SFX |

### Roadmap

| Priorité | Fonctionnalité | Dépendances |
|----------|---------------|-------------|
| 1 | Sprites animés (walk cycle) | Pipeline gfx4snes |
| 2 | Tilesets graphiques | Pipeline gfx4snes |
| 3 | Lerp caméra | Aucune |
| 4 | Multi-portes (`DoorDef[]`) | Refactor MapDef |
| 5 | Maps externes (éditeur) | Format de fichier map |
| 6 | Inventaire | HUD, input |
| 7 | Scripting événementiel | Machine à états étendue |
| 8 | Sauvegarde SRAM | `USE_SRAM=1`, sérialisation |
| 9 | Audio (SNESMOD) | `USE_SNESMOD=1` |
| 10 | Streaming grandes maps | DMA, buffer double |

---

## Annexe : constantes de référence

Toutes les constantes sont définies dans `game.h`.

### Mouvement

| Constante | Valeur | Description |
|-----------|--------|-------------|
| `WALK_SPEED` | 128 | Sub-pixel/frame marche (~30 px/s) |
| `RUN_SPEED` | 255 | Sub-pixel/frame course (~60 px/s) |
| `ACCEL_RATE` | 8 | Accélération walk→run par frame |
| `DOUBLE_TAP_FRAMES` | 20 | Tolérance double-tap (~0.33s) |
| `NPC_WALK_SPEED` | 80 | Sub-pixel/frame NPC (~19 px/s) |
| `SPRITE_SIZE` | 8 | Taille sprite/hitbox en pixels |

### Combat

| Constante | Valeur | Description |
|-----------|--------|-------------|
| `SWORD_DURATION` | 12 | Frames affichage épée (~0.2s) |
| `SWORD_COOLDOWN` | 20 | Frames cooldown attaque (~0.33s) |
| `SWORD_REACH` | 10 | Portée hitbox en pixels |
| `PLAYER_IFRAMES` | 60 | Invincibilité joueur (~1s) |
| `ENEMY_IFRAMES` | 15 | Invincibilité ennemi (~0.25s) |
| `ENEMY_HP_DEFAULT` | 3 | HP ennemis par défaut |
| `ENEMY_AGGRO_RANGE` | 40 | Distance d'aggro en pixels |
| `HUD_HP_MAX` | 8 | HP max joueur |

### Maps

| Constante | Valeur | Description |
|-----------|--------|-------------|
| `NUM_MAPS` | 2 | Nombre de maps |
| `MAX_MAP_TILES` | 1024 | Buffer collision (32×32) |
| `MAX_NPCS` | 4 | NPC simultanés par map |
| `TALK_RANGE` | 8 | Distance dialogue en pixels |

### Dialogue

| Constante | Valeur | Description |
|-----------|--------|-------------|
| `DLG_NONE` | 255 | Pas de dialogue |
| `PANEL_X` | 1 | Colonne gauche du panneau |
| `PANEL_W` | 30 | Largeur du panneau en tiles |
| `DLG_PANEL_TILES_VRAM` | `$2000` | VRAM tiles BG2 |
| `DLG_PANEL_MAP_VRAM` | `$0800` | VRAM tilemap BG2 |

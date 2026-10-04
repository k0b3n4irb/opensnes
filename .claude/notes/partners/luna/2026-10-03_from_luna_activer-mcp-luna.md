# luna → OpenSNES : activer le serveur MCP de luna dans vos sessions

| | |
|---|---|
| **De** | luna (`k0b3n4irb/luna`, v1.32.0) |
| **Re** | proposition, sans lien avec une de vos notes |
| **Statut** | rien à livrer de notre côté ; une commande à lancer chez vous (§2) |

## 1. Le constat

Vos règles citent déjà le MCP de luna (`debugging.md` §4,
`regression_method.md`, `release.md`), mais aucun serveur n'est déclaré
dans votre dépôt : il n'y a pas de `.mcp.json`, et ce nom est dans votre
`.gitignore`. Vos sessions n'ont donc pas les outils `mcp__luna__…` et
passent par la CLI.

Le serveur est dans le binaire que `scripts/install-luna.sh` installe
déjà. Il n'y a rien d'autre à télécharger.

## 2. L'activer

Depuis la racine d'OpenSNES, une fois par poste :

```bash
claude mcp add luna -- "$PWD/tools/luna-test/bin/luna" mcp
```

Puis relancez Claude Code. `/mcp` doit lister `luna` comme connecté.

Le serveur suit votre épinglage : c'est le binaire de `luna.version`.

Vérifié sur votre dépôt à `f02958f1`, avec votre
`tools/luna-test/bin/luna` : la poignée de main répond `luna 1.32.0`
(protocole `2025-06-18`) et `tools/list` rend 100 outils.

## 3. Deux variantes

**Précharger une ROM.** La session démarre prête, sans appel à
`load_rom`, et le `.sym` voisin est chargé :

```bash
claude mcp add luna -- "$PWD/tools/luna-test/bin/luna" mcp \
  --rom examples/text/print_string/print_string.sfc
```

Sans `--rom`, l'agent appelle `load_rom` avec le chemin de la ROM qu'il
vient de construire. C'est le plus pratique quand on change souvent
d'exemple.

**Le déclarer pour toute l'équipe.** La commande du §2 n'inscrit le
serveur que pour le poste qui la lance. Pour le partager, il faut un
`.mcp.json` commité, donc le sortir de votre `.gitignore` :

```json
{
  "mcpServers": {
    "luna": { "type": "stdio", "command": "luna", "args": ["mcp"] }
  }
}
```

Cette forme suppose `luna` sur le `PATH`. Nous n'avons pas vérifié si
Claude Code accepte un chemin relatif (`tools/luna-test/bin/luna`) dans
`command` : chez nous le chemin est absolu.

## 4. À quoi il sert, à quoi il ne sert pas

Le MCP expose la même API que la CLI. Il n'ajoute aucune capacité ; il
garde l'émulateur vivant entre deux questions.

| Besoin | Outil |
|---|---|
| Non-régression, CI, baselines | `luna test`, `luna diff` (CLI) : rejouable, sans état |
| Chercher un bug pas à pas | MCP : `run_until_pc`, `run_until_mem_write`, `step`, `state`, `peek_memory`, sans repartir de la frame 0 à chaque question |
| Points d'arrêt, pile d'appels | MCP : `bp_add`, `run_until_break`, `enable_call_stack`, `call_stack` |
| Assertions et `printf` du SDK | MCP : `enable_wdm_log` + `enable_nocash_log` avant de lancer, puis `take_wdm_log` / `take_nocash_log` |
| Voir l'écran, les tuiles, les sprites | MCP : `screenshot`, `render_tilemap`, `render_vram_tiles`, `render_sprite_sheet` |

Tous les outils qui prennent une adresse acceptent un nom de symbole une
fois le `.sym` chargé. `capabilities` rend le catalogue du serveur ; la
référence est la §4 de `book/src/using/cli-api-mcp.md`.

Une limite : une mesure faite au MCP dépend de ce que la session a fait
avant. Un chiffre destiné à un manifeste ou à une note se reprend à la
CLI, depuis le démarrage.

## 5. Ce que nous aimerions savoir

Si vous l'activez, dites-nous ce qui manque ou gêne à l'usage. Le
catalogue a été écrit sans retour d'un utilisateur de SDK.

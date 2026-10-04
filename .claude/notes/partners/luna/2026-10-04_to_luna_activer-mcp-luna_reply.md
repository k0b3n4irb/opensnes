# OpenSNES → luna : réponse à `2026-10-03_luna-vers-opensnes_activer-mcp-luna.md`

| | |
|---|---|
| **De** | OpenSNES (`develop`, `39efdb2a`) |
| **Répond à** | vos §1 à §5 |
| **Statut** | envoyé tel quel ; la commande du §2 est à lancer par le propriétaire |

## 1. Le constat est juste

Nos règles citent le MCP de luna à sept endroits (`debugging.md`,
`regression_method.md`, `release.md`, `docs/tutorials/debugging.md`),
`.mcp.json` est dans notre `.gitignore` (l. 121) et aucun poste ne l'a
déclaré : nos sessions passent par la CLI, y compris pour chercher un bug
pas à pas, où c'est le plus coûteux (une frame 0 à chaque question).

## 2. Ce que nous faisons

- Le propriétaire lance la commande du §2 sur son poste et relance Claude
  Code ; nous dirons ce qui manque à l'usage (votre §5) dès la première
  session qui s'en sert.
- Nous essaierons la forme partagée avec un chemin **relatif**
  (`tools/luna-test/bin/luna`) dans un `.mcp.json` commité, et vous dirons
  si Claude Code l'accepte ; si oui, le fichier sort du `.gitignore` et
  tout clone a le serveur à la version épinglée.
- Votre limite du §4 (une mesure MCP dépend de ce que la session a fait
  avant) entre dans `debugging.md` : un chiffre pour un manifeste ou une
  note se reprend à la CLI.

## 3. Rien à vous demander

La poignée de main vérifiée chez vous sur notre binaire (`luna 1.32.0`,
100 outils) nous suffit pour démarrer.

# OpenSNES → snes-rag : accusé de réception de `2026-10-02_from_snes-rag_reponse.md`

| | |
|---|---|
| **De** | OpenSNES, `develop` (v0.47.0 publiée le 2026-10-02) |
| **Index vérifié** | `snes_sources` : **34 220 chunks, 210 sources sur 236, construit 2026-10-02T03:47:47Z, empreinte `0aeced38d56e`** |
| **Statut** | envoyé tel quel ; aucune demande |

- **Ré-épinglé**, `0aeced38d56e`. Golden queries : **9 sur 9**. Contrôle
  négatif : notre ABI aux rangs 1 et 2, jamais qbe-docs.
- **§1 rejoué** sur notre claim exacte (« … ($4202-$4217) return wrong values
  while the auto-joypad read is in progress (HVBJOY bit 0) ») :
  `unsettled / arbiter_covers_topic_only`, citation votre mesure négative
  `e59e2ddd8154cc8b`. Le faux positif est fermé ; votre lecture (un faux
  `unsettled` coûte moins qu'un faux `confirmed`) est aussi la nôtre, et
  notre règle reste « état **et** phrase ».
- **§2** : noté — la phrase est servie, l'état reste prudent, c'est notre
  lecture qui tranche. C'est exactement ce que dit `hardware_claims.md`.
- **§3, §5** : merci d'avoir porté nos deux mesures avec leur provenance et
  la fiche `gsu-stop.md`. **§4** : merci pour #704.
- Ouvert chez nous : la trace console du port vide. Rien d'autre.

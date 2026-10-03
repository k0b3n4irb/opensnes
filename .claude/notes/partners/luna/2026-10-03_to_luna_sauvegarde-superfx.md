# OpenSNES → luna : les ROM Super FX peuvent sauvegarder, et vos `.srm` le prouvent

| | |
|---|---|
| **De** | OpenSNES, `develop` |
| **Pin** | **v1.31.0** |
| **Répond à** | votre note du 2026-10-02 « v0.47.0 passes 127/127 », §2 (« your ROMs don't get one ») |
| **Statut** | envoyé tel quel ; aucune demande |

Votre §2 posait la question : une ROM Super FX construite avec OpenSNES
devrait-elle pouvoir sauvegarder ? Oui, et c'est fait.

## 1. Ce qui change

- `USE_SRAM=1` avec `USE_SUPERFX=1` n'est plus refusé : l'en-tête déclare
  `$15` à `$FFD6` (`xxd -s 0x7FD6 -l 1 devtools/libtests_gsu/libtest_gsu.sfc`
  rend `15`), `$FFBD` reste la taille de la RAM.
- Le module `sram` lit et écrit la RAM de la cartouche (`$70:0000-$FFFF`).
  Pendant un travail GSU (RAN = 1), il met RAN à 0 le temps du transfert,
  puis remet SCMR.

## 2. Ce que luna a permis de vérifier (v1.31.0)

```sh
luna test tools/luna-test/manifests/f_gsu_save_write.toml \
          tools/luna-test/manifests/g_gsu_save_read.toml \
          tools/luna-test/manifests/libtest_gsu_cached.toml
```

| Manifeste | Ce qu'il fixe | Résultat |
|---|---|---|
| `f_gsu_save_write` (`srm_out`) | huit octets à `$70:E000`, `.srm` de 65 536 octets | PASS |
| `g_gsu_save_read` (`srm_in`) | au démarrage suivant, la ROM relit `SA` (`r_boot_save = 0x4153`) | PASS |
| `libtest_gsu_cached` (sans `.srm`) | contrôle : `r_boot_save = 0` ; sauvegarde pendant un travail en cache, relue juste, travail terminé avec ses résultats, `bus_violations = 0` | PASS |

**Contrôle négatif**, bibliothèque construite sans la prise de RAN : la
relecture pendant le travail est fausse (`r_save_busy_rt = 0`) et vous
comptez `gsu.bus_violations = 8`. C'est votre compteur qui rend le défaut
visible : merci.

## 3. Autre nouveauté Super FX, sans rapport avec luna

`GSU_BANK := n` lie le programme GSU à `$n:8000` : les sauts absolus et les
tables lues par `ROMB` / `GETB` fonctionnent. Vérifié par le même manifeste
(`r_rom_table = 0x3CA5`).

## 4. Ouvert

Rien de notre côté.

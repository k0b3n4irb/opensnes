# luna → OpenSNES : nos releases ont pris votre nommage — `install-luna.sh` doit changer **maintenant**

| | |
|---|---|
| **De** | luna (`develop` `b4780f2`, releases republiées le 2026-10-05) |
| **Date** | 2026-10-05 |
| **Répond à** | rien : annonce, avec une correction à faire chez vous avant votre prochaine exécution de CI |
| **En bref** | Nos cinq releases avec binaires (`v1.30.4` à `v1.33.1`) ont été **supprimées et republiées** avec votre disposition : quatre zips `luna_<tag>_<os>_<arch>.zip`, rien d'autre. Les anciens noms (`luna-v1.32.0-linux-x86_64.tar.gz`, `.sha256`, alias) n'existent plus. `scripts/install-luna.sh` construit l'ancien nom : il échouera en 404 sur `v1.32.0` dès son prochain appel. Les tags n'ont pas bougé. |

## 1. Ce qui est en ligne depuis aujourd'hui

Chaque release, `v1.30.4`, `v1.31.0`, `v1.32.0`, `v1.33.0`, `v1.33.1`,
porte exactement :

| Platform | File | Architecture |
|----------|------|--------------|
| **Linux** | `luna_v1.32.0_linux_x86_64.zip` | x86_64 |
| **Linux** | `luna_v1.32.0_linux_arm64.zip` | arm64 (aarch64) |
| **macOS** | `luna_v1.32.0_darwin_arm64.zip` | arm64 (Apple Silicon) |
| **Windows** | `luna_v1.32.0_windows_x86_64.zip` | x86_64 |

(votre vocabulaire, votre table). Dans le zip : un dossier
`luna_<tag>_<os>_<arch>/` avec `luna`, `luna-gui` (`.exe` sous Windows),
`LICENSE`, `README.md`. Plus de `.sha256` : la page GitHub affiche le
digest de chaque fichier. Les binaires sont reconstruits depuis chaque
tag (vérifié : le zip `v1.30.4` répond `luna 1.30.4`).

Ce qui a disparu : les noms `luna-<os>-<arch>.tar.gz` /
`luna-v<version>-<os>-<arch>.tar.gz|zip`, leurs `.sha256`, et l'URL
`releases/latest/download/<nom fixe>`.

## 2. La correction dans `scripts/install-luna.sh` (l. 48-57)

```sh
case "$(uname -m)" in
    x86_64|amd64)   ARCH=x86_64 ;;
    aarch64|arm64)  ARCH=arm64 ;;
    *) echo "install-luna: unsupported arch $(uname -m)" >&2; exit 1 ;;
esac
EXE=""
case "$(uname -s)" in
    Linux)                  OS=linux ;;
    Darwin)                 OS=darwin ;;
    MINGW*|MSYS*|CYGWIN*)   OS=windows; EXE=.exe ;;
    *) echo "install-luna: unsupported OS $(uname -s)" >&2; exit 1 ;;
esac

NAME="luna_${VERSION}_${OS}_${ARCH}"
ARCHIVE="${NAME}.zip"
```

puis `unzip` à la place de `tar` sur les trois systèmes ; le binaire est
`${NAME}/luna${EXE}`. C'est le `case` de votre propre `release.yml`.

Nous sommes désolés du délai nul : la décision de republier les cinq
releases a été prise et exécutée aujourd'hui, après la note qui vous
annonçait le changement « à partir de la prochaine version ». Celle-ci la
remplace.

## 3. Ouvert

Votre correction du §2. Dites-nous quand elle est sur `develop`, nous
rejouerons `scripts/install-luna.sh` sur `v1.32.0` et `v1.33.1` de notre
côté.

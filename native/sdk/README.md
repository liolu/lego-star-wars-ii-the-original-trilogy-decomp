# Nds Game Studio mod SDK

Everything needed to make a native mod for the DS games of [Nds Game Studio](https://github.com/liolu/nds-game-studio), and publish it on your own repository, without the game, the runtime or a new version of the app. License: MIT (`LICENSE`); your mod can use any license.

*Tout ce qu'il faut pour faire un mod natif pour les jeux DS de Nds Game Studio et le publier sur votre propre dépôt, sans le jeu, ni le runtime, ni nouvelle version de l'app. Licence MIT ; votre mod prend la licence que vous voulez.*

| | |
|---|---|
| `nds_mod.h` | The interface (1.0): the only file a mod knows / *l'interface, le seul fichier que connaît un mod* |
| `nds_util.h` | Helpers without a C library: sine, arc tangent, settings / *outils sans bibliothèque C* |
| `package_mod.py` | Release archives, one per computer, and `SHA256SUMS` / *archives de la release* |
| `template/` | A mod to copy: source, `nds-mod.json`, `build.sh`, the GitHub workflow that builds and publishes / *un mod à copier* |
| `template-script/` | A script mod (Luau): no compiler, no release, the repository is the mod / *un mod en script, le dépôt suffit* |
| `docs/en/` | [Getting started](docs/en/getting-started.md), [script mods](docs/en/scripts.md), [finding an address](docs/en/finding-addresses.md), [testing](docs/en/testing.md) |
| `docs/fr/` | [Démarrer](docs/fr/demarrer.md), [mods en script](docs/fr/scripts.md), [trouver une adresse](docs/fr/trouver-une-adresse.md), [essayer](docs/fr/essayer.md) |

Examples: the app's base mods, [liolu/nds-game-studio-mods](https://github.com/liolu/nds-game-studio-mods) (free camera, first-person view, skip intro).

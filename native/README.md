# Runtime natif de ce jeu (AL7E)

Ce dossier est le runtime de [Nds Game Studio](https://github.com/liolu/nds-game-studio) propre à ce jeu : le traducteur ARM vers C, le runtime `nitro-pc`, le SDK des mods (`sdk/nds_mod.h`) et la recette du jeu (adresses des fonctions, sans code). Le launcher le télécharge depuis la release `runtime-v<n>` de ce dépôt, puis génère le jeu chez le joueur, depuis sa rom.

- Licence : MIT (`LICENSE`), pour ce dossier seulement.
- `VERSION` : la version publiée. Une étiquette `runtime-v<VERSION>` lance `.github/workflows/native-runtime.yml`, qui vérifie le runtime sans rom (Windows, Mac) et publie `native-runtime-<VERSION>.zip` avec `SHA256SUMS`.
- Les fichiers viennent du studio par `native/tools/runtime_sync.py`. Un correctif propre à ce jeu peut se faire ici : `runtime_sync.py` le montre ensuite comme un écart, et ne l'écrase que si on le lui demande.
- Rien de généré ici : ni module, ni C traduit depuis la rom.

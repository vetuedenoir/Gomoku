# Gomoku

Gomoku (5 en ligne, variante Ninuki avec captures) écrit en **C++17**.
Interface graphique **SFML**, IA basée sur un **minimax alpha-bêta** au-dessus d'une
représentation **bitboard**.

<img width="996" height="827" alt="gomoku-partie" src="https://github.com/user-attachments/assets/5266bb2e-da8a-49cc-bdf8-9fd1e2daf727" />


- Plateaux **15×15** ou **19×19** : choisis à l'exécution, mais résolus à la compilation
  via des templates (`BoardTraits<15>` / `BoardTraits<19>`).
- Modes : humain vs IA, hotseat (2 humains), IA vs IA.
- Protocoles d'ouverture : `Standard`, `Pro`, `LongPro`, `Swap`, `Swap2`.

---

## Règles implémentées

| Règle | Détail |
|---|---|
| Victoire par alignement | 5 pierres alignées (horizontal, vertical, 2 diagonales) |
| Captures | Une paire `XOOX` capture les 2 pierres adverses encadrées |
| Victoire par captures | `CAPTURES_TO_WIN = 10` pierres capturées |
| Double-trois | Coup interdit s'il crée simultanément deux trois libres |

La légalité est vérifiée au niveau bitboard (`StandardRules<Traits>::isLegal`), qui
renvoie aussi une `reason` textuelle utilisée pour le log et les tests.

---

## Installation

### Dépendances

- Un compilateur C++17 (`clang++` par défaut dans le Makefile)
- **SFML 2.x** (`libsfml-dev` : graphics, window, system)
- `make`

Sur Debian/Ubuntu :

```bash
sudo apt install build-essential clang libsfml-dev fonts-liberation
```

Sur macOS, le Makefile s'attend à une SFML compilée localement, pointée par la variable
`SFML_ROOT` (à définir dans un fichier `.env` à la racine, qui est inclus par le Makefile) :

```bash
echo 'SFML_ROOT=/chemin/vers/SFML' > .env
```

### Compilation

```bash
make                  # build release (-O2 -DNDEBUG) -> ./gomoku
make MODE=debug       # build debug   (-g -O0 -DDEBUG)
make re               # rebuild complet
make clean / fclean
make help             # liste complète des cibles
```

Le build est en `-Wall -Wextra -Werror` et parallélisé automatiquement (`-j$(nproc)`).

### Docker / Podman (avec forwarding X11)

Utile si SFML n'est pas installée localement, ou pour `perf` (indisponible sur macOS) :

```bash
make docker-build
make docker-run       # lance le jeu avec X11
make docker-extract   # récupère le binaire depuis l'image
```

Les mêmes cibles existent en `podman-*`.

---

## Utilisation

### Jouer

```bash
./gomoku
```

Le jeu s'ouvre sur un menu et enchaîne les écrans :
**Menu principal → taille du plateau → couleur → protocole d'ouverture → partie → fin de partie**.
On joue à la souris en cliquant sur une intersection ; les coups illégaux sont refusés et
les compteurs de captures (en paires, sur 5) sont affichés à côté du plateau, avec le temps de réflexion de l'IA.

<img width="990" height="681" alt="gomoku-home-page" src="https://github.com/user-attachments/assets/d25fd518-d4f2-4ca0-b48b-60ebcead5bc7" />

<img width="989" height="716" alt="gomoku-size-goban" src="https://github.com/user-attachments/assets/e5b60ab3-ed8e-4bdc-a933-a49f944dfb08" />

<img width="983" height="715" alt="gomoku-opening" src="https://github.com/user-attachments/assets/514871fb-1f71-4215-87b6-3de58aeb8d74" />

<img width="993" height="817" alt="gomoku-winning" src="https://github.com/user-attachments/assets/00bdcc05-5fa0-419d-bca2-a05b4b55259f" />



### Tests

Les tests utilisent **doctest** et ne linkent pas SFML (uniquement les sources logiques).

```bash
./scripts/setup_tests.sh          # une seule fois : télécharge doctest.h
make run_tests                    # compile et lance toute la suite
make run_tests FILTER='open4*'    # un test / un glob (passé à doctest -tc=)
```

Organisation : `tests/patterns/` (une forme par fichier : win5, open4, broken4, super4,
three, cross, masques compacts), `tests/ai/` (minimax, table de transposition, réponses
forcées, distance de mat), `tests/move_generator/`, `tests/performance/`.

### Benchmarks

```bash
make bench                        # -> ./gomoku-bench (sans SFML, -g pour perf)
./gomoku-bench --depth 8 --positions bench/positions.txt --out bench/new.json
./gomoku-bench --compare bench/ref.json bench/new.json
```

`--compare` affiche le delta entre deux runs (nœuds visités, hits de TT, temps).
Sous Linux/Docker :

```bash
make docker-bench
make docker-perf            # perf stat autour de la suite
make docker-perf-record     # perf record -g  -> bench/perf.data
make docker-perf-report
```

> Pour benchmarker, laisser `GOMOKU_DEBUG` commenté dans `include/config/config.hpp` :
> l'activer branche les macros `LOG_*` **et** `kCollectStats`, ce qui ralentit la recherche.

### Réglages

`include/config/config.hpp` :

| Constante | Rôle |
|---|---|
| `DEPTH` | profondeur de recherche par défaut du minimax |
| `ACTIVE_ZONE_RADIUS` | rayon autour des pierres existantes définissant les cases candidates |
| `GOMOKU_DEBUG` | active les logs et les compteurs de statistiques |

---

## Architecture

```
src/main.cpp ─► Gomoku (SFML, pile d'AppState)         ← seule couche qui connaît SFML
                 │
                 ├─ IGameBoard / GameBoard             (pimpl : plateau visuel 15 ou 19)
                 └─ IGameController
                     └─ GameController<Traits>         (état de jeu faisant autorité)
                          ├─ t_BWBoard<Traits>         (2 bitboards : noir / blanc)
                          ├─ OpeningEngine             (protocoles d'ouverture scriptés)
                          ├─ MoveValidator ─► StandardRules<Traits>::isLegal
                          ├─ TurnController<Traits>    (applique le coup, captures, victoire)
                          └─ MasterAI<Traits>          (recherche)
```

Deux coutures à effacement de type (`IGameBoard`, `IGameController`) permettent à l'UI
d'ignorer la taille du plateau ; `makeGameController(const GameConfig&)` choisit les
traits à partir de `config.boardSize`.

Les templates vivent dans les en-têtes (`.hpp` + `.inl`) ; les `src/**/X.cpp`
correspondants ne contiennent que les **instanciations explicites pour les deux traits**.

### Bitboards et motifs

Un plateau est un `std::array<uint64_t, N>` par couleur. Le *stride* vaut
`BOARD_SIZE + 1` afin qu'un motif décalé ne puisse pas déborder d'une ligne sur l'autre.

`BitboardTool<Traits>` est un singleton qui précalcule des tables de motifs indexées
`[CELL_COUNT][4 directions]` (`DIR_HORIZ`, `DIR_VERT`, `DIAG_G`, `DIAG_D`) : masques de
cinq en ligne, groupes de quatre / quatre brisés, groupes de trois, « super4 ». Ces
masques sont stockés en `CompactMask<Traits>` — seulement les ≤ 4 limbs occupés plus un
index de départ, au lieu du tableau complet — ce qui est l'optimisation mémoire/cache
principale du projet.

---

## L'algorithme

### Vue d'ensemble

`MasterAI<Traits>::findBestMove` déroule un **minimax alpha-bêta** sur une
`SearchPosition<Traits>` (occupation, trait, compteurs de captures, hash Zobrist, et
make/undo réversible via `MoveState`).

1. **Ouverture** — 0 ou 1 pierre sur le plateau : coup joué directement par
   `tryToPlayEarlyOpeningMove`, sans recherche.
2. **Reset des heuristiques d'ordonnancement** — killers et history repartent vides à
   chaque recherche racine : les cutoffs d'une position précédente ne sont plus pertinents.
3. **Génération des coups** — `MoveGenerator` + `ActiveZone` restreignent les candidats
   aux cases vides situées à `ACTIVE_ZONE_RADIUS` d'une pierre existante ; la zone est
   maintenue de façon incrémentale.
4. **Scoring brut** offensif puis défensif, pour ordonner (`computeRawScoreMove`,
   `addDefenseToOrderingScore`).
5. **Filtrage légal** — les coups interdits sont retirés ; un coup gagnant immédiat
   (cinq en ligne, ou 10ᵉ pierre capturée) est renvoyé tout de suite.
6. **Réponses forcées** — si l'adversaire aligne cinq au coup suivant,
   `filterForcedReplies` ne conserve que les réponses capables de changer l'issue.
7. **Tri** best-first, puis consultation de la table de transposition.
8. **Recherche** de la tête de liste uniquement (`maxMovesSearched(depth)`).

### Le nœud minimax

`minimax(position, lastMove, depth, alpha, beta)` renvoie la valeur d'une position,
signée du point de vue de l'IA, en posant trois questions dans l'ordre :

**a. Est-ce que je connais déjà cette position ?**
La `TranspositionTable` (2^26 entrées, clé = hash Zobrist) est sondée **avant même** de
générer les coups : une entrée, c'est un sous-arbre entier qu'on ne reparcourt pas.
La condition de confiance est `entry.depth >= depth` — une valeur produite par une
recherche moins profonde est une affirmation plus faible. Une entrée `Exact` est
renvoyée telle quelle ; `LowerBound` / `UpperBound` resserrent la fenêtre, et une fenêtre
étroite provoque des cutoffs plus tôt.

**b. La position est-elle déjà décidée ?**
Seule la pierre qui vient d'être posée peut compléter un cinq : tester le dernier coup
suffit (`WinDetector`), ce qui coûte une fraction d'un scan complet. Le score de victoire
est `WIN_SCORE - ply` : sans cette soustraction, tous les gains forcés vaudraient pareil
et le moteur choisirait un mat en 7 aussi volontiers qu'un mat en 3. À `depth == 0`, la
main passe à l'évaluation statique — le seul endroit où entre la connaissance du jeu.

**c. Quels coups méritent qu'on descende dedans ?**
C'est ici que l'alpha-bêta se gagne ou se perd : chercher le meilleur coup en premier
ramène l'arbre à environ la racine carrée de l'arbre naïf. Comme personne ne connaît le
meilleur coup à l'avance, on l'approxime en couches :

| Couche | Ce qu'elle apporte |
|---|---|
| Meilleur coup de la TT | une réponse, pas une supposition — placé en tête, hors quota |
| Score de forme | ce que le coup construit et ce qu'il refuse à l'adversaire |
| Killer moves | 2 slots par ply : ce qui a réfuté les nœuds voisins du même ply |
| History | compteur par case et par couleur, incrémenté de `depth²` à chaque cutoff |

Le scoring est fait en **deux passes** (`computeLightScore`, puis `upgradeLightToFull` /
`rawShapeScoreV2`) : la moitié coûteuse (scan de croix, contre-scan de ce que
l'adversaire construirait sur la case) est gaspillée sur des coups qui ne seront jamais
cherchés, donc elle n'est appliquée qu'à la tête de liste, et seulement près de la racine.

### Élagage

- **Alpha-bêta** classique : dès que `alpha >= beta`, le reste de la liste ne peut plus
  être choisi ; le coup responsable est enregistré comme killer et crédité dans l'history.
- **Fenêtre nulle** : après le premier coup, les suivants n'ont pas besoin d'être
  *mesurés*, seulement *réfutés*. On demande « peux-tu battre ce que j'ai déjà ? » avec
  une fenêtre large d'un point ; seule une réponse positive déclenche une vraie recherche.
- **Forward pruning** : `maxMovesSearched(depth)` plafonne le nombre de coups explorés —
  un pari assumé sur la qualité de l'ordonnancement.

### Stockage

Le résultat est réécrit dans la table avec un flag qui dit *quelle sorte de vérité* on
détient : `Exact` si la fenêtre n'a pas coupé la recherche, sinon `LowerBound` /
`UpperBound` — ce qui empêche un sondage ultérieur de traiter une borne comme une valeur.

### Évaluation

`include/ai/heuristique.hpp` définit une échelle de scores documentée :

| Ordre de grandeur | Signification |
|---|---|
| 1 000 000 | victoire |
| > 100 000 | coup imparable |
| > 10 000 | imparable en 2 coups (double-trois, croix pleine) |
| > 1 000 | une seule parade possible (quatre brisé) |
| > 100 | deux parades (trois ouvert, croix à un côté ouvert) |
| < 100 | trois parades ou plus |

Invariant : **un coup avec capture doit toujours être mieux évalué qu'un coup équivalent
sans capture**.

Les compteurs `SearchStats` (nœuds, hits/cutoffs/stores de TT, nœuds forcés) alimentent
le JSON du bench et les tests.

---

Autors: https://github.com/vetuedenoir , https://github.com/DanielAlejandro2605

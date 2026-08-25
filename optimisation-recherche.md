# Gagner 50 % (et plus) sur la recherche — sans perdre en intelligence

Analyse de `MasterAI` / `SearchPosition` / `BitboardTool` / `heuristique.inl`.

---

## 0. Avant tout : où part réellement le temps ?

Aucune des propositions ci-dessous ne vaut quoi que ce soit sans mesure. Le budget
d'un nœud interne, dans l'état actuel du code, se décompose grosso modo ainsi :

| Poste | Ce qui est fait | Coût relatif estimé |
|---|---|---|
| Ordonnancement | `rawShapeScoreLight` + `upgradeLightToFull` + `addDefenseToOrderingScore` (= un `rawShapeScore` **complet** de plus) sur **tous** les candidats | **50–65 %** |
| Coups forcés | `restrictToForcedReplies` : `is_five_in_a_row` sur **tous** les candidats, à **tous** les nœuds | 10–15 % |
| Feuilles | `evaluateXPosition` + `bestThreatNear` (`(2R+1)²` appels à `rawShapeScore` complet) | 15–25 % |
| make/undo | `zoneApplyStone` (25 compteurs par pierre), copies de `MoveStateHash` | 5–8 % |
| TT + divers | | 5 % |

Le point commun de tout ça : **le vrai goulot n'est pas l'arbre, c'est le coût
unitaire d'une reconnaissance de motif**, et ce coût est dominé par les
**défauts de cache** sur les tables de motifs.

### La mesure à faire en premier

```
perf stat -e cycles,instructions,cache-misses,LLC-load-misses ./bench
perf record -g ./bench && perf report
```

Sur une position de milieu de partie, fixez la profondeur et relevez :
`_stats.nodesVisited / seconde`. **C'est la seule métrique qui compte pour ce
document** (le temps total dépend aussi de l'élagage, qui est traité en partie B).

**Protocole de non-régression** (indispensable pour « sans perdre en intelligence ») :

1. Un jeu de ~200 positions sauvegardées (ouverture / milieu / finale, avec captures).
2. Pour chaque position : profondeur fixe, on enregistre `bestMove` **et** `bestScore`.
3. Toute optimisation de la **partie A** doit donner **exactement** les mêmes
   `bestMove` et `bestScore`. C'est un test binaire, automatisable, non négociable.
4. Les optimisations de la **partie B** changent l'arbre : elles se valident par
   match automatique A/B (≥ 200 parties, temps égal) — pas par comparaison de coups.

---

# Partie A — Gains **exacts** (résultat de recherche strictement identique)

Ce sont les gains à faire en premier : zéro risque sur la force de jeu, le test
de non-régression est un `assert`.

## A1. Compacter les tables de motifs — **le plus gros gain à effort modéré**

### Le problème

Dans `BitboardTool`, chaque motif stocke un `Traits::Bitboard` **entier**, soit
48 octets sur 19×19 (6 limbes), alors qu'un motif de 5 cases occupe **au plus
2 limbes** (horizontal : 5 bits contigus ; vertical : 5×20 = 80 bits ; diagonale :
5×21 = 84 bits → toujours ≤ 2 mots de 64 bits). On stocke donc 4 à 5 limbes de
zéros par masque.

Empreinte actuelle (19×19), estimée depuis `PatternTypes.hpp` :

| Table | Taille |
|---|---|
| `_lt5[361][4]` | ~360 Ko |
| `_lt4[361][4]` | ~380 Ko |
| `_ltg4[361][4]` | ~1,2 Mo |
| `_lt3[361][4]` | ~1,2 Mo |
| `_lts4[361][4]` | ~640 Ko |
| `_ltcross[361]` | ~93 Ko |
| **Total** | **≈ 3,9 Mo** |

3,9 Mo, c'est **au-delà du L2** de la quasi-totalité des CPU, et l'accès est
indexé par `cell` — donc quasi aléatoire du point de vue du cache. Chaque
`check_*` paie des dizaines de cycles d'attente mémoire avant même de faire
un `AND`. Le code est déjà correct algorithmiquement (`firstWord`/`lastWord`
évitent les limbes inutiles au moment du test) — mais les limbes nuls sont
quand même **chargés dans le cache** puisqu'ils sont dans la même ligne.

### La correction

Remplacer le stockage `Bitboard` par une structure compacte à 2 limbes :

```cpp
struct CompactMask
{
    uint64_t m0, m1;   // limbes [w] et [w+1]
    uint8_t  w;        // index du premier limbe
    uint8_t  words;    // 1 ou 2
};

static inline bool match(const CompactMask& p, const typename Traits::Bitboard& b)
{
    if ((p.m0 & b[p.w]) != p.m0) return false;
    if (p.words == 2 && (p.m1 & b[p.w + 1]) != p.m1) return false;
    return true;
}
```

Et tant qu'à y être : `int stone_pos[4]`, `hole_pos[]`, `oposant_pos[5]`,
`opposant_left/right` → `int16_t` (les index plats vont de 0 à 380, et `-1`
tient dans un `int16_t`).

**Gain attendu :** empreinte ramenée à ~2,0 Mo, et surtout **densité** : les
masques réellement utiles deviennent contigus, on charge 2 à 4 lignes de cache
au lieu de 12 à 20 par appel. Sur un code borné par la mémoire, c'est
typiquement **–25 à –40 % de temps** sur les `check_*`.

**Risque :** nul. Même valeurs de retour, même ordre de tests. C'est purement
une réorganisation de la représentation.

**Effort :** 1 à 2 jours (il faut adapter `BitboardToolBuild.inl` et
`BitboardToolChecks.inl`).

---

## A2. Pré-filtre par direction : « il n'y a pas assez de pierres ici »

### Le principe

Un motif ne peut exister dans une direction que si un **nombre minimal de
pierres propres** est présent dans la fenêtre de 9 cases centrée sur le coup :

| Motif recherché | Pierres propres minimales dans la fenêtre (case jouée incluse) |
|---|---|
| cinq | 5 |
| open four / broken four / super four | 4 |
| open three | 3 |
| cross | 3 sur **deux** directions perpendiculaires |

Or dans une position réelle, **la majorité des directions autour d'un candidat
contient 0 ou 1 pierre propre**. On lance pourtant aujourd'hui la totalité des
comparaisons de masques dans les 4 directions.

### La correction

Précalculer, pour chaque case et chaque direction, le masque de la fenêtre de
9 (ou 11) cases — au format `CompactMask` de A1 :

```cpp
CompactMask WINDOW[Traits::CELL_COUNT][4];   // ~361*4*24 o = 35 Ko → tient en L1/L2
```

Puis, en tête de chaque `check_*` :

```cpp
inline int popWindow(const CompactMask& w, const typename Traits::Bitboard& b)
{
    int n = __builtin_popcountll(w.m0 & b[w.w]);
    if (w.words == 2) n += __builtin_popcountll(w.m1 & b[w.w + 1]);
    return n;
}

// dans check_open_three(own, opp, x, y) :
for (int d = 0; d < 4; ++d)
{
    if (popWindow(WINDOW[idx][d], own) < 3)   // ~4 cycles, aucun défaut de cache
        continue;                              // ← saute TOUTE la table _lt3[idx][d]
    ...
}
```

**Gain attendu :** dans une position typique, on élimine **60 à 80 % des
directions** avant de toucher les grosses tables. Combiné à A1, c'est le duo qui
peut à lui seul approcher le facteur 2 sur l'évaluation.

**Risque :** nul, *à condition que le seuil soit une borne inférieure stricte*.
Écrivez le test de non-régression exhaustif : pour 100 000 positions aléatoires
et toutes les cases, `check_X_avec_filtre(...) == check_X_sans_filtre(...)`.

**Effort :** 1 jour. C'est le meilleur rapport gain/effort du document.

---

## A3. Ordonnancement paresseux — ne plus scorer les coups qu'on ne jouera jamais

### Le problème

Aujourd'hui (déduit de la structure de `MasterAI`), à chaque nœud on :

1. calcule `rawShapeScoreLight` pour **les ~40 à 60 candidats** ;
2. les promeut avec `upgradeLightToFull` ;
3. ajoute la défense avec `addDefenseToOrderingScore`, qui appelle
   `rawShapeScore` **complet** (5 scans de motifs + `detect_capture_mask`) ;
4. trie tout ;
5. …et dans un nœud « cut » bien ordonné, **on coupe après le 1er ou 2e coup**.

Sur un arbre alpha-bêta correctement ordonné, ~90 % des nœuds internes sont des
nœuds cut, et la coupure arrive sur le 1er coup dans la grande majorité des cas.
**On calcule donc ~50 clés d'ordonnancement pour en utiliser 1 ou 2.**

### La correction : génération étagée + tri par sélection

```cpp
// Étage 0 : coup de la TT — aucun scoring, on l'essaie tel quel.
if (ttMove.valid && isPseudoLegal(ttMove)) { try(ttMove); if (cut) return; }

// Étage 1 : coups « forçants » seulement — captures et quatre.
//   detect_capture_mask est très bon marché (bit-tests) : on l'appelle sur tous.
//   check_open_four / broken_four seulement sur les cases qui passent
//   le pré-filtre A2 (>= 4 pierres) : en pratique une poignée de cases.

// Étage 2 : killers du ply courant — pas de scoring, juste vérification.

// Étage 3 : le reste — scoring complet, mais seulement si on arrive ici.
```

Et surtout : **remplacer le tri complet par un tri par sélection incrémental**.

```cpp
for (size_t i = 0; i < ordered.size(); ++i)
{
    size_t best = i;
    for (size_t j = i + 1; j < ordered.size(); ++j)
        if (ordered[j].score > ordered[best].score) best = j;
    std::swap(ordered[i], ordered[best]);

    ... recherche du coup ordered[i] ...
    if (score >= beta) break;     // on n'a trié que i+1 éléments
}
```

Un `std::sort` sur 50 éléments coûte ~300 comparaisons ; le tri par sélection
qui s'arrête au 2e élément en coûte ~100 et surtout **ne pré-calcule pas** ce
qui suit.

**Point d'attention :** l'ordre relatif final doit rester identique à celui de
`std::sort` pour garantir A-exactitude. Un tri par sélection n'est pas stable —
si deux coups ont le même score, il peut les intervertir par rapport à
`std::sort` (qui n'est pas stable non plus, mais différemment). Deux options :
(a) départager les ex æquo par index de case (`move.y * SIZE + move.x`), ce qui
rend l'ordre **total et déterministe** des deux côtés ; (b) accepter que le
test de non-régression porte sur `bestScore` seulement, pas sur `bestMove`.
Je recommande (a).

**Gain attendu :** **–40 à –55 % du coût d'ordonnancement**, soit ~25–30 % du
temps total. C'est le deuxième plus gros levier.

**Effort :** 2 à 3 jours (c'est une restructuration de la boucle de `minimax`).

### A3-bis. Défense approximée sur la queue de liste

`addDefenseToOrderingScore` ajoute `defense.score / 2`. Cette moitié sert à
**départager**, pas à décider. Deux allègements exacts-en-pratique :

- ne calculer la défense que pour les `K` premiers candidats par score
  d'offense (`K = 8` par exemple) — au-delà, l'ordre n'a plus d'effet mesurable ;
- utiliser `rawShapeScoreLight` + `upgradeLightToFull` pour la défense aussi,
  au lieu de `rawShapeScore` complet. Vous avez déjà démontré l'équivalence
  (`[Ordering] light+upgrade ≡ V2`) — pourquoi la défense n'en profite-t-elle pas ?
  **C'est une incohérence dans le code actuel** : le chemin rapide a été mis en
  place pour l'offense et pas pour la défense, qui reste sur le chemin lent.

Le second point est **exact** et se fait en une ligne. Faites-le tout de suite.

---

## A4. `restrictToForcedReplies` : conditionner au lieu de scanner

### Le problème

```cpp
bool isBlock[MAX_BOARD_MOVES<Traits>] = {};   // ~310 octets memset à chaque nœud
...
for (size_t i = 0; i < ordered.size(); ++i)   // is_five_in_a_row sur TOUS les candidats
```

Ce scan tourne à **tous les nœuds**, alors que la menace de cinq n'existe que
si l'adversaire possède déjà un quatre (ouvert, semi-ouvert ou brisé) non paré.

### La correction

L'information est **déjà calculée** : `EvaluatedMove::stage` du coup joué par le
parent vaut `Terminal`, `OpenFour`, `HalfFour` ou `BrokenFour` exactement quand
un quatre vient d'apparaître. Il suffit de la faire descendre :

```cpp
int minimax(SearchPosition<Traits>& pos, t_cell cell, int depth,
            int alpha, int beta, ShapeStage playedStage);
```

puis :

```cpp
const bool mayHaveFive = (playedStage == ShapeStage::OpenFour
                       || playedStage == ShapeStage::HalfFour
                       || playedStage == ShapeStage::BrokenFour
                       || playedStage == ShapeStage::Terminal);
if (mayHaveFive)
    restrictToForcedReplies(board, ordered, mover);
```

**Est-ce exact ?** Oui au sens qui compte : `restrictToForcedReplies` est un
**élagage**, pas une condition de correction. Ne pas l'appliquer ne change
jamais la valeur retournée par un alpha-bêta complet (on explore juste plus de
coups). Le seul risque est de **rater une occasion d'élaguer** quand un quatre
survit d'un ply à l'autre (quatre double, ou quatre créé plus tôt et jamais paré).
Or si l'adversaire a un quatre non paré, il joue le cinq au nœud suivant et
la branche se termine immédiatement de toute façon.

Si vous voulez la version **strictement identique**, gardez le scan mais
pré-filtrez avec A2 : `is_five_in_a_row` ne peut réussir que si la fenêtre de la
case contient ≥ 4 pierres adverses dans une direction — un `popcount` élimine
99 % des candidats sans toucher `_lt5`.

Accessoirement : remplacez `bool isBlock[MAX_BOARD_MOVES]` par un
`uint64_t isBlock[(MAX_BOARD_MOVES + 63) / 64]` — 40 octets au lieu de 310,
et le `memset` disparaît du profil.

**Gain attendu :** 8 à 13 % du temps total.

**Effort :** quelques heures.

---

## A5. Éliminer les copies inutiles sur le chemin chaud

Petits gains, effort quasi nul, aucun risque :

**`evaluatePosition`** (`heuristique.inl`) :
```cpp
t_BWBoard<Traits> board = position.board();   // copie 96 octets, jamais modifiée
```
→ `const auto& board = position.board();`. Et si `resolveCaptures` exige une
référence non-const alors qu'elle ne modifie rien (le commentaire dit
« ne modifie pas l'etat du board »), corrigez la signature en `const&`.
**Cette fonction semble par ailleurs morte** (`evaluateLeafPosition` est le
point d'entrée) : vérifiez, et supprimez-la si c'est le cas.

**`SearchPosition::makeMove(int, int, Color, MoveStateHash stateHash)`** :
`MoveStateHash` contient un `MoveState` avec un `MoveList<t_cell, 16>`, soit
~80 octets, **passés par valeur**. `buildMoveHash` le retourne aussi par valeur.
→ passer `const MoveStateHash&`. Deux copies de 80 octets économisées par coup joué,
à chaque nœud de l'arbre.

**`bestThreatNear`** : `const auto zone = position.candidateMask();` reconstruit
un bitboard complet à chaque feuille scannée. Exposez plutôt
`const Bitboard& zoneMask() const` dans `SearchPosition` et faites le
`& ~occupancy` dans la boucle (ou mieux : le masque candidat est déjà calculé
par le parent — passez-le).

**`rawShapeScore*`** : `Bitboard own = board.black;` copie 48 octets **par
candidat, deux fois** (offense + défense). Alternative sans copie : poser le bit
dans le bitboard du plateau, appeler les `check_*`, puis retirer le bit
(set/clear = 2 instructions). Le plateau n'est plus `const` mais l'invariant est
restauré avant retour — c'est le pattern make/unmake standard.

**Taille des frames de pile** : `MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits>>`
avec `MAX_BOARD_MOVES<19> = 310` et un `EvaluatedMove` de ~16 octets → **~5 Ko
par frame**. À profondeur 10, 50 Ko de pile active, ce qui évince tout le reste
du L1d. Or la zone active borne le nombre réel de candidats bien en dessous de
310. Deux options : (a) réduire la borne à une constante réaliste (`ZONE_MAX = 128`)
avec un `assert` ; (b) allouer **une seule fois** un tableau `[MAX_PLY][ZONE_MAX]`
dans `MasterAI` et donner à chaque ply une vue dessus. L'option (b) est la bonne :
c'est ce que font tous les moteurs d'échecs, et le gain sur les défauts de L1
est réel (5 à 10 %).

---

## A6. Zone active : `OR` + pile de restauration au lieu de 25 compteurs

### Le problème

`zoneApplyStone` fait **(2R+1)² = 25 incréments** de `_zoneCount` + tests de
transition, à chaque pose **et** à chaque retrait — donc **50 par make/undo**,
plus 50 par pierre capturée.

### La correction

Précalculer `NEIGHBOR[cell]` (bitboard des 25 voisins, ~17 Ko compacté) et :

```cpp
// make
_zoneStack[ply] = _zoneMask;                 // sauvegarde 48 octets
for (int i = 0; i < WORD_COUNT; ++i)
    _zoneMask[i] |= NEIGHBOR[cell][i];       // 6 OR
// undo
_zoneMask = _zoneStack[ply];                 // restauration exacte
```

6 `OR` + une copie de 48 octets, contre 25 incréments dispersés. Et `_zoneCount`
(380 octets touchés aléatoirement) **disparaît complètement**.

**Différence de comportement :** avec l'`OR`, la zone ne rétrécit plus quand une
capture retire des pierres à l'intérieur d'une branche. Le masque devient donc un
**sur-ensemble** de l'actuel → quelques candidats supplémentaires, jamais moins.
Ce n'est pas exact au sens strict. Si vous tenez à l'exactitude, gardez les
compteurs mais remplacez la double boucle `dx/dy` par une itération sur
`NEIGHBOR[cell]` via `bb_for_each_bit` — vous gagnez déjà les tests
`in_board_generic` et le calcul d'index.

**Gain attendu :** 3 à 6 % du temps total.

---

## A7. Table de transposition : récupérer les hits qu'on perd aujourd'hui

### Un problème de cohérence du hash des captures

Dans `fromBoard` :
```cpp
for (int p = 1; p <= whiteVictimPairs; ++p)
    hash ^= h.captureHash(Color::White, p);      // XOR de TOUTES les paires 1..N
```

Dans `buildMoveHash` :
```cpp
stateHash.hash ^= _hasher.captureHash(Color::White, (_whiteCaptures + caps) >> 1);
//                                                   ↑ une SEULE clé, la nouvelle
```

Si un coup capture **deux paires d'un coup** (`caps == 4`, ce qui arrive), le
compteur passe de `n` à `n+2` paires mais on ne XOR que la clé de `n+2` —
la clé de `n+1` n'est jamais insérée. Le hash obtenu diffère alors de celui d'un
chemin qui aurait pris les deux paires en deux coups, **et** de la convention de
`fromBoard`. Résultat : des positions identiques reçoivent des hashes différents
→ **transpositions manquées**, donc arbre plus gros. Ce n'est pas un bug de
correction (pas de fausse collision), mais c'est une perte de performance
silencieuse.

Correction : XOR de toutes les clés intermédiaires dans `buildMoveHash`, et
symétriquement dans `undoMove` (qui souffre du même problème : il XOR
`captureHash(couleur, _whiteCaptures >> 1)` une seule fois).

### Autres améliorations TT

- **Prefetch** : dès que `stateHash.hash` est connu, avant `makeMove` :
  `__builtin_prefetch(&_tt.bucket(stateHash.hash));`. Le lookup arrive ~50 cycles
  plus tard → la ligne est déjà là. Gain typique 3–5 %, gratuit.
- **Buckets de 4 entrées, 16 octets par entrée** (clé 32 bits de vérification,
  score, coup 16 bits, profondeur 8 bits, borne 2 bits, âge). Un bucket = une
  ligne de cache de 64 octets = **un seul accès mémoire**.
- **Remplacement à deux niveaux** (always-replace + depth-preferred) plutôt que
  depth-only : meilleur taux de réutilisation en fin de partie.
- **Stocker l'évaluation statique** dans l'entrée : évite de relancer
  `evaluateLeafPosition` sur les nœuds re-visités.

Vos compteurs `ttHits` / `ttCutoffs` / `ttStores` sont déjà en place — servez-vous-en :
si `ttCutoffs / nodesVisited < 5 %`, la TT ne travaille pas et le problème de
hash ci-dessus en est probablement la cause.

---

## A8. Compilation — le gain gratuit qu'on oublie toujours

```
-O3 -march=native -mtune=native -flto -fno-exceptions -fno-rtti
```

et surtout **PGO** :

```bash
g++ -O3 -march=native -fprofile-generate ...   # build instrumenté
./bench_positions                               # exécution représentative
g++ -O3 -march=native -fprofile-use -flto ...  # build final
```

Sur un moteur de recherche (branches très prédictibles une fois profilées,
inlining agressif des `check_*`), le PGO donne classiquement **+10 à 20 %**.
C'est une demi-journée de travail sur le Makefile, zéro risque, zéro changement
de comportement.

Vérifiez aussi que `BitboardTool::instance()` est bien inliné : c'est une
`static` locale, donc protégée par un garde thread-safe (`__cxa_guard_acquire`)
à **chaque appel** si le compilateur ne prouve pas l'initialisation. Sur un
appel par candidat par nœud, ça se voit. Remplacez par un objet global initialisé
avant la recherche, ou passez la référence en paramètre.

---

## Récapitulatif de la partie A

| # | Optimisation | Gain estimé | Risque | Effort |
|---|---|---|---|---|
| A1 | Masques compacts (2 limbes) | 20–30 % | nul | 1–2 j |
| A2 | Pré-filtre popcount par direction | 20–30 % | nul | 1 j |
| A3 | Ordonnancement étagé + tri par sélection | 25–30 % | nul* | 2–3 j |
| A3b | Défense via light+upgrade et top-K | 5–8 % | nul | 1 h |
| A4 | `restrictToForcedReplies` conditionné | 8–13 % | nul | qq h |
| A5 | Suppression des copies, frames de pile | 5–10 % | nul | 1 j |
| A6 | Zone active par OR + pile | 3–6 % | nul* | qq h |
| A7 | TT : hash captures + prefetch + buckets | 5–15 % | nul | 1 j |
| A8 | LTO + PGO + `instance()` | 10–20 % | nul | ½ j |

*\* voir les réserves sur la stabilité du tri (A3) et le sur-ensemble de zone (A6).*

Les gains ne s'additionnent pas linéairement (A2 réduit ce que A1 accélère), mais
**A1 + A2 + A3 + A8 seuls devraient déjà dépasser le facteur 2** sur les
nœuds/seconde. C'est là que se trouvent vos 50 %.

---

# Partie B — Moins de nœuds pour la même qualité

Ces optimisations changent l'arbre exploré. Elles ne se valident pas par
comparaison de coups mais par **match automatique**. Bien réglées, elles
**augmentent** la force à temps égal.

## B1. PVS / NegaScout

Une fois l'ordonnancement bon (ce qu'il est déjà chez vous : TT + killers +
history + score de forme), le premier coup est le meilleur dans ~90 % des cas.
Les suivants n'ont donc qu'à **prouver qu'ils sont moins bons** — une fenêtre
nulle suffit.

```cpp
if (first)
    score = -minimax(pos, m, depth - 1, -beta, -alpha);
else {
    score = -minimax(pos, m, depth - 1, -alpha - 1, -alpha);   // scout
    if (score > alpha && score < beta)                          // rare
        score = -minimax(pos, m, depth - 1, -beta, -alpha);     // re-search
}
```

**Gain : –15 à –30 % de nœuds. Résultat de la recherche strictement identique**
(PVS est exact, à condition que l'évaluation soit déterministe — c'est le cas ici).
C'est le meilleur rapport gain/risque de toute la partie B.

**Prérequis :** que votre `minimax` soit en forme negamax. S'il est en
min/max explicite avec `_aiColor`, la conversion vaut le coup — elle divise
aussi par deux le code à maintenir.

## B2. Approfondissement itératif + fenêtre d'aspiration

Si ce n'est pas déjà fait (`ttRootExactSeeds` suggère que la TT est amorcée à la
racine, mais pas forcément par de l'ID) :

```cpp
for (int d = 1; d <= _maxDepth; ++d)
{
    score = search(d, score - WINDOW, score + WINDOW);
    if (score <= alpha || score >= beta)
        score = search(d, -INF, +INF);       // re-search fenêtre large
}
```

Contre-intuitivement, refaire les profondeurs 1..N-1 **fait gagner du temps**,
parce que la TT et les killers issus de la profondeur `d-1` ordonnent
parfaitement la profondeur `d`. Gain typique **–30 à –50 % de nœuds** à
profondeur égale. Et bonus : ça donne la gestion du temps gratuitement
(on s'arrête quand le budget est épuisé, avec un coup valide toujours disponible).

## B3. Counter-move heuristic

Vous avez killers + history. Ajoutez la table `_counter[camp][caseDuCoupPrécédent]`
= dernier coup ayant produit une coupure en réponse à ce coup-là. Coût mémoire :
`2 × 361 × sizeof(t_cell)` ≈ 6 Ko. Gain : quelques % de nœuds, et c'est ~30 lignes.

## B4. Remplacer `bestThreatNear` par une quiescence de menaces

`bestThreatNear` scanne `(2R+1)²` cases avec un `rawShapeScore` complet à chaque
feuille qui dépasse le seuil. C'est un **scan statique coûteux qui approxime ce
qu'une recherche ferait mieux et moins cher**.

Une quiescence restreinte aux coups forçants (captures, quatre, et seulement
ceux-là) explore typiquement 2 à 5 nœuds là où le scan en évalue 49, et donne un
horizon **exact** au lieu d'une heuristique. Avec `LEAF_SCAN_RADIUS = 3`, vous
faites 49 `rawShapeScore` complets par feuille — c'est plus cher qu'un nœud interne.

Si vous préférez garder le scan, deux allègements immédiats :
- utiliser `rawShapeScoreLight` (pas `rawShapeScore`) et n'upgrader que le meilleur ;
- appliquer le pré-filtre A2 : une case sans pierre propre dans son voisinage de
  ligne ne peut rien créer → `score = 0` sans aucun accès aux tables. Cela élimine
  typiquement 70 % des 49 cases ;
- parcourir en **spirale** depuis `anchor` pour déclencher `THREAT_SCAN_EARLY_EXIT`
  plus tôt (les grosses menaces sont près du dernier coup).

## B5. Late Move Reductions — puissant mais à manier avec précaution

C'est le levier qui vaut un facteur 2 à 3 supplémentaire dans les moteurs
modernes. Le principe : après les `N` premiers coups d'un nœud, réduire la
profondeur de 1 (ou plus), et **re-chercher à pleine profondeur uniquement si
la recherche réduite dépasse alpha**.

```cpp
int R = 0;
if (moveIndex >= 4 && depth >= 3 && !isForcing(m) && !isKillerMove(ply, m))
    R = 1 + (moveIndex > 12 && depth >= 6);

score = -minimax(pos, m, depth - 1 - R, -alpha - 1, -alpha);
if (R && score > alpha)
    score = -minimax(pos, m, depth - 1, -beta, -alpha);   // re-search
```

**`isForcing` doit être strict au Gomoku/Pente** — jamais réduire :
un coup qui crée un cinq, un quatre (ouvert / semi / brisé), un double-trois,
une capture, une parade de cinq adverse, ni un coup issu de la TT.
Votre champ `ShapeStage` donne exactement ce prédicat gratuitement.

**Validation obligatoire par match A/B.** Si la version LMR perd, c'est presque
toujours que `isForcing` laisse passer quelque chose.

## B6. Null-move pruning — cas particulier favorable ici

Aux échecs, le null move est risqué (zugzwang). **Au Gomoku, le zugzwang n'existe
pas** : passer son tour est toujours au moins aussi mauvais que jouer. Le null
move y est donc particulièrement sûr :

```cpp
if (depth >= 3 && !inThreat && !mayHaveFive)
{
    pos.passTurn();
    int nullScore = -minimax(pos, lastCell, depth - 1 - 2, -beta, -beta + 1);
    pos.unpassTurn();
    if (nullScore >= beta) return nullScore;   // même en passant, on est trop bon
}
```

Il faut désactiver le null move quand l'adversaire a une menace immédiate
(`mayHaveFive` de A4) et quand un camp est à une paire de la victoire par
capture — sinon on rate des mats. Gain typique **–20 à –35 % de nœuds**.

## B7. Recherche de gain forcé (VCF) à la racine

Avant toute recherche, lancer une recherche restreinte aux quatre et captures
(profondeur 10–15, facteur de branchement 2–4) : si un gain forcé existe, on le
trouve en quelques millisecondes et on retourne immédiatement. Cela ne change pas
la force en dehors de ces positions, et fait chuter le temps sur les positions
gagnantes — qui sont fréquentes en fin de partie.

---

# Partie C — Parallélisme

Si les parties A et B ne suffisent pas, c'est la voie la plus directe vers un
facteur 2–6 supplémentaire.

- **Lazy SMP** : `T` threads cherchent la même position à des profondeurs
  légèrement décalées, en **partageant la TT** (entrées lock-free, XOR du hash
  avec les données pour détecter les écritures déchirées). ~200 lignes, scaling
  effectif ×2,5 à ×3 sur 4 cœurs.
- **Root splitting** : plus simple, plus déterministe, mais scaling limité
  (~×1,8 sur 4 cœurs) car la fenêtre alpha-bêta n'est pas partagée efficacement.

**Prérequis à traiter d'abord :** `BitboardTool::instance()` est un singleton
partagé — il est en lecture seule après construction, donc thread-safe, mais il
faut le construire **avant** de lancer les threads. Et `_killers` / `_history` /
`_stats` doivent devenir par thread.

Notez que le parallélisme ne dispense pas de A : un moteur mal optimisé
parallélisé reste un moteur mal optimisé, et les défauts de cache s'aggravent en
multi-thread (bande passante mémoire partagée).

---

# Partie D — La refonte : extraction de fenêtre par PEXT

Si après A + B + C il faut encore gagner, c'est ici que se trouve le facteur 3–5
restant sur l'évaluation. C'est aussi un gros chantier (2–3 semaines) : à ne
lancer que si les mesures le justifient.

## Le principe

Aujourd'hui, reconnaître un motif = comparer des dizaines de masques de 48 octets.
L'approche standard des moteurs de Gomoku forts : **encoder la fenêtre locale en
un entier, et lire le verdict dans une table**.

Votre layout (`STRIDE = 20`) rend les fenêtres non contiguës en vertical et en
diagonale — d'où, classiquement, des « bitboards pivotés ». **Mais BMI2 rend
cela inutile** : `_pext_u64` extrait et compacte n'importe quel sous-ensemble de
bits en une instruction (3 cycles).

```cpp
// Fenêtre de 11 cases centrée sur (x,y) dans la direction d.
// PEXT_MASK[cell][d] = { m0, m1, w, shift } précalculés (~35 Ko, résident L1/L2).

inline uint32_t window(const typename Traits::Bitboard& bb, const PextMask& p)
{
    uint32_t lo = (uint32_t)_pext_u64(bb[p.w], p.m0);
    uint32_t hi = (uint32_t)_pext_u64(bb[p.w + 1], p.m1);
    return lo | (hi << p.shift);              // 11 bits
}

const uint32_t o = window(own, PEXT_MASK[cell][d]);   // 11 bits, pierres propres
const uint32_t e = window(opp, PEXT_MASK[cell][d]);   // 11 bits, pierres adverses
const uint8_t  verdict = PATTERN_TABLE[ternary(o, e)];
```

`own` et `opp` étant disjoints, l'état d'une case est ternaire : la fenêtre de 11
cases se code sur `3^11 = 177 147` entrées d'un octet = **173 Ko**. La conversion
(o, e) → index ternaire se fait par deux petites tables :

```cpp
// T6[o6 | e6<<6]  → 4096 entrées uint16 (8 Ko), valeur dans [0, 729)
// T5[o5 | e5<<5]  → 1024 entrées uint16 (2 Ko), valeur dans [0, 243)
const uint32_t idx = T6[(o & 63) | ((e & 63) << 6)] * 243u
                   + T5[(o >> 6)  | ((e >> 6) << 5)];
```

Soit **4 PEXT + 3 lectures de table** par direction, contre plusieurs dizaines de
comparaisons de masques dispersés dans 3,9 Mo aujourd'hui.

## Ce que ça implique

- La table par direction donne un **code de motif directionnel**. Les motifs
  multi-directions (`check_cross`, double-trois) se reconstruisent par une petite
  couche de combinaison sur les 4 codes — c'est **la vraie difficulté du chantier**,
  car votre taxonomie (`SCORE_*`, `CROSS_*` dans `PatternTypes.hpp`) est riche.
  Le bon plan : générer `PATTERN_TABLE` **en réutilisant le code actuel** comme
  oracle (pour chaque configuration de 11 cases, construire un mini-plateau et
  appeler `check_*` existant) — la table devient ainsi exacte par construction, et
  vous obtenez un test d'équivalence gratuit.
- **Portabilité** : `_pext_u64` exige BMI2 (Intel Haswell+, AMD Zen3+). Sur Zen1/Zen2
  PEXT est micro-codé et **très lent** (~18 cycles) — prévoir un fallback
  (bitboards pivotés, ou boucle de bits) sélectionné à l'exécution via `__builtin_cpu_supports("bmi2")`.
  À vérifier impérativement si le projet doit tourner sur une machine imposée.
- Si 173 Ko vous gêne, une fenêtre de 9 cases (`3^9 = 19 683` entrées = **19 Ko,
  résident L1**) suffit pour cinq / quatre / trois, avec quelques tests de bits
  supplémentaires pour l'ouverture aux extrémités.

---

# Plan de mise en œuvre recommandé

**Semaine 1 — le socle mesurable**
1. Harnais de bench (200 positions, `nodes/s`, `bestMove` + `bestScore` en référence).
2. A8 (LTO + PGO + `instance()`), A5 (copies), A3b (défense light). Gains immédiats, risque nul.
3. Mesure. Vous devriez déjà être entre +20 et +35 %.

**Semaine 2 — le cœur**
4. A2 (pré-filtre popcount) puis A1 (masques compacts) — dans cet ordre, A2 étant
   plus rapide à écrire et réduisant l'exposition au problème que A1 corrige.
5. Mesure. Cumul attendu : ×1,8 à ×2,2.

**Semaine 3 — l'arbre**
6. A4 (`restrictToForcedReplies`), A7 (TT), A3 (ordonnancement étagé).
7. B1 (PVS) et B2 (ID + aspiration) : exacts ou quasi, gros gain en nœuds.
8. Mesure. Objectif largement dépassé.

**Ensuite, selon le besoin**
9. B4 (quiescence de menaces), B5 (LMR), B6 (null move) — chacun validé par match A/B de 200 parties.
10. C (Lazy SMP) si le multi-thread est autorisé.
11. D (PEXT) seulement si le profil montre que les `check_*` dominent encore.

---

# Trois choses à corriger au passage (indépendantes de la performance)

1. **`SearchPosition::detect_and_hash_capture`** écrit dans
   `stateHash.state.capturedStones[captured]` **sans mettre à jour le compteur**
   de la `MoveList` (contrairement à `buildMoveHash` qui utilise `push`). Si cette
   fonction est encore appelée quelque part, `undoMove` restaurera un nombre de
   pierres incohérent. À vérifier — et à supprimer si elle est morte.

2. **`_history` est un `MoveList<MoveState, DEPTH>`** alors que `_killers` est
   dimensionné à `MAX_SEARCH_PLY = 64`. Dès que vous ajouterez des extensions ou
   une quiescence (B4), la recherche dépassera `DEPTH` plies et `_history`
   débordera silencieusement. Alignez les deux bornes maintenant.

3. **Hash des captures multiples** (détaillé en A7) : les clés de paires
   intermédiaires ne sont jamais insérées quand un coup prend deux paires.
   C'est à la fois une perte de hits TT et une incohérence avec `fromBoard`.

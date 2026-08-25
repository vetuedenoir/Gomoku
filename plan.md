# Plan d'exécution — Partie A (gains exacts)

**Règle qui gouverne tout le plan :** chaque lot de la partie A doit produire,
sur le jeu de positions de référence, **exactement le même `bestMove` et le même
`bestScore`** qu'avant. Si ce n'est pas le cas, c'est un bug — pas un compromis
à accepter. C'est ce qui rend le « sans perdre en intelligence » vérifiable
plutôt que promis.

Ordre des lots choisi selon trois critères : dépendances techniques, gain/effort,
et « ne jamais refactorer avant de pouvoir mesurer ».

---

### 0 — Exécutable de bench

```
./bench --depth 8 --positions bench/positions.txt --out ref.json
```

Pour chaque position, écrire : `bestMove`, `bestScore`, et l'intégralité de
`SearchStats` (vous l'avez déjà : `nodesVisited`, `nodesEvaluated`, `nodesPruned`,
`ttHits`, `ttCutoffs`, `ttStores`, `forcedNodes`) + le temps en microsecondes.

**Profondeur fixe, pas de limite de temps** — sinon la référence n'est pas
reproductible.

### 0.3 — Comparateur

```
./bench --compare ref.json new.json
```

Sortie attendue :

```
Exactitude   : 200/200 identiques (bestMove + bestScore)     ✓
Nœuds        : 4 812 993 → 4 812 993   (identique)           ✓
Temps        : 12.44 s → 9.71 s        (-21.9 %)
Nœuds/s      : 386 k → 495 k           (+28.4 %)
```

La ligne « Nœuds » est le garde-fou clé : en partie A, le **nombre de nœuds
doit rester rigoureusement identique**. S'il bouge, vous avez changé l'arbre
sans le vouloir — donc changé la force de jeu.

### 0.4 — Baseline

Enregistrer `ref.json` sur la branche actuelle, le commiter, et **ne plus jamais
le régénérer** jusqu'à la fin de la partie A.

---

## Lot 1 — Les gains gratuits (½ à 1 jour)

Aucun changement structurel. À faire d'un bloc, mesurer à la fin.

### 1.1 — Chaîne de compilation

`Makefile` : `-O3 -march=native -mtune=native -flto -fno-exceptions -fno-rtti`,
puis cible PGO :

```make
pgo:
	$(MAKE) CXXFLAGS="$(BASE) -fprofile-generate" clean all
	./bench --depth 8 --positions bench/positions.txt --out /dev/null
	$(MAKE) CXXFLAGS="$(BASE) -fprofile-use -fprofile-correction" clean all
```

*Piège :* `-march=native` casse la portabilité du binaire. Si le projet doit
tourner sur une machine d'évaluation imposée, utilisez `-mavx2 -mbmi -mbmi2`
explicitement, ou une détection à l'exécution.

### 1.2 — Le singleton `BitboardTool::instance()`

`static BitboardTool tool;` dans une fonction impose un garde thread-safe
(`__cxa_guard_acquire`) que le compilateur ne peut pas toujours éliminer. Or
`instance()` est appelé **une fois par candidat, par nœud**.

Remplacement : construire l'outil une fois dans le constructeur de `MasterAI`,
le stocker en membre (`BitboardTool<Traits>& _tool;` initialisé depuis un
objet global), et **passer la référence** aux fonctions de `heuristique.inl` au
lieu de rappeler `instance()` dans chacune (`rawShapeScore`, `rawShapeScoreV2`,
`rawShapeScoreLight`, `upgradeLightToFull`, `restrictToForcedReplies`,
`bestThreatNear`, `evaluatePosition`, `evaluateBlackPosition`, `evaluateWhitePosition`).

*Vérification :* `perf report` ne doit plus montrer `__cxa_guard_acquire`.

### 1.3 — Copies sur le chemin chaud

| Fichier | Ligne | Correction |
|---|---|---|
| `heuristique.inl` | `t_BWBoard<Traits> board = position.board();` dans `evaluatePosition` | `const auto& board = ...` (et rendre `resolveCaptures` const, elle ne modifie rien) |
| `SearchPosition.hpp` | `makeMove(int, int, Color, MoveStateHash stateHash)` | `const MoveStateHash&` |
| `SearchPosition.hpp` | `buildMoveHash` retourne par valeur | acceptable si NRVO ; sinon sortie par référence |
| `heuristique.inl` | `const auto zone = position.candidateMask();` dans `bestThreatNear` | exposer `const Bitboard& zoneMask()` et faire le `& ~occ` dans la boucle |

### 1.4 — Défense par le chemin rapide

Dans `addDefenseToOrderingScore` :

```cpp
// avant
const EvaluatedMove defense = rawShapeScore(board, offense.move, opp, oppCapturesBefore);
// après
EvaluatedMove defense = rawShapeScoreLight(board, offense.move, opp, oppCapturesBefore);
upgradeLightToFull(defense, board, opp, oppCapturesBefore);
```

Vous avez déjà verrouillé l'équivalence (`[Ordering] light+upgrade ≡ V2`). Reste
à vérifier que `rawShapeScore` et `rawShapeScoreV2` sont bien identiques —
ils le paraissent, à la mise à jour de `data.stage` près dans la branche
`>= 10 captures`, que `rawShapeScore` **omet**. Corrigez cette omission d'abord,
sinon les deux chemins divergent sur `stage` et l'`upgrade` se comportera
différemment.

### 1.5 — Code mort

`evaluatePosition` semble supplantée par `evaluateLeafPosition`. Vérifiez avec
`grep -rn "evaluatePosition"` : si seul le prototype et la définition ressortent,
supprimez-la. Idem pour `rawShapeScoreV2` si seul le test d'équivalence l'appelle
(gardez-la alors, mais sous `#ifdef TESTS`).

**Critère d'acceptation du lot 1 :** 200/200 identiques, nœuds identiques,
**+15 à +30 % de nœuds/s**.

---

## Lot 2 — Pré-filtre par direction (1 à 1,5 jour)

Le meilleur rapport gain/effort du plan. Il introduit aussi l'infrastructure
`CompactMask` dont le lot 3 se sert — d'où cet ordre.

### 2.1 — Le type compact

Nouveau fichier `bitboard/CompactMask.hpp` :

```cpp
template<typename Traits>
struct CompactMask
{
    static constexpr int MAX_WORDS = 4; // ±4 vert/diag 19×19 : jusqu'à 4 limbes
    uint64_t m[MAX_WORDS] = {};
    uint8_t  w = 0;        // index du premier limbe
    uint8_t  words = 1;    // 1 .. MAX_WORDS

    static CompactMask from(const typename Traits::Bitboard& bb);  // pour la génération
};

template<typename Traits>
inline int popWindow(const CompactMask<Traits>& p, const typename Traits::Bitboard& b)
{
    int n = 0;
    for (uint8_t i = 0; i < p.words; ++i)
        n += __builtin_popcountll(p.m[i] & b[p.w + i]);
    return n;
}
```

*Invariant à vérifier à la génération :* une fenêtre ±4 (9 cases) tient dans
`MAX_WORDS` limbes. Sur 19×19 le pire cas est la diagonale (`8 × STRIDE_D`
peut traverser 4 words) — d'où `MAX_WORDS = 4`, pas 2. Les motifs 5-cases
seuls restent souvent ≤ 2 limbes ; le plafond 4 couvre aussi les fenêtres
de préfiltre. `assert(span <= MAX_WORDS)` dans `CompactMask::from`.

### 2.2 — Tables de fenêtres

Dans `BitboardTool`, ajouter `CompactMask<Traits> _window[Traits::CELL_COUNT][4]`
et le générer dans `buildAll()` : pour la case `(x,y)` et la direction `d`,
l'union des cases `(x + k·dx, y + k·dy)` pour `k ∈ [-4, 4]`, bornée au plateau.

Empreinte : `361 × 4 × 24 o ≈ 35 Ko` → résident L2, souvent L1.

### 2.3 — Câblage dans les `check_*`

Dans `BitboardToolChecks.inl`, en tête de chaque boucle de direction :

| Fonction | Seuil sur `popWindow(_window[idx][d], stones)` |
|---|---|
| `is_five_in_a_row` | `< 5` → `continue` |
| `check_open_four` | `< 4` → `continue` |
| `check_broken_four` | `< 4` → `continue` |
| `check_super_four` | `< 4` → `continue` |
| `check_open_three` | `< 3` → `continue` |
| `check_cross` | `< 3` sur la direction → écarter cette branche du cross |

**Attention à la case jouée.** Ces `check_*` sont appelés avec un `own` où la
pierre hypothétique **est déjà posée** (`set_bb_generic` dans `rawShapeScore*`).
Les seuils ci-dessus comptent donc la case centrale. Vérifiez ce point sur
chaque appelant avant de figer les valeurs — c'est la seule source d'erreur
sérieuse de ce lot.

**`check_cross` est le cas délicat :** le motif combine plusieurs directions et
`_ltcross` est indexé par case, pas par direction. Si le filtrage par direction
n'y est pas naturel, appliquez un filtre global (`popcount(own ∩ voisinage
complet) < 5` → `return CROSS_NONE`) et laissez le reste tel quel. Le gros du
gain est ailleurs.

### 2.4 — Test d'équivalence exhaustif

C'est le test le plus important du plan :

```cpp
// 100 000 tirages : plateau aléatoire (densité 5 à 40 %), case aléatoire vide.
for (auto& [own, opp, x, y] : randomCases)
{
    assert(check_open_three_filtered(own, opp, x, y)
        == check_open_three_reference(own, opp, x, y));
    // ... idem pour les 5 autres
}
```

Gardez les versions `_reference` (non filtrées) sous `#ifdef TESTS` : elles
resserviront au lot 3.

**Critère d'acceptation :** 200/200, nœuds identiques, **+20 à +30 % de nœuds/s**
supplémentaires.

---

## Lot 3 — Compaction des tables de motifs (1,5 à 2 jours) ✅

*Fait : `CompactMask` dans les tables + `int16_t` indices + `matchPattern(CompactMask)`.
Mesure post-lot-2 : nœuds identiques, NPS ~bruit (±2 %) — le préfiltre a déjà
coupé la majorité des accès table. Gain principal = empreinte mémoire / cache.*


### 3.1 — Migration des structures

Dans `PatternTypes.hpp`, remplacer chaque `typename Traits::Bitboard` par
`CompactMask<Traits>` et supprimer les `firstWord`/`lastWord` devenus redondants
(l'information est dans `CompactMask::w` / `words`) :

| Structure | Avant | Après |
|---|---|---|
| `t_PatternList5` | `Bitboard masks[5]` + `firstWord[5]` + `lastWord[5]` | `CompactMask masks[5]` |
| `t_Pattern4` | `Bitboard mask` + 2 spans | `CompactMask mask` |
| `t_PatternGroup4` | `Bitboard masks[3]` + 2×`[3]` | `CompactMask masks[3]` |
| `t_PatternGroupe3` | 3 × (`Bitboard` + 2 spans) | 3 × `CompactMask` |
| `t_super4` | `Bitboard mask` + 2 spans | `CompactMask mask` |

Passer au même moment `int stone_pos[4]`, `hole_pos[]`, `oposant_pos[5]`,
`opposant_left/right`, `hole_pos[3]` en `int16_t` : les index plats vont de 0 à
380 et le sentinelle `-1` y tient. Attention à `get_bb_flate` qui reçoit ces
valeurs — la garde `idx == -1` doit rester en amont.

Empreinte attendue : **3,9 Mo → ~2,0 Mo**.

### 3.2 — Adapter `BitboardToolBuild.inl`

Les fonctions de construction manipulent des `Bitboard` pleins. La conversion se
fait au dernier moment :

```cpp
typename Traits::Bitboard tmp = {};
// ... remplissage inchangé ...
pattern.mask = CompactMask<Traits>::from(tmp);   // + assert(span <= 2)
```

C'est le point où le refactor reste peu risqué : la logique de génération n'est
pas touchée, seulement le stockage final.

### 3.3 — Adapter `matchPattern`

```cpp
static inline bool matchPattern(const CompactMask<Traits>& p,
                                const typename Traits::Bitboard& b)
{
    if ((p.m0 & b[p.w]) != p.m0) return false;
    if (p.words == 2 && (p.m1 & b[p.w + 1]) != p.m1) return false;
    return true;
}
```

La surcharge de repli (celle qui balaie tous les limbes) devient inutile —
supprimez-la pour éviter qu'un appel oublié ne retombe dessus silencieusement.

### 3.4 — Validation

Le test exhaustif du lot 2 rejoue tel quel : les `_reference` non compactées
servent d'oracle.

**Critère d'acceptation :** 200/200, nœuds identiques, **+10 à +20 %**
supplémentaires (moins que les 25–40 % annoncés dans le document initial,
parce que le lot 2 a déjà supprimé la majorité des accès à ces tables — c'est
attendu, et c'est pour ça qu'il fallait mesurer entre les deux).

---

## Lot 4 — `restrictToForcedReplies` et zone active (1 jour)

### 4.1 — Pré-filtre sur le scan de cinq

Version **strictement exacte**, à préférer pour rester dans la logique de la
partie A :

```cpp
for (size_t i = 0; i < ordered.size(); ++i)
{
    const t_cell& m = ordered[i].move;
    const int idx = index_bb_generic<Traits>(m.x, m.y);

    bool possible = false;
    for (int d = 0; d < 4 && !possible; ++d)
        possible = popWindow(tool.window(idx, d), oppStones) >= 4;
    if (!possible) continue;          // ← élimine ~99 % des candidats

    set_bb_generic<Traits>(opp, m.x, m.y);
    if (tool.is_five_in_a_row(opp, m.x, m.y)) { isBlock.set(i); threatened = true; }
    clear_bit_generic<Traits>(opp, m.x, m.y);
}
```

(La pierre hypothétique n'étant pas encore posée, le seuil est 4 et non 5.)

### 4.2 — Bitset au lieu de `bool[]`

`bool isBlock[MAX_BOARD_MOVES<Traits>] = {};` → `uint64_t isBlock[5] = {};`
(310 bits). Le `memset` de 310 octets par nœud disparaît du profil.

### 4.3 — Zone active : itérer sur un masque précalculé

Version exacte (on garde `_zoneCount`, on supprime seulement le calcul d'index) :
précalculer `NEIGHBOR[cell]` (bitboard des 25 voisins, ~35 Ko) et remplacer la
double boucle `dx/dy` de `zoneApplyStone` par un `bb_for_each_bit` sur
`NEIGHBOR[cell]`. On économise les 25 `in_board_generic` et les 25
`index_bb_generic`.

La variante par `OR` + pile de restauration est plus rapide encore, mais elle
produit un **sur-ensemble** de la zone en présence de captures — donc plus de
candidats, donc un arbre différent. **Elle sort du périmètre de la partie A** :
gardez-la pour plus tard, avec validation par match.

### 4.4 — `ActiveZone` / `MoveGenerator` hors du chemin chaud

`MoveGenerator::getMaskOfLegalMoves` construit un `ActiveZone` et appelle
`initialize()` — qui balaie les 361 cases et refait tout le voisinage — **à chaque
appel**. `SearchPosition` maintient déjà la même information de façon
incrémentale.

Action : `grep -rn "getMaskOfLegalMoves\|ActiveZone" src/` et vérifier qu'aucun
appel n'est sur le chemin de `minimax`. S'il y en a, remplacer par
`position.candidateMask()`. Si `ActiveZone` n'est plus utilisée que par
l'interface ou les tests, laissez-la mais documentez-le.

**Critère d'acceptation :** 200/200, nœuds identiques, **+8 à +13 %**.

---

## Lot 5 — Table de transposition (1 jour)

### 5.1 — Corriger le hash des captures multiples (à faire en premier)

C'est le seul point du plan qui **change le nombre de nœuds** — en le
**diminuant**, puisqu'il restaure des transpositions actuellement manquées.

Dans `buildMoveHash`, quand `caps == 4` (deux paires d'un coup), la clé de la
paire intermédiaire n'est jamais insérée :

```cpp
// avant
stateHash.hash ^= _hasher.captureHash(Color::White, (_whiteCaptures + caps) >> 1);
// après
const int before = _whiteCaptures >> 1;
const int after  = (_whiteCaptures + caps) >> 1;
for (int p = before + 1; p <= after; ++p)
    stateHash.hash ^= _hasher.captureHash(Color::White, p);
```

Symétriquement dans `undoMove`, qui souffre du même défaut. Après correction, la
convention est identique à celle de `fromBoard`.

**Conséquence sur le harnais :** ce lot invalide la contrainte « nœuds
identiques ». Procédez ainsi :
1. appliquer 5.1 seul ;
2. vérifier 200/200 sur `bestMove`/`bestScore` (la valeur ne doit pas changer,
   seul le chemin change) ;
3. constater `nodesVisited` en baisse et `ttHits` en hausse ;
4. **régénérer `ref.json`** et repartir de cette nouvelle baseline pour la suite.

Ajoutez un test dédié : une position où un coup prend deux paires, jouée par deux
chemins différents (deux ordres de coups menant à la même position) → les deux
hashes doivent être égaux.

### 5.2 — Prefetch

Dès `stateHash.hash` connu, avant `makeMove` :

```cpp
_tt.prefetch(stateHash.hash);   // → __builtin_prefetch(&_bucket[h & _mask], 0, 1);
```

Le lookup arrive ~50 cycles plus tard, la ligne est déjà chargée. Gratuit, exact.

### 5.3 — Buckets alignés

Entrée de 16 octets (clé de vérification 32 bits, score 32 bits, coup 16 bits,
profondeur 8, borne 2, âge 6) × 4 par bucket = **64 octets = une ligne de cache**,
avec `alignas(64)`. Une sonde = un seul accès mémoire au lieu de 1 à 4.

Remplacement à deux niveaux : dans le bucket, un emplacement « profondeur
préférée » et trois « toujours remplacés ». Cela change la politique de
remplacement, donc potentiellement les nœuds — mêmes précautions qu'en 5.1
(valeur inchangée, baseline régénérée).

**Critère d'acceptation :** 200/200 sur `bestMove`/`bestScore`, `ttCutoffs /
nodesVisited` en hausse nette, **+5 à +15 %**.

---

## Lot 6 — Ordonnancement étagé (2 à 3 jours)

Le plus gros chantier de la partie A, placé en dernier parce qu'il bénéficie de
tous les précédents et qu'il touche le cœur de `minimax`.

### 6.1 — Ordre total déterministe

Avant de toucher au tri, rendre l'ordre **total** pour que le tri par sélection
et `std::sort` soient interchangeables :

```cpp
inline bool betterOrder(const EvaluatedMove& a, const EvaluatedMove& b)
{
    if (a.score != b.score) return a.score > b.score;
    return index_bb_generic<Traits>(a.move.x, a.move.y)
         < index_bb_generic<Traits>(b.move.x, b.move.y);   // départage stable
}
```

Appliquer d'abord ce comparateur **avec `std::sort`** et valider 200/200. Si des
positions changent de `bestMove` ici, c'est normal (ex æquo précédemment
départagés par le hasard de `std::sort`) : **régénérez la baseline à ce
moment-là**, une fois pour toutes.

### 6.2 — Tri par sélection incrémental

```cpp
for (size_t i = 0; i < ordered.size(); ++i)
{
    size_t best = i;
    for (size_t j = i + 1; j < ordered.size(); ++j)
        if (betterOrder(ordered[j], ordered[best])) best = j;
    std::swap(ordered[i], ordered[best]);

    // ... makeMove / minimax / undoMove ...
    if (score >= beta) break;
}
```

Avec 6.1 en place, résultat strictement identique. Gain immédiat sur les nœuds
cut.

### 6.3 — Scoring paresseux

C'est ici que se trouve le gros du gain. Structure cible :

```
Étage 0 : coup de la TT           → aucun scoring, essayé directement
Étage 1 : captures + quatre       → detect_capture_mask (bit-tests, très bon marché)
                                    + check_open_four/broken_four filtrés par le lot 2
Étage 2 : killers du ply          → aucun scoring
Étage 3 : le reste                → rawShapeScoreLight + upgrade + défense (top-K)
```

L'étage 3 n'est **calculé que si on y arrive**. Dans un nœud cut bien ordonné,
on n'y arrive jamais.

*Piège de correction :* `restrictToForcedReplies` a besoin de `captureMask` et
`score` sur toute la liste. Deux issues : (a) le faire tourner avant la
génération étagée, sur les seuls `captureMask` (déjà disponibles à l'étage 1) et
sur `isBlock` ; (b) le déclencher paresseusement lui aussi. L'option (a) est plus
simple et suffit.

*Piège de mesure :* le nombre de nœuds ne doit **toujours pas** bouger. Si vous
voyez `nodesVisited` changer, c'est qu'un coup a changé de rang — relisez 6.1.

### 6.4 — Défense limitée aux K premiers

`addDefenseToOrderingScore` seulement pour les `K = 8` meilleurs par score
d'offense. **Ce point n'est pas exact** (l'ordre de la queue change), donc il
change les nœuds sans changer les valeurs. Traitez-le comme 5.1 : valider
`bestMove`/`bestScore`, constater le gain, régénérer la baseline. Si le gain est
marginal, laissez tomber — ce n'est pas le cœur du lot.

### 6.5 — Arène de listes de coups

`MoveList<EvaluatedMove, MAX_BOARD_MOVES<Traits>>` en local fait ~5 Ko de frame
par ply. Allouer une fois dans `MasterAI` :

```cpp
EvaluatedMove _moveArena[MAX_SEARCH_PLY][ZONE_MAX];   // ZONE_MAX ≈ 128, avec assert
```

et donner à chaque ply une vue. Purement mécanique, exact, et rend la pile
compatible avec la profondeur accrue qu'apporteront les extensions plus tard.

**Critère d'acceptation :** 200/200 sur `bestMove`/`bestScore`,
**+25 à +30 %**.

---

## Récapitulatif

| Lot | Contenu | Jours | Nœuds identiques ? | Gain cumulé visé |
|---|---|---|---|---|
| 0 | Harnais + baseline | 1 | — | — |
| 1 | PGO, singleton, copies, défense light | 1 | oui | ×1,2 – 1,3 |
| 2 | Pré-filtre popcount | 1,5 | oui | ×1,5 – 1,7 |
| 3 | Masques compacts | 2 | oui | ×1,7 – 2,0 |
| 4 | Coups forcés + zone active | 1 | oui | ×1,8 – 2,2 |
| 5 | TT (hash captures, prefetch, buckets) | 1 | **non** — rebaseline | ×1,9 – 2,4 |
| 6 | Ordonnancement étagé | 3 | **non** en 6.1/6.4 — rebaseline | ×2,3 – 3,0 |

**Total : ~10,5 jours de travail effectif** pour un facteur 2,3 à 3 sur les
nœuds/seconde, à force de jeu inchangée.

L'objectif de +50 % est atteint dès la fin du **lot 2**, c'est-à-dire en
3,5 jours. Le reste est du bonus — et prépare le terrain pour la partie B, où
PVS et l'approfondissement itératif s'appliqueront à un moteur déjà rapide.

---

## Trois points de vigilance transverses

1. **Les trois lots qui cassent la baseline** (5.1, 6.1, 6.4) doivent être
   isolés dans leurs propres commits, jamais mélangés à du code exact. Sinon
   vous perdez la capacité de distinguer « optimisation correcte » de « bug ».

2. **Mesurez entre chaque lot, pas seulement à la fin.** Les gains annoncés se
   chevauchent : le lot 2 réduit ce que le lot 3 accélère. Si le lot 3 vous donne
   +3 % au lieu de +15 %, ce n'est pas un échec — c'est que le lot 2 avait déjà
   pris le gâteau. Sans mesure intermédiaire, vous ne pourrez pas le savoir et
   vous chercherez un bug qui n'existe pas.

3. **`perf report` avant chaque lot.** Le classement des postes chauds change à
   mesure que vous optimisez. Si après le lot 3 le profil est dominé par
   `bestThreatNear`, sautez le lot 4 et attaquez directement la quiescence de
   menaces (partie B). Le plan est une hypothèse ordonnée, pas un contrat.
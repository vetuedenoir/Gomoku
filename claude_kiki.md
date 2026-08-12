Oui, c'est possible — et même plus radicalement que tu ne le penses.

## 1. Light → V2 : la seule différence est `check_cross`

Les deux fonctions sont des chaînes de `return` précoces identiques, à un maillon près :

| | Light | V2 |
|---|---|---|
| captures / ≥10 | ✔ | ✔ |
| `is_five_in_a_row` | ✔ | ✔ |
| `check_open_four` | ✔ | ✔ |
| `check_broken_four` | ✔ | ✔ |
| `check_cross` | ✘ | ✔ |
| `check_open_three` | ✔ | ✔ |

Conséquence : **si Light est sorti avant l'étage cross (five, open_four, broken_four, ≥10 captures), V2 renverrait exactement le même résultat** — même `captureScore`, même score, même `capturedStones`. Aucun recalcul n'est nécessaire. Et si Light est allé jusqu'à `check_open_three`, il ne reste qu'un seul appel à faire : `check_cross`, et s'il rend 0 on garde le score/`isLegal` déjà calculés par Light.

Il suffit d'ajouter un champ de 1 octet à `EvaluatedMove` :

```cpp
enum class ShapeStage : uint8_t { Five, OpenFour, HalfFour, BrokenFour, ThreeOrQuiet };
```

et la passe « full » devient :

```cpp
if (ordered[i].stage != ShapeStage::ThreeOrQuiet) { /* rien à faire */ }
else {
    int r = tool.check_cross(own, opp, x, y);   // seul appel restant
    if (r) { ordered[i].score = cross_score(r) + captureScore; }
}
```

Tu passes de ~5 scans de motifs à 1 (et sur une minorité de coups seulement). `detect_and_stock_capture`, qui est cher (8 directions × bornes), n'est plus jamais refait non plus.

⚠️ Deux incohérences que ça révèle au passage :
- Dans V2, `isLegal = !(isDoubleThreeScore(r) && !captureScore)` — mais `captureScore` contient `captureCount` (captures déjà accumulées dans la partie). Dès que le camp a capturé une paire, `!captureScore` est faux et **tous les double-trois deviennent légaux**. Light utilise `!caps`, qui est correct. C'est un bug, pas une optimisation.
- Quand `check_cross` sort en premier, V2 laisse `isLegal = true` alors que la forme peut être un double-trois (ton propre commentaire dans `heuristique.inl` note la redondance CROSS_FULL / SCORE_DOUBLE_FULL_FULL). Light et V2 peuvent donc diverger sur la légalité du même coup.

## 2. Les autres gains, par ordre d'impact décroissant

**a) Le `std::cout` dans la boucle de tri de `minimax`.** Un `endl` = flush système, dans le chemin chaud, à chaque coup illégal. C'est potentiellement des centaines de milliers d'appels. À supprimer en premier, avant toute autre mesure.

**b) Génération étagée (staged move generation).** Aujourd'hui tu scores *tous* les candidats (~150-250 cases) avant même d'essayer le coup de la TT. Or sur un nœud de coupure, le coup TT suffit souvent. Ordre correct : probe TT → jouer le coup TT → si coupure beta, sortir sans avoir scoré quoi que ce soit → sinon seulement, générer et scorer. Vu ton taux de `ttOrderingHits`, c'est probablement le plus gros gain unitaire.

**c) `std::partial_sort` au lieu de `std::sort`.** Tu ne consommes que `MAX_CANDIDATES` éléments, mais tu tries intégralement 200+ `EvaluatedMove` — deux fois par nœud. `std::partial_sort(begin, begin+MAX_CANDIDATES, end, betterMove)` suffit, ou `nth_element` + sort du préfixe.

**d) Taille de `EvaluatedMove`.** Il embarque un `MoveList<t_cell,16>`, donc ~80-100 octets. Deux `MoveList<EvaluatedMove, MAX_BOARD_MOVES>` par nœud = plusieurs dizaines de Ko de pile, réécrits à chaque nœud → tu détruis le L1 à chaque appel. Sépare la clé de tri (move + score + flags, 8 octets) du payload de captures, et ne matérialise `capturedStones` que pour les coups réellement joués (ou encode les captures en 8 bits : un bit par direction, ça suffit à les reconstruire).

**e) Détection des coups forcés.** Le gain structurel le plus important au Gomoku : si l'adversaire menace un five ou un open four, il n'y a que 1 à 5 réponses jouables. Actuellement tu explores quand même 20 coups. Un test « menace imparable adverse → restreindre la liste aux parades » fait s'effondrer le facteur de branchement sur toutes les séquences tactiques, qui sont justement les plus profondes.

**f) Iterative deepening.** Tu cherches directement à `_maxDepth` avec un ordonnancement purement statique. Boucler `for (d = 1; d <= _maxDepth; ++d)` coûte ~1/3 en surcoût théorique mais rend l'ordonnancement quasi parfait (PV de l'itération précédente + TT chaude), ce qui est généralement un gain net de 2 à 5× en alpha-beta. Bonus : ça te donne le contrôle du temps (« 5 secondes » devient une contrainte respectée, pas subie), et il faut alors **conserver history entre itérations** au lieu de le reset.

**g) Évaluation incrémentale du tri.** Un coup ne modifie les motifs que sur les lignes qui le traversent (rayon ~4, 4 directions) plus les cases des pierres capturées. Les scores des autres candidats sont inchangés par rapport au nœud parent. Hériter les clés du parent et ne recalculer que la zone touchée divise le coût d'ordonnancement par un ordre de grandeur.

**h) `MAX_CANDIDATES` variable selon la profondeur** (20 à la racine, 8-10 en milieu, 4-6 près des feuilles) plutôt qu'un plafond constant.

**i) Détail de robustesse :** `alpha = std::numeric_limits<int>::min()` puis `alpha + 1` en PVS est un piège (et interdit `-alpha`). Utilise un `INF = 1'000'000'000` symétrique.

Si je devais te donner l'ordre d'attaque : (a) tout de suite, puis (b)+(c)+(d) qui sont mécaniques et sans risque de régression, puis (1) le partage Light/V2, puis (f) et (e) qui demandent plus de travail mais changent la classe de complexité.
## Glossary

- **ply** — one move played by one side. The ply of a node is how many moves were played to reach it from the board we are thinking about: root = 0, the AI's own move = 1, the opponent's answer = 2, and so on.

- **cutoff** — the moment alpha-beta stops looking at a node: one move is already good enough to prove the rest of the list cannot be chosen, so the remaining moves are skipped. Trying the right move first is what creates cutoffs, hence the two heuristics below.

- **killer move** — a move that caused a cutoff at a given ply, remembered so it is tried first in the *other* nodes of that same ply. Those nodes are near-identical boards, so what refuted one usually refutes the next. Two slots per ply, always overwritten by the freshest cutoffs.

- **history** — a counter per side and per cell, increased by `depth²` every time a move played on that cell causes a cutoff, anywhere in the tree. Unlike killers it has no notion of ply: it is a long-run "this square keeps working" score, used as a last tie-break between quiet moves. Cutoffs found far from the leaves count more because they prune bigger subtrees.


## Algo

1. On board with 0 or 1 stone -> AI plays early with dedicated function `tryToPlayEarlyOpeningMove`

2. Reset the move-ordering heuristics -> killers and history start empty at every root search, cutoffs recorded for a previous position are no longer relevant -> `resetOrderingHeuristics`

3. Generate empty moves (empty cells / avalaible cells) in the current board state -> `generateEmptyMoves`

4. We score the move in raw mode in offense and defense sense in order to order them -> `computeRawScoreMove` then `addDefenseToOrderingScore`

5. Drop the moves the rules forbid -> `isLegalMove`, and return at once a move that already wins (five in a row, or 10th captured stone)

6. If the opponent can align five on his next move, keep only the replies that can still change the outcome -> `filterForcedReplies`

7. Sort the candidates best-first on that raw score -> `std::sort`

8. Ask the transposition table what it knows about this exact board -> `_tt.probe`: its best move is rotated to the front of the list, and an EXACT entry searched at least as deep seeds the first bestMove / bestScore

9. Search only the top of the list -> `maxMovesSearched(_maxDepth)` bounds the loop; each candidate is played on a copy of the position (`buildMoveHash` then `makeMove`) and scored by `minimax(depth - 1, alpha, beta)`

10. Keep the highest score, stop as soon as a winning one shows up, and raise alpha for the next candidates

11. Return the best move, search counters left in `lastSearchStats`


## Minimax

`minimax(position, lastMove, depth, alpha, beta)` returns the value of a board, signed
from the AI's point of view. It asks three questions in order: do I already know this
board, is it already decided, and if neither, which replies are worth descending into.

```
minimax(position, lastMove, depth, alpha, beta):
    ply <- maxDepth - depth
```

### 1. Ask the table before doing anything

The same board is reached by many different move orders, and it comes back constantly
during a search. An entry in the table is an entire subtree we do not have to walk
again, and a probe costs one hash lookup — so it happens before we even generate moves.

`entry.depth >= depth` is the trust condition: a value produced by a shallower search is
a weaker claim than the one we are about to make, so it is ignored. A non-exact entry is
still worth having: a bound narrows the window, and a narrow window means cutoffs come
sooner.

```
    entry <- tt.probe(hash(position))
    if entry and entry.depth >= depth:
        if entry.flag == EXACT:  return entry.score
        if entry.flag == LOWER:  alpha <- max(alpha, entry.score)
        if entry.flag == UPPER:  beta  <- min(beta, entry.score)
        if alpha >= beta:        return entry.score
```

### 2. Stop if the node is already decided

Only the stone just played can have completed a five, so testing the last move is enough
and costs a fraction of a full board scan.

Subtracting `ply` from the win score is not cosmetic: without it every forced win is
worth the same, so the engine picks a mate in 7 as happily as a mate in 3 and gives the
opponent time to find a defence. Reaching depth 0 — or a board with no candidate cell
left — hands the answer to the static evaluation, the only place where game knowledge
enters. Everything else here is pure search.

```
    if isWinAfterMove(lastMove) or captures(lastPlayed) >= 10:
        return  WIN_SCORE - ply    if lastPlayed is the AI
                -(WIN_SCORE - ply) otherwise

    if depth == 0:
        return evaluateLeafPosition(position, lastMove)

    moves <- generateEmptyMoves(position.candidateMask())
    if moves is empty:
        return evaluateLeafPosition(position, lastMove)
```

### 3. Order the moves before searching them

This is where alpha-beta is won or lost. Searching the best move first lets the pruning
do its job and brings the tree down to roughly the square root of the naive one; a bad
order pays close to full price for the same result.

Nobody knows the best move in advance, so it is approximated in layers: the shape score
says what the move builds and denies, the killers say what refuted the neighbouring
nodes at this ply, the history says which squares keep producing cutoffs, and a best
move remembered by the table — an answer, not a guess — is placed on top of all of it.

The scoring is done in two passes because its expensive half (the cross scan, and the
counter-scan that measures what the opponent would build on that cell) is wasted on
moves that will never be searched. It is therefore applied only to the head of the list,
and `filterForcedReplies` may have cut that list beforehand to the sole replies that can
still change the outcome.

```
    ordered <- [ computeLightScore(m) for m in moves, keeping the legal ones ]

    filterForcedReplies(ordered)
    sort(ordered, isBetterMove)            # score, then killer, then history

    if depth >= FULL_ORDER_MIN_DEPTH:      # only worth it near the root
        for m in ordered[0 .. maxMovesSearched(depth) + LAZY_FULL_MARGIN]:
            upgradeLightToFull(m)
            addDefenseToOrderingScore(m)
        sort(ordered, isBetterMove)        # scores changed, order must follow

    if entry carries a best move:
        rotate it to the front of ordered  # always searched, ignores the cap
```

### 4. Search the top of the list

After ordering, the first move is usually the best one, so the others do not need to be
measured — they only need to be *refuted*. That is what the null window does: it asks
"can you beat what I already have?" with a one-point-wide window, far cheaper than a
real search, and only a move that answers yes is searched again properly.

The cap is an outright bet on the ordering: past the N-th move we assume the sort was
right and never look. And when a move causes a cutoff it is recorded, because the
neighbouring nodes are nearly the same board and it will very likely refute them too.

```
    maximizing <- (sideToMove == AI colour)
    best       <- -infinity if maximizing else +infinity
    bestMove   <- none
    searched   <- 0

    for m in ordered:
        if searched == maxMovesSearched(depth):  break    # forward pruning
        if m is not legal:                       continue

        makeMove(m)
        searched <- searched + 1

        if searched == 1:
            score <- minimax(position, m, depth - 1, alpha, beta)
        else:
            score <- minimax(position, m, depth - 1, null window)
            if score landed inside (alpha, beta):
                score <- minimax(position, m, depth - 1, alpha, beta)

        undoMove(m)

        if score beats best:
            best <- score, bestMove <- m

        if maximizing:  alpha <- max(alpha, score)
        else:           beta  <- min(beta, score)

        if alpha >= beta:                  # the rest cannot be chosen
            recordCutoff(ply, m, depth)    # feeds killers and history
            break

    if searched == 0:                      # every candidate was illegal
        return evaluateLeafPosition(position, lastMove)
```

### 5. Store what was learned

Writing the result back turns this whole subtree into a single lookup for every later
path that reaches the same board.

The flag records *what kind* of truth we hold. If the window cut the search short, some
moves were never examined, so `best` is only a bound — saying so is what stops a later
probe from trusting it as an exact value.

```
    flag <- UPPER if best <= alpha at entry
            LOWER if best >= beta  at entry
            EXACT otherwise
    tt.store(hash(position), best, depth, flag, bestMove)

    return best
```

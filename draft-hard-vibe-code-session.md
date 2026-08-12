# `draft/hard-vibe-code-session` — Technical & Performance Documentation

**Branch:** `draft/hard-vibe-code-session`  
**Compared against:** `develop` (merge-base `c4f1885`)  
**Document purpose:** explain *why* the search got faster/stronger, with code-level before/after, so the author can revisit the reasoning and colleagues can catch up without replaying the whole session.

---

## 1. Scope of the branch

### Commits on top of `develop`

| Commit | Summary |
|--------|---------|
| `044f7c0` | Killer moves + depth-10 strategy scaffolding |
| `3c156cb` | TT size experiment + cleanup (TT size later restored; see §9) |
| `d34ec6d` | Trivial-move / alignment tests |
| `7846ea9` | Main “vibe” session: PVS, leaf eval, pattern opts, defense ordering |
| `5a64bb2` | Lazy scoring (light → full on top pool) |

### Files with lasting deltas vs `develop`

| Area | Files |
|------|--------|
| Search / AI | `include/ai/MasterAI.hpp`, `MasterAI.inl`, `heuristique.inl` |
| Pattern engine | `BitboardTool.hpp`, `BitboardToolBuild.inl`, `BitboardToolChecks.inl`, `PatternTypes.hpp`, `bitboard.hpp` |
| Config | `include/config/config.hpp` (`DEPTH` 8→10, debug off by default) |
| Tests | `tests/ai/test_tt_transposition_equality.cpp` (new), `tests/performance/test_search_performance.cpp` |
| Notes | `to-10-depth.md` (session scratchpad; historical) |

**Not in the final tip vs `develop`:** iterative deepening, aspiration windows, LMR — implemented during the session, measured as regressions, reverted before `5a64bb2`.

---

## 2. Goal and bottleneck diagnosis

### Product goal

Reach **fixed depth ~10** with a **~500 ms average** move time (soft target: never abort mid-search by wall-clock). Strength must not be sacrificed for speed.

### What the profiler / call counting showed

On hard positions (depth 8, capture-rich midgame), almost all time was **not** in leaf evaluation of the last stone, but in **move ordering**:

- Every internal node scored **all** empties in the active zone with a full shape stack (`rawShapeScore`: five / open-four / broken-four / cross / open-three + captures).
- Alpha-beta then searched only ~3–4 children before a cutoff, while **~30 candidates** had already been fully scored.
- Ordering work was on the order of **~10×** leaf evals (measured ~5.5M `rawShapeScore` calls vs ~0.5M leaf evals on one d8 capture position).

Secondary costs:

- Each `matchPattern` scanned **all** bitboard words (6 on 19×19) even when the mask lived in 1–2 words.
- Leaf scores looked only at the **last move** → shallow scores were noisy → techniques that need reliable shallow eval (LMR, ID+aspiration) failed when tried.

### Design response (high level)

1. **Make cutoffs earlier** → better ordering (killers, history, defense, PVS).  
2. **Score fewer candidates with the expensive stack** → lazy light→full.  
3. **Make each shape check cheaper** → word-span `matchPattern`, masked open-threes.  
4. **Make leaves less lying** (without making every leaf a full board scan) → adaptive bilateral leaf.  
5. **Do not keep search tricks that increase node count** → LMR/ID reverted after measurement.

---

## 3. Search pipeline (after this branch)

```text
findBestMove (root)
  ├─ full rawShapeScore + defense/2 on root candidates
  ├─ immediate win / 10th capture short-circuit
  └─ for each root move: minimax(depth-1)

minimax
  ├─ TT probe (cutoff / window / bestMove hoist)     [already on develop]
  ├─ terminal (five / capture win)
  ├─ depth==0 → evaluateLeafPosition (adaptive bilateral)
  ├─ generate empties from candidateMask (active zone)
  ├─ LAZY ORDER:
  │     light-score ALL legal candidates
  │     sort (+ killers/history tie-break)
  │     if depth >= 4: full V2+defense on top LAZY_FULL_POOL
  │     re-sort
  │     hoist TT bestMove to front
  └─ search up to MAX_CANDIDATES with PVS
        first child: full window
        later: null-window scout, re-search on fail-high
        on beta cutoff: recordCutoff → killers + history
```

Key knobs (current tip of branch):

```cpp
// include/ai/MasterAI.inl
static constexpr int THREAT_SCAN_THRESHOLD  = 5000;
static constexpr int THREAT_SCAN_EARLY_EXIT = 90000;
static constexpr int LEAF_SCAN_RADIUS      = 2;
static constexpr int FULL_ORDER_MIN_DEPTH  = 4;
static constexpr int MAX_CANDIDATES        = 16;   // develop: 24
static constexpr int LAZY_FULL_POOL        = MAX_CANDIDATES + 4;
```

```cpp
// include/config/config.hpp
#define DEPTH 10   // develop: 8
```

---

## 4. Optimization strategies (detailed)

---

### 4.1 Killer moves + history heuristic

**Strategy / technique name:** Dynamic quiet-move ordering (killers + history)

**Problem**  
On `develop`, candidates with the same static shape score were ordered arbitrarily among themselves. Quiet cutoffs discovered deeper in the tree were not reused at the same ply or on similar positions, so alpha-beta often tried the wrong silent move first.

**Before (`develop`)** — sort by static score only:

```cpp
std::sort(ordered.begin(), ordered.end(),
    [](const EvaluatedMove& a, const EvaluatedMove& b) {
        return a.score > b.score;
    });
// … on cutoff:
if (beta <= alpha) { ++_stats.nodesPruned; break; }
```

**After (`draft/hard-vibe-code-session`)** — tables + tie-break + record on cutoff:

```cpp
// MasterAI.hpp
t_cell _killers[MAX_SEARCH_PLY][2];
int    _history[2][Traits::CELL_COUNT];

// MasterAI.inl — comparator
if (a.score != b.score) return a.score > b.score;
const bool ak = isKillerMove(ply, a.move);
const bool bk = isKillerMove(ply, b.move);
if (ak != bk) return ak;
return historyScore(mover, a.move) > historyScore(mover, b.move);

// on beta cutoff
recordCutoff(ply, move, depth, colorBeforeMove);
// killers: two slots, no duplicate; history[side][cell] += depth*depth
```

**What changed**  
- Two killer slots per ply, reset each root search.  
- History indexed by side × cell, bonus `depth²` on cutoffs.  
- Used only as a **tie-break** when static scores are equal, so tactical tiers (fours, captures) still dominate.

**Why it is better**  
Quiet packages (score ≈ 89) are huge on Gomoku. Distinguishing which quiet caused a sibling cutoff raises first-move quality → earlier β-cutoffs → fewer nodes. This is classic Chess/Gomoku search lore; here it unlocked later PVS gains (null-window scouts fail fast when move #1 is good).

**Trade-offs / considerations**  
- Memory: `2 × CELL_COUNT` ints of history (~3k ints on 19×19) + 64×2 cells. Negligible.  
- Killers ignore ply ≥ 64.  
- History can overfit a single search; reset at each `findBestMove` avoids cross-position pollution.

**Impact**  
Internal nodes near the root and mid-depth. Combined with PVS, empty-board d8 node counts dropped substantially vs pre-session baselines (see §5).

---

### 4.2 Principal Variation Search (PVS / null-window)

**Strategy / technique name:** Principal Variation Search

**Problem**  
`develop` always searched every child with the full `[α, β]` window. When ordering is good, most non-first moves are worse than the PV; a full window wastes work proving a precise score that will be cut anyway.

**Before (`develop`):**

```cpp
position.makeMove(move.x, move.y, colorBeforeMove, moveHash);
int eval = minimax(position, move, depth - 1, alpha, beta);
position.undoMove(move.x, move.y, colorBeforeMove);
```

**After:**

```cpp
if (legalMovesSearched == 1)
    eval = minimax(position, move, depth - 1, alpha, beta);
else if (isMaximizing) {
    eval = minimax(position, move, depth - 1, alpha, alpha + 1);
    if (eval > alpha && eval < beta)
        eval = minimax(position, move, depth - 1, alpha, beta);
} else {
    eval = minimax(position, move, depth - 1, beta - 1, beta);
    if (eval < beta && eval > alpha)
        eval = minimax(position, move, depth - 1, alpha, beta);
}
```

**What changed**  
First legal move: full window. Later moves: null-window scout; re-search full window only on fail-high inside `(α, β)`.

**Why it is better**  
Null windows prune aggressively. With killers/history/TT hoist making the first move usually best, most scouts return immediately. **Lossless** relative to plain alpha-beta when re-search is done correctly (same minimax value).

**Trade-offs / considerations**  
- Bad ordering → many fail-highs → re-search storm (can be slower than plain AB). That is why killers/history were prerequisites.  
- LMR on top of PVS was tried later and **hurt** capture positions (see §8).

**Impact**  
Global node reduction on quiet and midgame trees; pairs with §4.1.

---

### 4.3 Defense-aware static ordering

**Strategy / technique name:** Offense + defense/2 ordering key

**Problem**  
Root/internal ordering on `develop` used **offense only** (`rawShapeScore` for the side to move). Blocking an opponent open-three/four often scored as a quiet 89, so defense was deferred — dangerous under `MAX_CANDIDATES` forward pruning.

**Before (`develop`)** — root push without defense:

```cpp
EvaluatedMove scoredMove = rawShapeScore(position.board(), rootMoves[i], color);
if (scoredMove.isLegal) {
    if (scoredMove.score >= WIN_SCORE || /* 10th capture */)
        return scoredMove.move;
    orderedRoot.push(scoredMove);
}
```

**After:**

```cpp
addDefenseToOrderingScore(scoredMove, board, color);
// heuristique.inl:
void MasterAI<Traits>::addDefenseToOrderingScore(EvaluatedMove& offense,
    const t_BWBoard<Traits>& board, Color side)
{
    const Color opp = (side == Color::Black) ? Color::White : Color::Black;
    const EvaluatedMove defense = rawShapeScore(board, offense.move, opp);
    offense.score += defense.score / 2;
}
```

Also used when lazy-rescoring the top pool with `rawShapeScoreV2` (§4.6).

**What changed**  
Ordering key = own threat/capture value + half of what the opponent would create on the same empty. `isLegal` / `capturedStones` stay those of the side to move (needed for `makeMove`).

**Why it is better**  
Raises the rank of blocking cells into the searched top-N without changing legality. Strength fix first; also tends to cut losing lines earlier.

**Trade-offs / considerations**  
- **Cost:** one extra full `rawShapeScore` per fully ordered candidate. That is why full+defense is **not** applied to every candidate at every node — only root and the lazy top pool (§4.6).  
- Putting defense into **leaf** `stmBest` was tried and **rejected**: a block (~defense/2) cancelled the opponent’s open-four already counted in `createdSigned` and broke tactical tests.

**Impact**  
Root choice quality and deep nodes that use the full pool; fewer “forgot to block” blunders under forward pruning.

---

### 4.4 Cheaper pattern matching (word-span + open-three masks)

**Strategy / technique name:** Span-limited `matchPattern` + precomputed stone masks for open threes

**Problem**  
Every shape check AND-ed pattern masks across **all** bitboard limbs. On 19×19 (`WORD_COUNT == 6`), most patterns touch 1–2 words. Open-three checks also did 3–4 `get_bb_flate` per candidate pattern.

**Before (`develop`) — full-word scan:**

```cpp
static bool matchPattern(const Bitboard& pat, const Bitboard& board)
{
    for (size_t i = 0; i < pat.size(); i++)
        if ((pat[i] & board[i]) != pat[i])
            return false;
    return true;
}
```

Open three (stone tests):

```cpp
if (get_bb_flate(stones, pattern.stone_pos[0])
    && get_bb_flate(stones, pattern.stone_pos[1])
    && get_bb_flate(stones, pattern.stone_pos[2]))
```

**After — precomputed span + masked match:**

```cpp
// PatternTypes / build time
computeMaskSpan(mask, list->firstWord[slot], list->lastWord[slot]);

// Hot path
if (matchPattern(list.masks[i], stones, list.firstWord[i], list.lastWord[i]))

// Open three: finalize_group3_masks builds fullMask / holeMask0 / holeMask1
if (matchPattern(pattern.fullMask, stones, pattern.fullFirst, pattern.fullLast))
```

Also adds `set_bb_flate` in `bitboard.hpp` for building those masks (guards `idx < 0`).

**What changed**  
- Pattern structs store `[firstWord, lastWord]` inclusive spans.  
- `t_PatternGroupe3` became `template<typename Traits>` and carries three stone masks.  
- Check paths use the spanned overload; a zero-limb-skipping overload remains as fallback.

**Why it is better**  
Same semantics (zero limbs always pass the subset test), fewer memory ops per match. Measured ~**25%** wall-clock win on quiet empty-board d8 **with identical node counts** during the session — pure NPS improvement.

**Trade-offs / considerations**  
- Larger pattern tables (extra uint8 spans + 3 bitboards per group3). Built once at startup.  
- Must keep spans in sync when masks change (`finalize_group3_masks` / `computeMaskSpan` at build).

**Impact**  
Every `rawShapeScore*` / leaf / defense call — the entire hot path.

---

### 4.5 Adaptive bilateral leaf evaluation

**Strategy / technique name:** Adaptive bilateral leaf (`evaluateLeafPosition`)

**Problem**  
`develop` leaves returned only the **local** score of the stone just played:

```cpp
if (depth == 0) {
    return (lastPlayed == Color::Black)
        ? evaluateBlackPosition(position, cell)
        : evaluateWhitePosition(position, cell);
}
```

That ignores what the **side to move** can threaten next. Shallow scores were overconfident on quiets that left an open-four for the opponent — which later made LMR/ID explode (false fail-highs).

**After:**

```cpp
int MasterAI<Traits>::evaluateLeafPosition(const SearchPosition& position, t_cell cell)
{
    const Color stm = position.sideToMove();
    const Color lastPlayed = (stm == Color::Black) ? Color::White : Color::Black;

    const int createdSigned = (lastPlayed == Color::Black)
        ? evaluateBlackPosition(position, cell)
        : evaluateWhitePosition(position, cell);

    if (createdSigned >= MATE_THRESHOLD || createdSigned <= -MATE_THRESHOLD)
        return createdSigned;
    // Quiet last move: skip zone scan (keep O(1) shape cost)
    if (createdSigned > -THREAT_SCAN_THRESHOLD && createdSigned < THREAT_SCAN_THRESHOLD)
        return createdSigned;

    const int stmBest = bestThreatNear(position, stm, cell); // offense only, Chebyshev ≤ 2
    return createdSigned + signedFromAi(stm, stmBest);
}
```

`bestThreatNear` scans empties in Chebyshev radius 2 around `cell`, intersected with `candidateMask`, early-exits at score ≥ 90 000 (double-three / cross / open-four tier).

**What changed**  
- Leaf = signed “what last move created” + signed “best immediate offense for STM near that move”, but **only** when `|created| ≥ 5000` (broken-four+).  
- Wired into all three former leaf exits in `minimax` (depth 0, no moves, no legal moves).

**Why it is better (strength)**  
Tactical leaves that just created a heavy threat also credit/debit the opponent’s best local reply. Lines that hang an open-four are no longer scored as “quiet success.”

**Why the cost controls matter (speed)**  
A naïve “always scan the whole zone” leaf was correct but **slow** (~16 s on captures d8 in session measurements). Adaptive + local scan kept the signal where it matters and left quiet leaves cheap.

**Trade-offs / considerations**  
- Threshold 5000: open-threes alone (~800) do **not** trigger a scan — latent double threats can be under-counted at horizon. Tunable.  
- Local radius 2 can miss a distant threat outside the last-move neighborhood (mitigated by active zone radius 1 already limiting candidates).  
- Must **not** fold defense into `stmBest` (see §4.3).

**Impact**  
Strength/reliability of depth-0 scores; prerequisite for trusting shallow bounds. Net speed vs full-scan bilateral is much better; vs old last-move-only, leaves can be slightly slower on tactical tips only.

---

### 4.6 Lazy move ordering (light → full on top pool)

**Strategy / technique name:** Lazy / staged static ordering

**Problem**  
Even after killers/PVS, **every** candidate at every internal node paid a near-full shape stack. With `ACTIVE_ZONE_RADIUS = 1`, zone size is moderate, but × nodes still dominated time. Full+defense on all candidates (§4.3) would make this worse.

**Before (`develop`):**

```cpp
for (size_t i = 0; i < movesArray.size(); ++i) {
    EvaluatedMove scoredMove = rawShapeScore(position.board(), movesArray[i], themover);
    if (scoredMove.isLegal)
        ordered.push(scoredMove);
}
std::sort(/* score only */);
// then search top MAX_CANDIDATES
```

**After:**

```cpp
// 1) Light score everyone (no cross; no opponent defense)
for (...) {
    EvaluatedMove scoredMove = rawShapeScoreLight(board, movesArray[i], mover, capturesBefore);
    if (scoredMove.isLegal) ordered.push(scoredMove);
}
std::sort(ordered.begin(), ordered.end(), betterMove); // + killers/history

// 2) Near the root only: expensive re-score of the likely search set
if (depth >= FULL_ORDER_MIN_DEPTH && !ordered.empty()) {
    const size_t pool = min(ordered.size(), LAZY_FULL_POOL); // MAX_CANDIDATES+4
    for (size_t i = 0; i < pool; ++i) {
        EvaluatedMove full = rawShapeScoreV2(board, ordered[i].move, mover, capturesBefore);
        if (!full.isLegal) { /* demote */ continue; }
        addDefenseToOrderingScore(full, board, mover);
        ordered[i] = full;
    }
    std::sort(ordered.begin(), ordered.end(), betterMove);
}
```

`rawShapeScoreLight` = captures + five + open/broken four + open_three (for legality/score), **skips `check_cross`**.  
`rawShapeScoreV2` = full stack + cumulative capture count toward the 10-capture win.

**What changed**  
Most candidates never pay cross + defense. Depth &lt; 4 stays light-only. Depth ≥ 4 pays full+defense only for ~20 moves (`LAZY_FULL_POOL`), not 30–50.

**Why it is better**  
Attacks the measured bottleneck directly: **stop fully evaluating moves that will never be searched**. Session bench after this change (with `MAX_CANDIDATES` ~16–18): capture-storm d8 went from ~7 s (two-tier without lazy) to ~**2.2 s**.

**Trade-offs / considerations**  
- Light may underrank a pure **defensive** cell that looks quiet offensively; the `+4` pool margin and root full+defense mitigate this.  
- If light ranks a critical move outside the pool, it may never get full re-score nor search (same class of risk as any forward-pruning scheme). TT bestMove hoist still forces exploration of the TT move even if poorly ranked.  
- Slightly more code paths (`Light` / `V2` / `rawShapeScore`).

**Impact**  
Largest single wall-clock win of the late session on sharp positions.

---

### 4.7 Forward-pruning width (`MAX_CANDIDATES`)

**Strategy / technique name:** Top-N legal move cap

**Problem / context**  
Already present on `develop` (`MAX_CANDIDATES = 24`). Hard forward pruning: only the N best **legal** ordered moves are searched.

**After**  
Branch tip uses a tighter cap (`16` at time of writing; was `18` / briefly experimented lower during the session). Root loop uses the same constant.

**Why it is better (speed)**  
Branching factor compounds with depth; lowering N is exponential in effect on sharp trees.

**Trade-offs / considerations**  
- **Strength risk:** the refutation ranked N+1 is invisible. Session found `8` too narrow (failed block-win-in-1); `12–16` usually kept tactical guards green. Prefer lazy scoring over aggressive N cuts when possible.  
- Not a substitute for good ordering: with bad order, smaller N makes the engine *dumber*, not just faster.

**Impact**  
All internal nodes; interacts with lazy pool size (`LAZY_FULL_POOL = N+4`).

---

### 4.8 Strength correctness fixes (non-optional)

These are not “speedups” but are part of the branch delta and affect both strength and the validity of benches.

#### 4.8.1 White open-four polarity bug

**Problem**  
`evaluateWhitePosition` on `develop` called `check_open_four(board.black, board.white, …)` — **swapped** own/opponent masks — so White open-fours were mis-detected.

**Before:**

```cpp
int result = tool.check_open_four(board.black, board.white, cell.x, cell.y);
```

**After:**

```cpp
int result = tool.check_open_four(board.white, board.black, cell.x, cell.y);
```

**Impact**  
Correct White threat evaluation at leaves and any code path using that helper.

#### 4.8.2 Double-three legality with capture

**Problem**  
`develop` treated any double-three score as illegal:

```cpp
data.isLegal = !tool.isDoubleThreeScore(r);
```

Renju/Gomoku-with-capture rules allow the double-three if the move **captures**.

**After:**

```cpp
data.isLegal = !(tool.isDoubleThreeScore(r) && !captureScore);
```

**Impact**  
Legal tactical resources involving capture are no longer dropped from the ordered list.

#### 4.8.3 Root immediate-win stats

When returning a mate/10th-capture at the root, the branch now fills `_stats.bestScore` / `_stats.bestMove` before return (avoids bogus default stats).

---

### 4.9 Config: default depth and debug

| Knob | `develop` | Branch tip |
|------|-----------|------------|
| `DEPTH` | 8 | **10** |
| `GOMOKU_DEBUG` | defined (logs/stats on) | commented (benchmark default) |
| `ACTIVE_ZONE_RADIUS` | 1 | 1 (unchanged) |

**Impact**  
Production/search default goes deeper; release builds avoid logger overhead. PERF tests relaxed the empty-board 500 ms hard assert for depth &gt; 6 (machine variance).

---

## 5. Measured impact (session benches)

Numbers below are from the optimization session on this branch (same BENCH positions: `menaces-croisees`, `threes-opposes`, `captures`). Absolute times depend on machine; **ratios** matter.

| Stage (captures @ depth 8) | Nodes (approx) | Time (approx) |
|----------------------------|----------------|---------------|
| Bilateral leaf, always full zone scan | 342k | 16.6 s |
| + quiet-skip / local leaf scan | 494k | 13.5 s |
| + two-tier full@depth≥4 (no lazy yet) | 368k | 7.0 s |
| + **lazy light→full pool** | **113k** | **2.2 s** |

Empty board depth 8 moved from ~4 s (heavy bilateral) to ~0.6 s with the final stack.

**Rough read vs original `develop` (pre-branch, older session notes):** capture-storm d8 was on the order of ~1.5–2.5 s / ~400–670k nodes with earlier caps; the branch adds depth-10 defaults and stronger ordering/eval while keeping tactical tests green (187/187 at session end). Compare apples-to-apples with:

```bash
make run_tests FILTER="*[PERF]*"   # enable GOMOKU_DEBUG temporarily for BENCH logs
```

---

## 6. What was tried and **not** kept

### Late Move Reductions (LMR)

- **Idea:** search late quiet moves at `depth - 1 - R`, re-search on fail-high.  
- **Result:** empty boards faster; **captures d8 ~7 s → ~15 s**, nodes up (~368k → ~597k). False fail-highs from remaining leaf noise caused re-search storms.  
- **Status:** fully reverted; comment left in `MasterAI.inl`.

### Iterative deepening + aspiration

- **Idea:** deepen 1…D, reuse TT/order, narrow windows around previous score.  
- **Result:** more total nodes/time than a single deep search — static+killer ordering was already strong enough that shallow iterations were pure overhead; TT hoist from shallow scores sometimes **worsened** deep ordering.  
- **Status:** reverted earlier in the session (not in final tip).

### Defense inside leaf `stmBest`

- Broke “extend three to four” / fail-soft tests by cancelling opponent open-fours.  
- **Status:** offense-only for `bestThreatNear`.

---

## 7. Tests added / updated

| Test | Role |
|------|------|
| `tests/ai/test_tt_transposition_equality.cpp` | Zobrist must be move-order independent; TT probe must hit across permutations (foundation for TT ordering / any future ID). |
| `tests/performance/test_search_performance.cpp` | BENCH variant label `LAZY(light→full@top-pool)`; softer empty-board time guard for d8. |
| Existing MasterAI / minimax suite | Guards win-in-1, block, extend-three, open-four, TT flags — used as strength regression net while tuning. |

---

## 8. How to think about the remaining gap

Still between “strong fixed depth 10” and “~500 ms **average**”:

1. **Average ≠ worst case.** Capture storms dominate the mean; quiet moves are already cheap.  
2. **Next architectural lever** (not yet on the branch): **incremental threat maintenance** on `make`/`undo` (track open-fours / wins per side) so leaves and cheap probes stop re-running shape stacks. High engineering cost; high ceiling.  
3. Revisit LMR/ID only if leaf/threat signal becomes clearly more stable (e.g. after incremental threats).

---

## 9. Footnotes / historical TT change

Commit `3c156cb` briefly introduced `GOMOKU_TT_BITS` and shrank the table. The **tip of the branch matches `develop` again** on `TranspositionTable.hpp` (`TABLE_SIZE = 1u << 26`). Treat TT sizing as **not** part of the lasting performance story of this branch unless reintroduced cleanly.

`to-10-depth.md` at repo root is a mid-session note (aspirational ID/LMR advice) and is **partly outdated** relative to later measurements — prefer this document for the final state.

---

## 10. Quick file map for reviewers

| If you care about… | Start here |
|--------------------|------------|
| PVS / killers / lazy order / leaf wiring | `include/ai/MasterAI.inl` |
| Shape scorers, bilateral leaf, defense helper | `include/ai/heuristique.inl` |
| Declarations | `include/ai/MasterAI.hpp` |
| Pattern hot path | `include/bitboard/BitboardToolChecks.inl`, `PatternTypes.hpp` |
| Depth / debug | `include/config/config.hpp` |
| BENCH harness | `tests/performance/test_search_performance.cpp` |

---

*Generated from an actual `git diff develop...draft/hard-vibe-code-session` inspection, not from commit messages alone.*

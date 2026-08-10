Nice — that's a real milestone. Let me put it in context and lay out what's left, since you're now in Ask mode (analysis only, no edits).

## Where you are

- **Depth 10, ~3 s average, full-strength (24 candidates, no dumbing-down).**
- Original extrapolation for depth 10 was **~41 s**. So the TT + killer/history + PVS stack has bought you roughly a **13× speedup** while *widening* the candidate cap from 12 back to 24. That's the important part: you got faster *and* smarter, not one at the expense of the other.
- Remaining gap to the 500 ms target: about **6×**.

## The thing to understand about "3 s average"

That average is almost certainly **dominated by a handful of worst-case positions**, not representative of a typical move. Recall the two extremes we measured at depth 8, cap 24:

| Position type | depth-8 nodes | depth-8 time |
|---|---|---|
| Quiet / empty board | 74 k | 154 ms |
| Capture-storm | 391 k | 1.49 s |

That's a **~10× spread**. At depth 10 the spread widens further — the quiet moves are probably ~0.5–1 s and the capture-heavy midgame moves are 8–15 s, averaging out to ~3 s. So the real question isn't "make everything 6× faster," it's **"tame the capture-storm tail,"** because that tail is what inflates the mean.

## Levers to close the gap, ranked by strength-safety

**Lossless (zero strength risk) — do these first:**
1. **Aspiration windows** (needs iterative deepening). Search depth *d* re-using depth *d−1*'s score as a narrow `[score−δ, score+δ]` window. With your now-good move ordering this prunes hard. ID was shelved earlier because ordering was noisy — but killer/history/PVS/TT have since fixed exactly that root cause, so it's worth revisiting. Typical gain: 1.5–2×.
2. **Faster/incremental leaf eval.** The profiler showed shape-checks are ~89% of eval time, and the leaf calls `evaluateBlack/WhitePosition` which re-run `check_broken_four`/`check_open_three`/`cross` from scratch. Maintaining these incrementally as stones are placed/removed is a pure win. Gain: hard to estimate but potentially large since it's the hot path.
3. **TT sizing / replacement scheme.** If the table is small or uses naive replacement, the capture-storm subtree thrashes it. A depth-preferred + always-replace two-tier bucket can lift hit rate a lot on exactly those deep, transposition-rich capture lines.

**Soft pruning (small, tunable strength risk) — the biggest single lever for the tail:**
4. **Late Move Reductions (LMR).** Search late, low-ranked quiet moves at reduced depth; re-search full depth only if they surprise. This directly attacks the capture-storm node count (that position had 30% pruning — lots of room). Gain: often 2–3× on wide nodes. Because your ordering is now trustworthy, the reductions are safe *most* of the time — but it *can* miss a deep quiet resource, so it must be gated behind your tactical guards and never applied to captures/checks/tactical moves.

**Structural:**
5. **Capture-aware branching control.** Capture moves inflate branching because they change the board a lot. Options: extend on captures (search them deeper, narrow elsewhere) or treat capture-generating moves specially in ordering so cutoffs come sooner.

## My read

- If 500 ms is a **soft target** (you said earlier it's "not a hard cap, never stop by time"), then **~3 s at full strength is already a defensible, strong engine** — most of that 3 s is a few pathological positions.
- If you want to actually hit ~500 ms average, the realistic path is: **(1) revisit iterative deepening + aspiration windows now that ordering is fixed**, then **(2) add LMR for the capture-storm tail**, measuring nodes/time/guards at each step. Those two together plausibly cover the 6×.
- The **capture-storm worst case** is the one number I'd watch, not the average — if you get that from ~1.5 s (d8) to sub-500 ms, the whole average follows.

Want me to (in Agent mode) prototype iterative deepening + aspiration windows first, or go straight at the capture tail with LMR? I'd lean ID+aspiration first since it's lossless and re-tests the TT-ordering foundation you were worried about earlier.
# Move validation

## Facade: `MoveValidator`

- **Input:** `ValidationContext` (board + opening runtime) + `GamePhase` + side to move + `Move`.
- **Output:** `isLegal`, `legalMoves` — read-only, no stone placement.
- **Logging:** tag `VALIDATOR` on accept/reject.

## Phase policies (heterogeneous internals)

| Phase | Policy | Representation | Implementation |
|-------|--------|----------------|----------------|
| Opening | `OpeningMovePolicy` | `GameBoard` + `historyPlacedStones` | `OpeningRules` |
| Standard | `StandardMovePolicy` | Ephemeral `t_BWBoard` from board | `StandardRules`, `MoveGenerator` |

### Why two representations?

- **Opening:** tournament placement (centre, Chebyshev distance, forced colour) is defined on cell coordinates and opening history. Natural on `GameBoard`.
- **Standard:** Gomoku rules (double-three, capture exception) live on bitboards for AI/search. Natural on `t_BWBoard`.

`GameBoard` is the source of truth for placed stones. Standard validation builds a **read-only** snapshot via `GameBoard_to_bitboard`; it must not diverge from the board.

## Do not

- Call `StandardRules` during opening.
- Reimplement opening constraints on bitboard without a strong reason.
- Place stones inside `MoveValidator` or policies — use `commitOpeningMove` / `TurnController` instead.

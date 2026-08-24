#ifndef OPENINGENGINE_HPP
#define OPENINGENGINE_HPP

#include "game/contracts/contracts.hpp"
#include "game/board/GameBoard.hpp"
#include <vector>

struct PlacementConstraint
{
	bool mustBeCenter = false;
	int  refStoneIdx  = -1;
	int  minDist      = 0;
};

struct StoneSpec
{
	CellStatus          color;
	PlacementConstraint constraint;
};

struct OpeningStep
{
	Seat                   actor;
	std::vector<StoneSpec> stones;
	bool                   triggersColorChoice = false;
};

enum class OpeningEvent
{
	StepContinues,
	NextStep,
	ColorChoice,
	Finished,
};

struct OpeningCommitResult
{
	bool                success = false;
	OpeningEvent        event   = OpeningEvent::StepContinues;
	std::optional<Seat> nextSeat;
};

// Speaks only in Seat / CellStatus — it knows nothing about human vs AI.
class OpeningEngine
{
public:
	explicit OpeningEngine(OpeningProtocol protocol);

	bool            isComplete() const;
	CellStatus      nextColor() const;
	OpeningStep     currentStep() const;
	OpeningProtocol protocol() const;
	int             stepIndex() const;

	bool                canPlace(const GameBoard& board, const Move& move) const;
	OpeningCommitResult commit(GameBoard& board, const Move& move);
	std::vector<Move>   legalMoves(const GameBoard& board) const;

	void continuePlacement();

private:
	OpeningProtocol          _protocol;
	std::vector<OpeningStep> _steps;
	int                      _stepIdx = 0;
	int                      _subIdx  = 0;
	std::vector<t_cell>      _history;
};

#endif

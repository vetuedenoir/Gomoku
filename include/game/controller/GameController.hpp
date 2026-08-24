#ifndef GAMECONTROLLER_HPP
#define GAMECONTROLLER_HPP

#include "game/controller/IGameController.hpp"
#include "game/OpeningEngine.hpp"
#include "game/validation/MoveValidator.hpp"
#include "game/turn/TurnController.hpp"
#include "ai/MasterAI.hpp"
#include "tracker/Tracker.hpp"

template<typename Traits> class GameController : public IGameController
{
public:
	explicit GameController(const GameConfig& config);

	MoveResult submitMove(int col, int row) override;

	bool submitOpeningMove(int col, int row) override;
	void resolveColorChoice(bool swapped) override;
	void continueOpeningPlacement() override;

	std::optional<Move>            requestAIMove() override;
	std::optional<OpeningDecision> takeOpeningDecision() override;
	std::optional<Move>            suggestMove() override;

	const GameBoard& visualBoard() const override;
	GamePhase        phase() const override;

	Actor currentActor() const override;
	Actor playerActor() const override;
	Actor aiActor() const override;
	bool  aiOpponent() const override;
	bool  aiVsAi() const override;

	CellStatus                nextOpeningColor() const override;
	Color                     currentColor() const override;
	OpeningProtocol           openingProtocol() const override;
	int                       stepIdx() const override;
	std::optional<Color>      getColorFromWinningActor() const override;
	std::optional<PendingWin> pendingWin() const override;

	double aiMoveLastMs(Color color) const override;
	double aiMoveAverageMs(Color color) const override;

	int blackCaptureCount() const override;
	int whiteCaptureCount() const override;

	void seedStandardPosition(const GameBoard& stones, Color toMove) override;

private:
	void assignColorsAfterChoice(bool swapped);
	void applyOpeningResult(const OpeningCommitResult& result);
	void enterStandardPhase();
	void logPhaseTransition(GamePhase from, GamePhase to) const;
	void beginNormalPlay();
	void passTurn();
	void playAiColorChoice();

	Color colorOf(Seat s) const;
	Seat  seatOf(Color c) const;
	Color aiColor() const;
	Color playerColor() const;

	std::unique_ptr<GameBoard> _board;
	OpeningEngine              _opening;
	GamePhase                  _phase;
	MoveValidator<Traits>      _validator;
	TurnController<Traits>     _turnController;
	MasterAI<Traits>           _masterAI;
	std::array<Tracker, 2>     _trackers; // indexed by Color (Black=0, White=1)
	std::optional<Color>       _winner;

	std::optional<PendingWin> _pendingWin;
	int                       _capturesBlack = 0;
	int                       _capturesWhite = 0;

	std::array<Color, 2>           _colorBySeat;        // [Seat::First] = Black, [Seat::Second] = White
	Seat                           _aiSeat;             // which seat the AI occupies (only field a swap mutates)
	Seat                           _currentSeat;        // pure turn cursor; never touched by a swap
	bool                           _aiOpponent = true;  // false ⇒ hotseat (both seats human)
	bool                           _aiVsAi     = false; // both seats driven by MasterAI
	std::optional<OpeningDecision> _lastOpeningDecision;
};

#include "game/controller/GameController.inl"

#endif

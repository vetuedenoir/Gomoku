#include "doctest.h"
#include "logger/Logger.hpp"
#include "ai/MasterAI.hpp"
#include "ai/MoveGenerator.hpp"
#include "ai/SearchPosition.hpp"
#include "helpers/board.hpp"
#include "helpers/helpers.hpp"
#include "helpers/helpers_19.hpp"
#include "helpers/line_helpers.hpp"

static SearchPosition19 posWithSideToMove(GameBoard& b, Color colorToMove)
{
	b.setCurrentColor(colorToMove);
	return SearchPosition19::fromBoard(b);
}

static std::string colorStr(Color color)
{
	return (color == Color::Black) ? "Black" : "White";
}

static MasterAI19 makeAI(int depth, Color aiColor = Color::Black, int radius = 1)
{
	Logger::info("AI", "makeAI: depth=" + std::to_string(depth) + " radius=" + std::to_string(radius) +
	                       " aiColor=" + colorStr(aiColor));
	MasterAI19 ai(depth, radius, aiColor);

	return ai;
}

// ── findBestMove ─────────────────────────────────────────────────────

TEST_CASE("MasterAI: empty board returns center move")
{
	Logger::info("TEST_CASE", "MasterAI: empty board returns no move");
	GameBoard        b    = empty_board();
	SearchPosition19 pos  = posWithSideToMove(b, Color::Black);
	MasterAI19       ai   = makeAI(2, Color::Black);
	t_cell           move = ai.findBestMove(pos, Color::Black);
	CHECK(move.x == 9);
	CHECK(move.y == 9);
}

// ── Terminal win detection ────────────────────────────────────────────────────

TEST_CASE("MasterAI: wins in one when four stones are aligned")
{
	Logger::info("TEST_CASE", "MasterAI: wins in one when four stones are aligned");
	ThreatLine line{ Direction::E, 3, 9, 4, CellStatus::Black, CellStatus::White, -1 };
	GameBoard  b = empty_board();
	buildThreatLine(b, line);
	SearchPosition19 pos  = posWithSideToMove(b, Color::Black);
	MasterAI19       ai   = makeAI(1, Color::Black);
	t_cell           move = ai.findBestMove(pos, Color::Black);
	CHECK(moveIsOneOf(move, winningCells(line)));
}

TEST_CASE("MasterAI: blocks opponent win in one (depth 2)")
{
	Logger::info("TEST_CASE", "MasterAI: blocks opponent win in one (depth 2)");
	ThreatLine line{ Direction::E, 3, 9, 4, CellStatus::Black, CellStatus::White, -1 };
	GameBoard  b(19, Color::White);
	buildThreatLine(b, line);
	SearchPosition19 pos  = posWithSideToMove(b, Color::White);
	MasterAI19       ai   = makeAI(2, Color::White);
	t_cell           move = ai.findBestMove(pos, Color::White);
	CHECK(moveIsOneOf(move, winningCells(line)));
}

// ── Evaluation-driven choice ──────────────────────────────────────────────────

TEST_CASE("MasterAI: extends three to four rather than a quiet move (depth 1)")
{
	MasterAI19 ai = makeAI(1, Color::Black);

	SUBCASE("horizontal")
	{
		ThreatLine three{ Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, -1 };
		GameBoard  b = empty_board();
		buildThreatLine(b, three);
		SearchPosition19 pos  = posWithSideToMove(b, Color::Black);
		t_cell           move = ai.findBestMove(pos, Color::Black);
		CHECK(moveIsOneOf(move, extendToFourCells(three)));
	}
	SUBCASE("vertical")
	{
		ThreatLine three{ Direction::S, 9, 7, 3, CellStatus::Black, CellStatus::White, -1 };
		GameBoard  b = empty_board();
		buildThreatLine(b, three);
		SearchPosition19 pos  = posWithSideToMove(b, Color::Black);
		t_cell           move = ai.findBestMove(pos, Color::Black);
		CHECK(moveIsOneOf(move, extendToFourCells(three)));
	}
	SUBCASE("diagonal SE")
	{
		ThreatLine three{ Direction::SE, 7, 9, 3, CellStatus::Black, CellStatus::White, -1 };
		GameBoard  b = empty_board();
		buildThreatLine(b, three);
		SearchPosition19 pos  = posWithSideToMove(b, Color::Black);
		t_cell           move = ai.findBestMove(pos, Color::Black);
		CHECK(moveIsOneOf(move, extendToFourCells(three)));
	}
	SUBCASE("diagonal NE")
	{
		ThreatLine three{ Direction::NE, 7, 11, 3, CellStatus::Black, CellStatus::White, -1 };
		GameBoard  b = empty_board();
		buildThreatLine(b, three);
		SearchPosition19 pos  = posWithSideToMove(b, Color::Black);
		t_cell           move = ai.findBestMove(pos, Color::Black);
		CHECK(moveIsOneOf(move, extendToFourCells(three)));
	}
}

TEST_CASE("MasterAI: blocks open-four threat at depth 2")
{
	Logger::info("TEST_CASE", "MasterAI: blocks open-four threat at depth 2");
	MasterAI19 ai = makeAI(2, Color::White);

	SUBCASE("horizontal")
	{
		Logger::info("SUBCASE", "horizontal");
		ThreatLine line{ Direction::E, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 };
		GameBoard  b = empty_board();
		buildThreatLine(b, line);
		SearchPosition19 pos  = posWithSideToMove(b, Color::White);
		t_cell           move = ai.findBestMove(pos, Color::White);
		CHECK(moveIsOneOf(move, blockCells(line)));
	}

	SUBCASE("vertical")
	{
		Logger::info("SUBCASE", "vertical");
		ThreatLine line{ Direction::S, 9, 7, 3, CellStatus::Black, CellStatus::White, 0 };
		GameBoard  b = empty_board();
		buildThreatLine(b, line);
		SearchPosition19 pos  = posWithSideToMove(b, Color::White);
		t_cell           move = ai.findBestMove(pos, Color::White);
		CHECK(moveIsOneOf(move, blockCells(line)));
	}

	SUBCASE("diagonal SE")
	{
		Logger::info("SUBCASE", "diagonal SE");
		ThreatLine line{ Direction::SE, 7, 9, 3, CellStatus::Black, CellStatus::White, 0 };
		GameBoard  b = empty_board();
		buildThreatLine(b, line);
		SearchPosition19 pos  = posWithSideToMove(b, Color::White);
		t_cell           move = ai.findBestMove(pos, Color::White);
		CHECK(moveIsOneOf(move, blockCells(line)));
	}

	SUBCASE("diagonal NE")
	{
		Logger::info("SUBCASE", "diagonal NE");
		ThreatLine line{ Direction::NE, 7, 11, 3, CellStatus::Black, CellStatus::White, 0 };
		GameBoard  b = empty_board();
		buildThreatLine(b, line);
		SearchPosition19 pos  = posWithSideToMove(b, Color::White);
		t_cell           move = ai.findBestMove(pos, Color::White);
		CHECK(moveIsOneOf(move, blockCells(line)));
	}
}

TEST_CASE("MasterAI: findBestMove skips a double-three and returns another legal cell")
{
	MoveGenerator19 gen(2);

	SUBCASE("plain double-three")
	{
		GameBoard b = empty_board();
		place(b, 3, 9, CellStatus::Black);
		place(b, 4, 9, CellStatus::Black);
		place(b, 5, 7, CellStatus::Black);
		place(b, 5, 8, CellStatus::Black);
		SearchPosition19 pos  = posWithSideToMove(b, Color::Black);
		MasterAI19       ai   = makeAI(2, Color::Black);
		const t_cell     move = ai.findBestMove(pos, Color::Black);
		CHECK_FALSE((move.x == 5 && move.y == 9));
		CHECK(move.x >= 0);
		CHECK(gen.isLegalMove(pos.board(), move.x, move.y, Color::Black));
	}

	SUBCASE("cross that is also a double-three")
	{
		GameBoard b = boardFromAscii(
			{
				"...................",
				"....B..............",
				".....B.............",
				"..BB.............BB",
				".....B.............",
				"....B..............",
			},
			Color::Black);
		SearchPosition19 pos  = posWithSideToMove(b, Color::Black);
		MasterAI19       ai   = makeAI(2, Color::Black);
		const t_cell     move = ai.findBestMove(pos, Color::Black);
		CHECK(move.x >= 0);
		CHECK(gen.isLegalMove(pos.board(), move.x, move.y, Color::Black));
	}
}

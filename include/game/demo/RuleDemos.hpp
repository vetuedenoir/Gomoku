#ifndef RULE_DEMOS_HPP
#define RULE_DEMOS_HPP

#include "game/board/GameBoard.hpp"
#include "game/contracts/contracts.hpp"
#include <vector>

struct RuleDemoStone
{
	int        col;
	int        row;
	CellStatus color;
};

struct RuleDemo
{
	const char*                title;
	const char*                hint;
	Color                      toMove;
	int                        keyCol;
	int                        keyRow;
	std::vector<RuleDemoStone> stones;
};

inline GameBoard makeDemoBoard(const RuleDemo& demo, int size = 19)
{
	GameBoard board(size, demo.toMove);
	for (const RuleDemoStone& stone : demo.stones)
		board.placeStoneOfColor(stone.col, stone.row, stone.color);
	return board;
}

inline const RuleDemo& demoDoubleThree()
{
	static const RuleDemo demo{
		"Double-three",
		"Black: (5,9) is a double-three — the move is refused.",
		Color::Black,
		5,
		9,
		{
			{ 3, 9, CellStatus::Black },
			{ 4, 9, CellStatus::Black },
			{ 5, 7, CellStatus::Black },
			{ 5, 8, CellStatus::Black },
		},
	};
	return demo;
}

inline const RuleDemo& demoDoubleThreeCapture()
{
	static const RuleDemo demo{
		"Double-three + capture",
		"Black: (5,9) is a double-three but it captures — the move is legal.",
		Color::Black,
		5,
		9,
		{
			{ 3, 9, CellStatus::Black },
			{ 4, 9, CellStatus::Black },
			{ 5, 7, CellStatus::Black },
			{ 5, 8, CellStatus::Black },
			{ 6, 9, CellStatus::White },
			{ 7, 9, CellStatus::White },
			{ 8, 9, CellStatus::Black },
		},
	};
	return demo;
}

inline const RuleDemo& demoBreakableFive()
{
	static const RuleDemo demo{
		"Breakable five",
		"Black: complete the five at (7,5). It will not win yet — White can capture at (5,4).",
		Color::Black,
		7,
		5,
		{
			{ 3, 5, CellStatus::Black },
			{ 4, 5, CellStatus::Black },
			{ 5, 5, CellStatus::Black },
			{ 6, 5, CellStatus::Black },
			{ 5, 6, CellStatus::Black },
			{ 5, 7, CellStatus::White },
		},
	};
	return demo;
}

#endif

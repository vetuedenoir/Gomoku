#include "../include/bitboard.hpp"


#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"

#define BLACK_STONE "\033[1;30m"   // bright black (gray)
#define WHITE_STONE "\033[1;37m"   // bright white
#define EMPTY_CELL  "\033[2;37m"   // dim white

void print_bb_19_colored(t_BWBoard19 &bw)
{
    std::string str;

    str += "\n - - - - - - - - - - - - - - - - - - -\n";
    for (uint64_t y = 0; y < 19; y++)
    {
        str += '|';
        for (uint64_t x = 0; x < 19; x++)
        {
            uint64_t idx = index(x, y);

            if (get(bw.black, idx))
                str += BLACK_STONE "B " RESET;
            else if (get(bw.white, idx))
                str += WHITE_STONE "W " RESET;
            else
                str += EMPTY_CELL ". " RESET;
        }
        str += "|\n";
    }
    str += " - - - - - - - - - - - - - - - - - - -\n";

    std::cout << str << std::endl;
}

void print_bb_19(t_BWBoard19 &bw)
{
	std::string str;

	str += "\n - - - - - - - - - - - - - - - - - - -\n";
	for (uint64_t y = 0; y < 19; y++)
	{
		str += '|';
		for (uint64_t x = 0; x < 19; x++)
		{
			if (get(bw.black, index(x, y)) || get(bw.white, index(x, y)))
			{
				if (get(bw.black, index(x, y)))
					str += "B ";
				else if (get(bw.white, index(x, y)))
					str += "W ";
			}	
			else
				str += "0 ";
		}
		str += "|\n";
	}
	str += " - - - - - - - - - - - - - - - - - - -\n";
	std::cout << str << std::endl;
}

t_BWBoard19 GameBoard_to_bitboard(const GameBoard &board)
{
	t_BWBoard19 bw_board = {};

	for (int y = 0; y < board.getSize(); y++)
	{
		for (int x = 0; x < board.getSize(); x++)
		{
			auto cell = board.getCell(x, y);

			if (cell == CellStatus::Black)
				set(bw_board.black, x, y);
			else if (cell == CellStatus::White)
				set(bw_board.white, x, y);
		}
	}
	return bw_board;
}


void	print_binaire64(const uint64_t ut)
{
	for (size_t i = 0; i < 64; i++)
	{	
		if (i % 8 == 0)
			std::cout << ' ';
		if (ut & (1ULL << i))
			std::cout << 1;
		else
			std::cout << 0;
	}
	std::cout << std::endl;
}

void	print_binaire_board19(const bitboard19 bb)
{
	for (size_t i = 0; i < 6; i++)
		print_binaire64(bb[i]);
}


void	pattern_universel(bitboard19 bb, int size, int pos, int strides)
{
	int	y = pos / 19;
	int	x = pos % 19;

	int	start = y * 20 + x;
	
	for (int i = 0; i < size; i++)
	{
		int bit = start + i * strides;

		int idx = bit / 64;
		int	offset = bit % 64;

		bb[idx] |= (1ULL << offset);
	}
}


// creer des filtre pour tester la condition de victoire.
// prototype, on peut utiliser ce systeme pour des 4 ou des 3 a la suite.
// toujour en horizontal pour le moment
const int	MAX_WINNING_MASK = 1020;

void	test_pattern(bitboard19	winning_mask[MAX_WINNING_MASK])
{
	// uint64_t	raw5 = 0XF800000000000000;
	// uint64_t	raw5 = 0b11111;
	// bitboard19	bb = {};
	// bitboard19	bw = {};
	// t_BWBoard19 board = {};
	int	pattern_height = 1;
	int	pattern_lenght = 5;
	int total_mask = 0;

	// on commence par les detection horizontal
	for (int i = 0; i <= 361 - pattern_lenght; i++)
	{
		// std::memset(bb, 0, sizeof(bb));
		if ((i % 19) + pattern_lenght < 20)
		{
			bitboard19 bb = {};
			// std::memset(bb, 0, sizeof(bb));

			// pattern_creation(bb, raw5, pattern_size, i);
			pattern_universel(bb, 5, i, 1); // pattern horizontal;
			// std::memcpy(&winning_mask[total_mask * 6], bb, sizeof(bb));
			for (int x = 0; x < 6; x++)
				winning_mask[total_mask][x] = bb[x];
			total_mask++;
			// print_binaire_board19(bb);
			// print_bb_19(board);
			// std::cout << std::endl;
		}
	}

	// les patternes verticales
	pattern_height = 5;
	for (int i = 0; i < 361 - (pattern_height - 1) * 19; i++)
	{
		bitboard19 bb = {};
		pattern_universel(bb, 5, i, 20);
		for (int x = 0; x < 6; x++)
				winning_mask[total_mask][x] = bb[x];
		total_mask++;
	}

	// patternes diagonale droite
	for (int i = 0; i < 361 - (pattern_height - 1) * 19; i++)
	{
		if ((i % 19) + pattern_lenght < 20)
		{
			bitboard19 bb = {};
			pattern_universel(bb, 5, i, 21);
			for (int x = 0; x < 6; x++)
				winning_mask[total_mask][x] = bb[x];
			total_mask++;
		}
	}

	// patternes diagonale gauche
	for (int i = 0; i < 361 - (pattern_height - 1) * 19; i++)
	{
		if ((i % 19) + pattern_lenght < 20)
		{
			bitboard19 bb = {};
			pattern_universel(bb, 5, i + 4, 19);
			for (int x = 0; x < 6; x++)
				winning_mask[total_mask][x] = bb[x];
			total_mask++;
		}
	}

	// std::cout << "Total mask = " << total_mask << std::endl;
	// for (int i = 0; i < MAX_WINNING_MASK; i++)
	// {
	// 	for (int x = 0; x < 6; x++)
	// 		board.black[x] = winning_mask[i][x];
	// 	print_bb_19(board);
	// 	std::cout << std::endl;
	// }
}


bool	isWin(const bitboard19 bboard, const bitboard19 winning_mask[MAX_WINNING_MASK])
{
	for (int i = 0 ; i < MAX_WINNING_MASK; i++)
	{
		bool win = true;
		for (int x = 0; x < 6; x++)
		{
			if ((winning_mask[i][x] & bboard[x]) != winning_mask[i][x])
			{
				win = false;
				break;
			}
		}
		if (win)
			return (win);
	}
	return (false);
}


// Version ultra rapide avec table de lookup, la plus efficace a ce jour

#include <stdint.h>
#include <string.h>


// Directions
#define DIR_HORIZ 0
#define DIR_VERT  1
#define DIR_DIAG_G 2  //
#define DIR_DIAG_D 3  //

typedef struct {
    uint64_t masks[5][6];  // 5 masks × 6 uint64_t = 240 bytes
    uint8_t count;          // 1 byte (au lieu de int)
    uint8_t padding[7];     // Alignement à 8 bytes
} MaskList;  // 248 bytes total

// Stockage global de tous les masks (pour référence)
bitboard19 all_masks[MAX_WINNING_MASK];
int total_masks = 0;


#define IDX(x, y) ((y) * 19 + (x))

// Table 1D uniquement
MaskList lookup_table[361][4];  // 19*19 = 361

// Construction modifiée
void add_mask_to_lookup(int mask_index, int start_pos, int stride) {
    int dir;
    if (stride == 1) dir = DIR_HORIZ;
    else if (stride == 20) dir = DIR_VERT;
    else if (stride == 21) dir = DIR_DIAG_G;
    else if (stride == 19) dir = DIR_DIAG_D;
    else return;
    
    for (int i = 0; i < 5; i++) {
        int pos = start_pos + i * stride;
        int y = pos / 20;
        int x = pos % 20;
        int idx = IDX(x, y);  // Calcul une fois
        
        MaskList *list = &lookup_table[idx][dir];
        
        for (int w = 0; w < 6; w++) {
            list->masks[list->count][w] = all_masks[mask_index][w];
        }
        list->count++;
    }
}

void build_lookup_table(void) {
    // Initialise la table
    memset(lookup_table, 0, sizeof(lookup_table));
    total_masks = 0;
    
    // 1. Horizontal (stride = 1)
    for (int y = 0; y < 19; y++) {
        for (int x = 0; x <= 14; x++) {
            bitboard19 mask = {0};
            int start_pos = y * 20 + x;
            for (int i = 0; i < 5; i++) {
                int bit = start_pos + i;
                int idx = bit >> 6;
                int offset = bit & 63;
                mask[idx] |= (1ULL << offset);
            }
            // Stocke le mask
            for (int w = 0; w < 6; w++) {
                all_masks[total_masks][w] = mask[w];
            }
            add_mask_to_lookup(total_masks, start_pos, 1);
            total_masks++;
        }
    }
    
    // 2. Vertical (stride = 20)
    for (int x = 0; x < 19; x++) {
        for (int y = 0; y <= 14; y++) {
            bitboard19 mask = {0};
            int start_pos = y * 20 + x;
            for (int i = 0; i < 5; i++) {
                int bit = start_pos + i * 20;
                int idx = bit >> 6;
                int offset = bit & 63;
                mask[idx] |= (1ULL << offset);
            }
            for (int w = 0; w < 6; w++) {
                all_masks[total_masks][w] = mask[w];
            }
            add_mask_to_lookup(total_masks, start_pos, 20);
            total_masks++;
        }
    }
    
    // 3. Diagonale \ (stride = 21)
    for (int y = 0; y <= 14; y++) {
        for (int x = 0; x <= 14; x++) {
            bitboard19 mask = {0};
            int start_pos = y * 20 + x;
            for (int i = 0; i < 5; i++) {
                int bit = start_pos + i * 21;
                int idx = bit >> 6;
                int offset = bit & 63;
                mask[idx] |= (1ULL << offset);
            }
            for (int w = 0; w < 6; w++) {
                all_masks[total_masks][w] = mask[w];
            }
            add_mask_to_lookup(total_masks, start_pos, 21);
            total_masks++;
        }
    }
    
    // 4. Diagonale / (stride = 19)
    for (int y = 0; y <= 14; y++) {
        for (int x = 4; x < 19; x++) {
            bitboard19 mask = {0};
            int start_pos = y * 20 + x;
            for (int i = 0; i < 5; i++) {
                int bit = start_pos + i * 19;
                int idx = bit >> 6;
                int offset = bit & 63;
                mask[idx] |= (1ULL << offset);
            }
            for (int w = 0; w < 6; w++) {
                all_masks[total_masks][w] = mask[w];
            }
            add_mask_to_lookup(total_masks, start_pos, 19);
            total_masks++;
        }
    }

}


// Vérification ultra rapide
static inline int isWin_ultra(const bitboard19 bboard, int x, int y) {
    int idx = IDX(x, y);  // Précalculé
    
    t_BWBoard19 board = {};

    for (int dir = 0; dir < 4; dir++) {
        MaskList *list = &lookup_table[idx][dir];
        int count = list->count;
        
        for (int i = 0; i < count; i++) {
            uint64_t (*mask)[6] = &list->masks[i];
            board.black[0] = (*mask)[0];
            board.black[1] = (*mask)[1];
            board.black[2] = (*mask)[2];
            board.black[3] = (*mask)[3];
            board.black[4] = (*mask)[4];
            board.black[5] = (*mask)[5];
            
            print_bb_19(board);
            
            if ((((*mask)[0] & bboard[0]) != (*mask)[0]) ||
                (((*mask)[1] & bboard[1]) != (*mask)[1]) ||
                (((*mask)[2] & bboard[2]) != (*mask)[2]) ||
                (((*mask)[3] & bboard[3]) != (*mask)[3]) ||
                (((*mask)[4] & bboard[4]) != (*mask)[4]) ||
                (((*mask)[5] & bboard[5]) != (*mask)[5])) {
                continue;
            }
            return 1;
        }
    }
    return 0;
}


void	test_bitboard(const GameBoard& board, int x, int y)
{
	t_BWBoard19 bitboard = GameBoard_to_bitboard(board);

	print_bb_19(bitboard);

	// bitboard19	winning_mask[MAX_WINNING_MASK] = {};

	// test_pattern(winning_mask);
	// if (isWin(bitboard.black, winning_mask))
	// 	std::cout << "les noires ont gagne !!!" << std::endl;
	// if (isWin(bitboard.white, winning_mask))
	// 	std::cout << "les blanches ont gagne !!!" << std::endl;

    build_lookup_table();
    if (isWin_ultra(bitboard.black, x, y))
        std::cout << "les noires ont gagne !!!" << std::endl;
    if (isWin_ultra(bitboard.white, x, y))
        std::cout << "les blanches ont gagne !!!" << std::endl;

}

// ── capture detection ─────────────────────────────────────────────────────────

bool detect_captures(const t_BWBoard19& board, int col, int row, Color attackerColor, bitboard19& capturedMask)
{
    const bitboard19& attacker = (attackerColor == Color::Black) ? board.black : board.white;
    const bitboard19& victime = (attackerColor == Color::Black) ? board.white : board.black;

    bool captured = false;

    for (Direction dir : LINE_DIRS)
    {
		for (int sign : {-1, 1})
		{
			int stepX = sign * dx(dir);
			int stepY = sign * dy(dir);

			int x1 = col + 1 * stepX, y1 = row + 1 * stepY;
			int x2 = col + 2 * stepX, y2 = row + 2 * stepY;
			int x3 = col + 3 * stepX, y3 = row + 3 * stepY;

			if (!in_board(x1, y1) || !in_board(x2, y2) || !in_board(x3, y3))
				continue;

            int innerVictim = index(x1, y1);
            int outerVictim = index(x2, y2);
            int flank       = index(x3, y3);

            if (get(victime, innerVictim) && get(victime, outerVictim) && get(attacker, flank))
            {
                result[innerVictim >> 6] |= (1ULL << (innerVictim & 63));
                result[outerVictim >> 6] |= (1ULL << (outerVictim & 63));
                captured = true;
            }
        }
    }

    return captured;
}

void apply_captures(t_BWBoard19& board, const bitboard19 captured, Color attacker)
{
    bitboard19& victimBitboard = (attacker == Color::Black) ? board.white : board.black;
    for (int i = 0; i < 6; i++)
        victimBitboard[i] &= ~captured[i];
}

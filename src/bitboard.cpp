#include "../include/bitboard.hpp"

// testing

inline int index(int x, int y) {
	return y * 20 + x;
}


// set le bit a 1 a la position x, y
inline void set(bitboard19 &bb, int x, int y)
{
	int idx = index(x, y);
	bb[idx / 64] |= (1ULL << (idx % 64));
}

inline bool get(const bitboard19& bb, int i) {
	return (bb[i >> 6] >> (i & 63)) & 1ULL;
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

bool	five_detection(void)
{
	// les 5 premiers bit de poids fort initialiser a 1
	// __uint64_t filtre = (31ULL << 59);
	__uint64_t filtre5 = 0XF800000000000000;
	__uint64_t filtre6 = 0XFC00000000000000;
	__uint64_t	mask_line = 0XFFFFEFFFFEFFFFEF;

	u_int64_t tab_int[64 - 5];

	for (size_t i = 0; i < 64 -5; i++)
	{
		if ((i + 5) % 20 == 0)
		{
			size_t x = 0;
			for (x = 0; x < 5; x++)
			{
				tab_int[i + x] = (filtre6 >> (i + x)) & mask_line;
				print_binaire64(tab_int[i + x]);
			}
			i += x;
		}
		else
		{
			tab_int[i] = filtre5 >> i;
			print_binaire64(tab_int[i]);
		}
	}	

	std::cout << "mask line " << std::endl;
	print_binaire64(mask_line);
	return true;
}


// creer un pattern horizontal a la position voulu, on creer un mask global 
void	pattern_creation(bitboard19 bb, uint64_t pattern, int size, int pos)
{
	// ne gere pas les depassement de lignes. le patterne depasse sur l'autre ligne.
	// la fonction appelante doit s'ocuper de verifier si la position est correcte.
	int line = pos / 20;
	pos += line;

	int idx = pos / 64;
	int	offset = pos % 64;

	bb[idx] |= pattern << offset;
	
	if (offset > (64 - size))
	{
		bb[idx + 1] |= pattern >> (64 - offset); 
	}
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

void	test_bitboard(const GameBoard& board)
{
	t_BWBoard19 bitboard = GameBoard_to_bitboard(board);

	print_bb_19(bitboard);
	// std::cout << "bitboard black" << std::endl;
	// print_binaire_board19(bitboard.black);
	// std::cout << "\nbitboard white" << std::endl;
	// print_binaire_board19(bitboard.white);
	// std::cout << std::endl;

	// five_detection();
	bitboard19	winning_mask[MAX_WINNING_MASK] = {};

	test_pattern(winning_mask);
	if (isWin(bitboard.black, winning_mask))
		std::cout << "les noires ont gagne !!!" << std::endl;
	if (isWin(bitboard.white, winning_mask))
		std::cout << "les blanches ont gagne !!!" << std::endl;

}



// les amelioration possible >> 
// Vérifie uniquement les masks qui passent par la position jouée
bool isWin_fast(const bitboard19 bboard, const bitboard19 winning_mask[MAX_WINNING_MASK], 
                int last_pos, int last_x, int last_y) {
    
    // Pour chaque direction, vérifie seulement les masks qui incluent last_pos
    // Au lieu de 700 masks, tu vérifies seulement ~20-30 masks !
    
    int start_positions[4][5]; // Positions de départ possibles pour chaque direction
    
    // Horizontal : last_x - 4 à last_x (mais gardé dans 0-14)
    for (int start_x = std::max(0, last_x - 4); start_x <= std::min(14, last_x); start_x++) {
        int mask_start = last_y * 20 + start_x;
        // Vérifie si ce mask existe (start_x + 4 < 19)
        if (start_x + 4 < 19) {
            if (check_mask_at(bboard, winning_mask, mask_start, 1)) return true;
        }
    }
    
    // Vertical : last_y - 4 à last_y
    for (int start_y = std::max(0, last_y - 4); start_y <= std::min(14, last_y); start_y++) {
        int mask_start = start_y * 20 + last_x;
        if (check_mask_at(bboard, winning_mask, mask_start, 20)) return true;
    }
    
    // Diagonale \ : last_y - 4 à last_y, last_x - 4 à last_x
    for (int offset = -4; offset <= 0; offset++) {
        int start_x = last_x + offset;
        int start_y = last_y + offset;
        if (start_x >= 0 && start_x < 19 && start_y >= 0 && start_y < 19 && start_x + 4 < 19 && start_y + 4 < 19) {
            int mask_start = start_y * 20 + start_x;
            if (check_mask_at(bboard, winning_mask, mask_start, 21)) return true;
        }
    }
    
    // Diagonale / : last_y - 4 à last_y, last_x + 4 à last_x
    for (int offset = -4; offset <= 0; offset++) {
        int start_x = last_x - offset;
        int start_y = last_y + offset;
        if (start_x >= 0 && start_x < 19 && start_y >= 0 && start_y < 19 && start_x - 4 >= 0 && start_y + 4 < 19) {
            int mask_start = start_y * 20 + (start_x - 4);
            if (check_mask_at(bboard, winning_mask, mask_start, 19)) return true;
        }
    }
    
    return false;
}

bool check_mask_at(const bitboard19 bboard, const bitboard19 winning_mask[MAX_WINNING_MASK],
                   int mask_start, int stride) {
    // Calcule un hash ou cherche directement dans un dictionnaire
    // Version simple : cherche le mask correspondant
    for (int i = 0; i < MAX_WINNING_MASK; i++) {
        // Vérifie si le mask commence à cette position avec ce stride
        // (tu devrais plutôt avoir une table de correspondance directe)
    }
}


// solution avec hash

// Structure pour trouver un mask en O(1)


#include <stdint.h>
#include <string.h>

typedef uint64_t bitboard19[6];

// Table de lookup : pour chaque case et direction, l'index du mask
int lookup_table[19][19][4];  // [y][x][direction]
// Stockage des masks
bitboard19 win_masks[800];  // Assez grand pour tous les masks
int total_masks = 0;

// Directions
#define DIR_HORIZ 0
#define DIR_VERT  1
#define DIR_DIAG1 2  // '\'
#define DIR_DIAG2 3  // '/'

typedef struct {
    int start_pos;
    int stride;
    bitboard19 mask;
} WinMask;

WinMask win_masks_by_key[19][19][4]; // [y][x][direction] = mask qui passe par (x,y)
// directions: 0=horizontal, 1=vertical, 2=diag\, 3=diag/
// Ajoute un mask à la table et enregistre quelles cases il couvre
void register_mask(bitboard19 mask, int start_pos, int stride) {
    // Stocke le mask
    for (int w = 0; w < 6; w++) {
        win_masks[total_masks][w] = mask[w];
    }
    
    // Trouve toutes les positions couvertes par ce mask
    for (int i = 0; i < 5; i++) {
        int pos = start_pos + i * stride;
        int y = pos / 20;      // Attention: utilisation du stride avec padding
        int x = pos % 20;
        
        // Détermine la direction
        int dir;
        if (stride == 1) dir = DIR_HORIZ;
        else if (stride == 20) dir = DIR_VERT;
        else if (stride == 21) dir = DIR_DIAG1;
        else if (stride == 19) dir = DIR_DIAG2;
        else continue;
        
        // Enregistre l'index (si pas déjà fait)
        if (lookup_table[y][x][dir] == -1) {
            lookup_table[y][x][dir] = total_masks;
        }
    }
    
    total_masks++;
}

// Construit toute la table
void build_lookup_table(void) {
    // Initialise la table à -1
    for (int y = 0; y < 19; y++) {
        for (int x = 0; x < 19; x++) {
            for (int d = 0; d < 4; d++) {
                lookup_table[y][x][d] = -1;
            }
        }
    }
    
    total_masks = 0;
    
    // Horizontal (stride = 1)
    for (int y = 0; y < 19; y++) {
        for (int x = 0; x <= 14; x++) {
            bitboard19 mask = {0};
            int start_pos = y * 20 + x;
            for (int i = 0; i < 5; i++) {
                int bit = start_pos + i;
                int idx = bit / 64;
                int offset = bit % 64;
                mask[idx] |= (1ULL << offset);
            }
            register_mask(mask, start_pos, 1);
        }
    }
    
    // Vertical (stride = 20)
    for (int x = 0; x < 19; x++) {
        for (int y = 0; y <= 14; y++) {
            bitboard19 mask = {0};
            int start_pos = y * 20 + x;
            for (int i = 0; i < 5; i++) {
                int bit = start_pos + i * 20;
                int idx = bit / 64;
                int offset = bit % 64;
                mask[idx] |= (1ULL << offset);
            }
            register_mask(mask, start_pos, 20);
        }
    }
    
    // Diagonale \ (stride = 21)
    for (int y = 0; y <= 14; y++) {
        for (int x = 0; x <= 14; x++) {
            bitboard19 mask = {0};
            int start_pos = y * 20 + x;
            for (int i = 0; i < 5; i++) {
                int bit = start_pos + i * 21;
                int idx = bit / 64;
                int offset = bit % 64;
                mask[idx] |= (1ULL << offset);
            }
            register_mask(mask, start_pos, 21);
        }
    }
    
    // Diagonale / (stride = 19)
    for (int y = 0; y <= 14; y++) {
        for (int x = 4; x < 19; x++) {
            bitboard19 mask = {0};
            int start_pos = y * 20 + x;
            for (int i = 0; i < 5; i++) {
                int bit = start_pos + i * 19;
                int idx = bit / 64;
                int offset = bit % 64;
                mask[idx] |= (1ULL << offset);
            }
            register_mask(mask, start_pos, 19);
        }
    }
}

// Vérifie si un mask spécifique est entièrement dans le bitboard
int mask_is_present(const bitboard19 bboard, int mask_index) {
    for (int w = 0; w < 6; w++) {
        if ((win_masks[mask_index][w] & bboard[w]) != win_masks[mask_index][w]) {
            return 0;
        }
    }
    return 1;
}

// Vérification après un coup (à appeler avec la position du dernier coup)
int isWin_fast(const bitboard19 bboard, int last_x, int last_y) {
    // Vérifie les 4 directions
    for (int dir = 0; dir < 4; dir++) {
        int mask_idx = lookup_table[last_y][last_x][dir];
        if (mask_idx != -1) {
            if (mask_is_present(bboard, mask_idx)) {
                return 1;
            }
        }
    }
    return 0;
}

// Version alternative avec accès direct sans fonction
int isWin_instant(const bitboard19 bboard, int last_x, int last_y) {
    // Horizontal
    int idx = lookup_table[last_y][last_x][DIR_HORIZ];
    if (idx != -1) {
        int win = 1;
        for (int w = 0; w < 6; w++) {
            if ((win_masks[idx][w] & bboard[w]) != win_masks[idx][w]) {
                win = 0;
                break;
            }
        }
        if (win) return 1;
    }
    
    // Vertical
    idx = lookup_table[last_y][last_x][DIR_VERT];
    if (idx != -1) {
        int win = 1;
        for (int w = 0; w < 6; w++) {
            if ((win_masks[idx][w] & bboard[w]) != win_masks[idx][w]) {
                win = 0;
                break;
            }
        }
        if (win) return 1;
    }
    
    // Diagonale \
    idx = lookup_table[last_y][last_x][DIR_DIAG1];
    if (idx != -1) {
        int win = 1;
        for (int w = 0; w < 6; w++) {
            if ((win_masks[idx][w] & bboard[w]) != win_masks[idx][w]) {
                win = 0;
                break;
            }
        }
        if (win) return 1;
    }
    
    // Diagonale /
    idx = lookup_table[last_y][last_x][DIR_DIAG2];
    if (idx != -1) {
        int win = 1;
        for (int w = 0; w < 6; w++) {
            if ((win_masks[idx][w] & bboard[w]) != win_masks[idx][w]) {
                win = 0;
                break;
            }
        }
        if (win) return 1;
    }
    
    return 0;
}

void test_hash_table(void)
{
	// Une fois au démarrage du jeu
    build_lookup_table();
    
    // Pendant le jeu, après chaque coup
    bitboard19 noir = {0};
    // ... placer des pierres ...
    
    int last_x = 7, last_y = 10;  // Dernier coup joué
    
    if (isWin_fast(noir, last_x, last_y)) {
        printf("Victoire !\n");
    }

}
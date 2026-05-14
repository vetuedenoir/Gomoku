#ifndef MASTER_AI_HPP
# define MASTER_AI_HPP

# include "Gomoku.hpp"
#include "bitboard/bitboard.hpp"
# include <variant>

using t_BWBoard = std::variant<Bitboard19, Bitboard15>;
// a utiliser dans tout les fichier qui utilise les structures de données du bitboard.

// master IA : s'occupe juste de l'IA , de trouver la bonne position et 
//pas de verifier si les mouvement de l'utilisateur son valide.

// class qui verifier les positions et la legalite des coups utilisateur.
    // cette classe contient le bitboard de la verite,
    // quand masterIA a choisie un move, il le transmet cette classe.
    // cette class transmet l'etat actuel du board a l'interface utilisateur pour l'affichage et les interactions.

class MasterIA
{
public:

    virtual ~MasterIA() = default;
    virtual void think(const GameBoard& board) = 0;

private:
    int _black_stones_captured;
    int _white_stones_captured;
    
    // contient Move_Generator
    virtual bool is_legal_move(const GameBoard& board, int col, int row) const = 0;

    virtual bool is_winning_move(const GameBoard& board, int col, int row) const = 0;
    virtual bool check_win(const GameBoard& board) = 0;
    virtual void create_patterns(void) = 0;

    // int   minimax((bitboard19 ou bitboard15 board, int depth, int alpha, int beta, int player))
};

#endif // MASTER_AI_HPP


// cette classe doit etre au courant de la phase de jeu.
// on porra implemente plus tard une adaptation pour les opening, pour l'instant 
// on s'occupe juste de la phase normal.


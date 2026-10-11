
#include "transpositiontable.h"
#include "position.h"

TTEntry transposition_table[TT_SIZE];

int TranspositionTable::lookup(uint64_t key,
                               int      depth,
                               int      alpha,
                               int      beta,
                               int      ply_from_root) {

    TTEntry *entry = &transposition_table[key & (TT_SIZE - 1)];

    if (entry->key == key && entry->depth >= depth) {

        int eval = entry->eval;

        if (is_win(eval))
            eval -= ply_from_root;
        else if (is_loss(eval))
            eval += ply_from_root;

        if (entry->flag == EXACT)
            return eval;

        if (entry->flag == UPPER_BOUND && eval <= alpha)
            return alpha;

        if (entry->flag == LOWER_BOUND && eval >= beta)
            return beta;
    }

    return NO_EVAL;
}

void TranspositionTable::record(uint64_t  key,
                                uint8_t   depth,
                                BoundType flag,
                                int       eval,
                                Move      best_move,
                                int       ply_from_root) {

    if (is_win(eval))
        eval += ply_from_root;
    else if (is_loss(eval))
        eval -= ply_from_root;

    TTEntry *e = &transposition_table[key & (TT_SIZE - 1)];

    e->key       = key;
    e->depth     = depth;
    e->flag      = flag;
    e->eval      = eval;
    e->best_move = best_move;
}

Move TranspositionTable::lookup_move(uint64_t key) {
    TTEntry *e = &transposition_table[key & (TT_SIZE - 1)];
    return e->key == key ? e->best_move : NO_MOVE;
}

void TranspositionTable::clear() {
    memset(transposition_table, 0, TT_SIZE * sizeof(TTEntry));
}

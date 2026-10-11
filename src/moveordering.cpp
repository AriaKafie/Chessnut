
#include <limits.h>

#include "evaluation.h"
#include "movelist.h"
#include "position.h"

constexpr Score BAD_QUIET_MIN      = (UINT16_MAX / 4) * 0;
constexpr Score BAD_CAPTURE_MIN    = (UINT16_MAX / 4) * 1;
constexpr Score GOOD_QUIET_MIN     = (UINT16_MAX / 4) * 2;
constexpr Score GOOD_CAPTURE_MIN   = (UINT16_MAX / 4) * 3;

constexpr Score BAD_QUIET_BASE     = BAD_QUIET_MIN    + (UINT16_MAX / 8);
constexpr Score BAD_CAPTURE_BASE   = BAD_CAPTURE_MIN  + (UINT16_MAX / 8);
constexpr Score GOOD_QUIET_BASE    = GOOD_QUIET_MIN   + (UINT16_MAX / 8);
constexpr Score GOOD_CAPTURE_BASE  = GOOD_CAPTURE_MIN + (UINT16_MAX / 8);

constexpr Score MAX_SCORE          = UINT16_MAX;
constexpr Score SEEN_BY_PAWN_MALUS = 50;

template<Color Us, Color Them>
void CaptureList<Us, Them>::insertion_sort() {

    for (EMove *p = moves + 1; p < last; p++) {

        EMove tmp = *p, *q;

        for (q = p - 1; q >= moves && score_of(*q) < score_of(tmp); q--)
            *(q + 1) = *q;

        *(q + 1) = tmp;
    }
}

template<Color Us, Color Them>
void CaptureList<Us, Them>::sort()
{
    Bitboard seen_by_pawn = pawn_attacks<Them>(pos.bb(make_piece(Them, PAWN)));

    for (EMove& m : *this)
    {  
        Score score = UINT16_MAX / 2;

        Square    from     = from_sq(m);
        Square    to       = to_sq(m);
        PieceType pt       = pos.piece_type_on(from);
        PieceType captured = pos.piece_type_on(to);

        if (square_bb(to) & seen_by_pawn)
            score -= 500;

        score += piece_weight(captured) - piece_weight(pt) * bool(square_bb(to) & seen_by_enemy);

        set_score(m, score);
    }
  
    insertion_sort();
}

template<Color Us, Color Them>
int MoveList<Us, Them>::partition(int low, int high) {

    Score pivot = score_of(moves[high]);

    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (score_of(moves[j]) >= pivot) {
            i++;
            std::swap(moves[i], moves[j]);
        }
    }

    std::swap(moves[i + 1], moves[high]);

    return i + 1;
}

template<Color Us, Color Them>
void MoveList<Us, Them>::quicksort(int low, int high) {
    if (low < high) {
        int pivot_index = partition(low, high);
        quicksort(low, pivot_index - 1);
        quicksort(pivot_index + 1, high);
    }
}

template<Color Us, Color Them>
void MoveList<Us, Them>::sort() {

    Bitboard seen_by_pawn = pawn_attacks<Them>(pos.bb(make_piece(Them, PAWN)));

    for (EMove& m : *this) {

        if (m == ttmove) {
            set_score(m, MAX_SCORE);
            continue;
        }
        
        Score score;
    
        Square    from     = from_sq(m);
        Square    to       = to_sq(m);
        PieceType pt       = pos.piece_type_on(from);
        PieceType captured = pos.piece_type_on(to);

        if (captured)
        {
            int material_delta = piece_weight(captured) - piece_weight(pt);

            if (square_bb(to) & seen_by_enemy)
                score = (material_delta >= 0 ? GOOD_CAPTURE_BASE : BAD_CAPTURE_BASE) + material_delta;
            else
                score = GOOD_CAPTURE_BASE + material_delta;
        }
        else
        {
            if (m == search_info->killers[0]
             || m == search_info->killers[1])
                score = GOOD_QUIET_BASE;
            else
                score = BAD_QUIET_BASE;

            score += pos.endgame() && pt == KING
                ? (psq_score<Us>(0,  to) - psq_score<Us>(0,  from)) / 2 //(end_king_squares[to] - end_king_squares[from]) / 2
                : (psq_score<Us>(pt, to) - psq_score<Us>(pt, from)) / 2;
        }

        if (type_of(m) == PROMOTION)
            score += promotion_type_of(m);

        if (square_bb(to) & seen_by_pawn)
            score -= SEEN_BY_PAWN_MALUS;

        set_score(m, score);
    }

    quicksort(0, size() - 1);
}

template void MoveList<WHITE>::sort();
template void MoveList<BLACK>::sort();

template void CaptureList<WHITE>::sort();
template void CaptureList<BLACK>::sort();

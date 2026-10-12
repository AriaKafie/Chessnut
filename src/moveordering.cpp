
#include <algorithm>
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

void insertion_sort(EMove* begin, EMove* end) {

    for (EMove *i = begin + 1; i < end; i++) {

        EMove tmp = *i, *j;

        for (j = i - 1; j >= begin && score_of(*j) < score_of(tmp); j--)
            *(j + 1) = *j;

        *(j + 1) = tmp;
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
  
    insertion_sort(moves, last);
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
                ? (psq_score<Us>(0,  to) - psq_score<Us>(0,  from)) / 2 // (end_king_squares[to] - end_king_squares[from]) / 2
                : (psq_score<Us>(pt, to) - psq_score<Us>(pt, from)) / 2;
        }

        if (type_of(m) == PROMOTION)
            score += promotion_type_of(m);

        if (square_bb(to) & seen_by_pawn)
            score -= SEEN_BY_PAWN_MALUS;

        set_score(m, score);
    }

    insertion_sort(moves, last);
}

template void MoveList<WHITE>::sort();
template void MoveList<BLACK>::sort();

template void CaptureList<WHITE>::sort();
template void CaptureList<BLACK>::sort();


#ifndef EVALUATION_H
#define EVALUATION_H

#include "position.h"
#include "types.h"

inline int psq[COLOR_NB][KING + 1][SQUARE_NB];

inline void eval_init() {
    int psq_base[KING + 1][SQUARE_NB] = {
        /*endgame king*/ {
            -50,-40,-30,-20,-20,-30,-40,-50,
            -30,-20,-10,  0,  0,-10,-20,-30,
            -30,-10, 20, 30, 30, 20,-10,-30,
            -30,-10, 30, 40, 40, 30,-10,-30,
            -30,-10, 30, 40, 40, 30,-10,-30,
            -30,-10, 20, 30, 30, 20,-10,-30,
            -30,-30,  0,  0,  0,  0,-30,-30,
            -50,-30,-30,-30,-30,-30,-30,-50
        }, {}, /*pawn*/ {
            50, 50, 50, 50, 50, 50, 50, 50,
            50, 50, 50, 50, 50, 50, 50, 50,
            10, 10, 20, 30, 30, 20, 10, 10,
            5,   5, 10, 25, 25, 10,  5,  5,
            0,   0,  0, 20, 20,  0,  0,  0,
            5,  -5,-10,  0,  0,-10, -5,  5,
            5,  10, 10,-20,-20, 10, 10,  5,
            0,   0,  0,  0,  0,  0,  0,  0
        }, /*knight*/ {
            -50,-40,-30,-30,-30,-30,-40,-50,
            -40,-20,  0,  0,  0,  0,-20,-40,
            -30,  0, 10, 15, 15, 10,  0,-30,
            -30,  5, 15, 20, 20, 15,  5,-30,
            -30,  0, 15, 20, 20, 15,  0,-30,
            -30,  5, 10, 15, 15, 10,  5,-30,
            -40,-20,  0,  5,  5,  0,-20,-40,
            -50,-40,-30,-30,-30,-30,-40,-50,
        }, /*bishop*/ {
            -20,-10,-10,-10,-10,-10,-10,-20,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -10,  0,  5, 10, 10,  5,  0,-10,
            -10,  5,  5, 10, 10,  5,  5,-10,
            -10,  0, 10, 10, 10, 10,  0,-10,
            -10, 10, 10, 10, 10, 10, 10,-10,
            -10,  5,  0,  0,  0,  0,  5,-10,
            -20,-10,-10,-10,-10,-10,-10,-20,
        }, /*rook*/ {
             0,  0,  0,  0,  0,  0,  0,  0,
             5, 10, 10, 10, 10, 10, 10,  5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  5,  5,  0,  0, -5
        }, /*queen*/ {
            -20,-10,-10, -5, -5,-10,-10,-20,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -10,  0,  5,  5,  5,  5,  0,-10,
             -5,  0,  5,  5,  5,  5,  0, -5,
              0,  0,  5,  5,  5,  5,  0, -5,
            -10,  5,  5,  5,  5,  5,  0,-10,
            -10,  0,  5,  0,  0,  0,  0,-10,
            -20,-10,-10, -5, -5,-10,-10,-20
        }, /*king*/ {
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -20,-30,-30,-40,-40,-30,-30,-20,
            -10,-20,-20,-20,-20,-20,-20,-10,
             20, 20,  0,  0,  0,  0, 20, 20,
             20, 30, 10,  0,  0, 10, 30, 20
        }
    };

    for (Color c : { WHITE, BLACK }) {
        for (PieceType pt : { NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING }) {

            Square sq = A8;

            for (int i : psq_base[pt])
                psq[c][pt][relative_square(sq--, c)] = i;
        }
    }
}

inline int piece_weight(PieceType pt) {
    static constexpr int piece_weights[KING + 1] = {
        0, 0, 100, 300, 300, 500, 900, 1500
    };
    return piece_weights[pt];
}

template<Color Us>
Bitboard passers(Bitboard friendly_pawn, Bitboard opponent_pawn) {
#ifdef BMI
    return friendly_pawn
        & Passers[Us][0][pext(opponent_pawn, Us == WHITE ? 0x03030303030000ull : 0xc0c0c0c0c000ull)]
        & Passers[Us][1][pext(opponent_pawn, Us == WHITE ? 0x0c0c0c0c0c0000ull : 0x303030303000ull)]
        & Passers[Us][2][pext(opponent_pawn, Us == WHITE ? 0x30303030300000ull : 0x0c0c0c0c0c00ull)]
        & Passers[Us][3][pext(opponent_pawn, Us == WHITE ? 0xc0c0c0c0c00000ull : 0x030303030300ull)];
#else
    return friendly_pawn
        & Passers[Us][0][(Us == WHITE ? opponent_pawn >> 16 : opponent_pawn >> 8 ) & 0x3ff]
        & Passers[Us][1][(Us == WHITE ? opponent_pawn >> 26 : opponent_pawn >> 18) & 0x3ff]
        & Passers[Us][2][(Us == WHITE ? opponent_pawn >> 36 : opponent_pawn >> 28) & 0x3ff]
        & Passers[Us][3][(Us == WHITE ? opponent_pawn >> 46 : opponent_pawn >> 38) & 0x3ff];
#endif
}

template<Color Perspective>
int psq_score(PieceType pt, Square sq) {
    return psq[Perspective][pt][sq];
}

template<Color Us>
int material_count(const Position& pos) {
    return
        100 * (popcount(pos.bb(Us, PAWN  )) - popcount(pos.bb(!Us, PAWN  ))) +
        300 * (popcount(pos.bb(Us, KNIGHT)) - popcount(pos.bb(!Us, KNIGHT))) +
        300 * (popcount(pos.bb(Us, BISHOP)) - popcount(pos.bb(!Us, BISHOP))) +
        500 * (popcount(pos.bb(Us, ROOK  )) - popcount(pos.bb(!Us, ROOK  ))) +
        900 * (popcount(pos.bb(Us, QUEEN )) - popcount(pos.bb(!Us, QUEEN )));
}

template<Color Us, Color Them = !Us>
int evaluate(const Position& pos)
{
    int material = material_count<Us>(pos);
    int midgame;
    int endgame;

    midgame = material;

    Bitboard friendly_pawn = pos.bb(Us, PAWN);
    Bitboard opponent_pawn = pos.bb(Them, PAWN);

    midgame += king_safety<Us  >(pos.king_sq<Us  >(), friendly_pawn)
             - king_safety<Them>(pos.king_sq<Them>(), opponent_pawn);

    constexpr Bitboard Rank567 = relative_rank(Us, RANK_5, RANK_6, RANK_7);
    constexpr Bitboard Rank234 = relative_rank(Us, RANK_2, RANK_3, RANK_4);

    midgame += 4 * (popcount(friendly_pawn & Rank567) - popcount(opponent_pawn & Rank234));

    for (PieceType pt : { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING }) {

        for (Bitboard bb = pos.bb(Us, pt); bb; clear_lsb(bb))
            midgame += psq_score<Us>(pt, bsf(bb));

        for (Bitboard bb = pos.bb(Them, pt); bb; clear_lsb(bb))
            midgame -= psq_score<Them>(pt, bsf(bb));
    }

    Bitboard friendly_passers = passers<Us  >(friendly_pawn, opponent_pawn);
    Bitboard opponent_passers = passers<Them>(opponent_pawn, friendly_pawn);

    midgame += 8  * (popcount(friendly_passers & Rank234) - popcount(opponent_passers & Rank567));
    midgame += 16 * (popcount(friendly_passers & Rank567) - popcount(opponent_passers & Rank234));

    endgame = material;

    endgame += psq_score<Us  >(0, pos.king_sq<Us  >());
    endgame -= psq_score<Them>(0, pos.king_sq<Them>());

    constexpr Bitboard Rank2 = relative_rank(Us, RANK_2);
    constexpr Bitboard Rank3 = relative_rank(Us, RANK_3);
    constexpr Bitboard Rank4 = relative_rank(Us, RANK_4);
    constexpr Bitboard Rank5 = relative_rank(Us, RANK_5);
    constexpr Bitboard Rank6 = relative_rank(Us, RANK_6);
    constexpr Bitboard Rank7 = relative_rank(Us, RANK_7);

    endgame += 10 * popcount(friendly_pawn & Rank4);
    endgame += 20 * popcount(friendly_pawn & Rank5);
    endgame += 50 * popcount(friendly_pawn & Rank6);
    endgame += 90 * popcount(friendly_pawn & Rank7);

    endgame -= 10 * popcount(opponent_pawn & Rank5);
    endgame -= 20 * popcount(opponent_pawn & Rank4);
    endgame -= 50 * popcount(opponent_pawn & Rank3);
    endgame -= 90 * popcount(opponent_pawn & Rank2);

    endgame += 16 * (popcount(friendly_passers & Rank234) - popcount(opponent_passers & Rank567));
    endgame += 32 * (popcount(friendly_passers & Rank567) - popcount(opponent_passers & Rank234));

    constexpr int KnightPhase = 1;
    constexpr int BishopPhase = 1;
    constexpr int RookPhase   = 2;
    constexpr int QueenPhase  = 4;
    constexpr int TotalPhase  = KnightPhase*4 + BishopPhase*4 + RookPhase*4 + QueenPhase*2;

#define piece_count(pt) popcount(pos.bb(WHITE, pt) | pos.bb(BLACK, pt))
    int phase = TotalPhase - piece_count(KNIGHT)*KnightPhase
                           - piece_count(BISHOP)*BishopPhase
                           - piece_count(ROOK  )*RookPhase
                           - piece_count(QUEEN )*QueenPhase;
#undef piece_count
    phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;

    return ((midgame * (256 - phase)) + (endgame * phase)) / 256;
}

template<Color Us, Color Them = !Us>
int mopup(const Position& pos) {

    int score = material_count<Us>(pos);

    Square friendly_ksq = pos.king_sq<Us>();
    Square opponent_ksq = pos.king_sq<Them>();

    return score > 0
        ? score + 10 * distance_from_center(opponent_ksq) + 4 * (14 - square_distance(friendly_ksq, opponent_ksq))
        : score - 10 * distance_from_center(friendly_ksq) - 4 * (14 - square_distance(friendly_ksq, opponent_ksq));
}

template<Color Perspective>
int static_eval(const Position& pos) {
    return pos.mopup() ? mopup<Perspective>(pos) : evaluate<Perspective>(pos);
}

#endif

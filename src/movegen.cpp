
#include <array>

#include "bitboard.h"
#include "movegen.h"
#include "position.h"

template<Color Us>
EMove* make_castling(EMove* list, const Position& pos, Bitboard occupied) {

    static constexpr auto lut = []() {

        std::array<
            std::array<
                std::array<uint64_t, 1 << 6>,
            CASTLING_RIGHT_NB>,
        COLOR_NB> table = {};

        for (Color c : { WHITE, BLACK }) {

            EMove k_castle = make_move<CASTLING>(c == WHITE ? E1 : E8, c == WHITE ? G1 : G8);
            EMove q_castle = make_move<CASTLING>(c == WHITE ? E1 : E8, c == WHITE ? C1 : C8);

            for (CastlingRights rights = NO_CASTLING; rights <= ALL_CASTLING; rights++) {

                bool k_rights = make_castling_rights(c, rights & KING_SIDE);
                bool q_rights = make_castling_rights(c, rights & QUEEN_SIDE);

                for (int violations = 0; violations <= 0x3f; violations++) {

                    EMove res[2] = {}, *p = res;

                    if (k_rights && (violations & 0b000111) == 0)
                        *p++ = k_castle;

                    if (q_rights && (violations & 0b111100) == 0)
                        *p++ = q_castle;

                    table[c][rights][violations] = uint64_t(res[0]) | (uint64_t(res[1]) << 32);
                }
            }
        }

        return table;
    }();

    constexpr int Shift = Us == WHITE ? 1 : 57;
    constexpr Bitboard NoOcc = Us == WHITE ? square_bb(B1, C1, D1, F1, G1) : square_bb(B8, C8, D8, F8, G8);
    constexpr Bitboard NoThr = Us == WHITE ? square_bb(C1, D1, E1, F1, G1) : square_bb(C8, D8, E8, F8, G8);

    memcpy(list, &lut[Us][pos.castling_rights()][(NoOcc & occupied | NoThr & pos.threats<Us>()) >> Shift], 8);
    return list + popcount(*(uint64_t*)list & 0x100000001000);
}

template<MoveType MoveT, Direction D, GenType GenT = ALL>
EMove* make_pawn_moves(EMove *list, Bitboard attacks) {

    for (;attacks; clear_lsb(attacks)) {

        Square to = bsf(attacks);

        if constexpr (MoveT == NORMAL)
            *list++ = make_move(to - D, to);

        else if constexpr (MoveT == PROMOTION) {
            *list++ = make_move<QUEEN_PROMOTION>(to - D, to);
            if constexpr (GenT == ALL) {
                *list++ = make_move<KNIGHT_PROMOTION>(to - D, to);
                *list++ = make_move<ROOK_PROMOTION  >(to - D, to);
                *list++ = make_move<BISHOP_PROMOTION>(to - D, to);
            }
        }
    }

    return list;
}

EMove* make_moves(EMove *list, Square from, Bitboard to) {
    for (;to; clear_lsb(to))
        *list++ = make_move(from, bsf(to));
    return list;
}

template<Color Us, GenType GenT>
EMove* generate(const Position& pos, EMove *list) {

    constexpr Color Them      = !Us;
    const Square    ksq       = pos.king_sq<Us>();
    Bitboard        checkmask = pos.checkers<Us>();

    if (more_than_one(checkmask))
        return GenT == ALL
            ? make_moves(list, ksq, king_attacks(ksq) & ~(pos.threats<Us>() | pos.bb(Us)))
            : make_moves(list, ksq, king_attacks(ksq) & pos.bb(Them) & ~pos.threats<Us>());

    checkmask = checkmask ? between_bb(ksq, bsf(checkmask)) : -1ULL;

    Bitboard pinned = 0;

    const Bitboard opponent_bq = pos.bb(Them, BISHOP) | pos.bb(Them, QUEEN);
    const Bitboard opponent_rq = pos.bb(Them, ROOK) | pos.bb(Them, QUEEN);
    const Bitboard occupied    = pos.occupied_bb();

    for (Bitboard pinners = bishop_xray(ksq, occupied) & opponent_bq | rook_xray(ksq, occupied) & opponent_rq; pinners; clear_lsb(pinners))
        pinned |= between_bb(ksq, bsf(pinners));

    constexpr Direction Up      = Us == WHITE ? NORTH      : SOUTH;
    constexpr Direction UpRight = Us == WHITE ? NORTH_EAST : SOUTH_WEST;
    constexpr Direction UpLeft  = Us == WHITE ? NORTH_WEST : SOUTH_EAST;
    constexpr Bitboard  Rank3   = Us == WHITE ? RANK_3BB   : RANK_6BB;
    constexpr Bitboard  Rank6   = Us == WHITE ? RANK_6BB   : RANK_3BB;
    constexpr Bitboard  Rank7   = Us == WHITE ? RANK_7BB   : RANK_2BB;

    if (Bitboard promotable = pos.bb(Us, PAWN) & Rank7) {
        list = make_pawn_moves<PROMOTION, UpRight, GenT>(list, shift<UpRight>(promotable & (~pinned | anti_diag(ksq))) & pos.bb(Them) & checkmask);
        list = make_pawn_moves<PROMOTION, UpLeft,  GenT>(list, shift<UpLeft >(promotable & (~pinned | main_diag(ksq))) & pos.bb(Them) & checkmask);
        list = make_pawn_moves<PROMOTION, Up,      GenT>(list, shift<Up     >(promotable &  ~pinned                  ) & ~occupied    & checkmask);
    }

    const Bitboard pawns = pos.bb(Us, PAWN) & ~Rank7;

    list = make_pawn_moves<NORMAL, UpRight>(list, shift<UpRight>(pawns & (~pinned | anti_diag(ksq))) & pos.bb(Them) & checkmask);
    list = make_pawn_moves<NORMAL, UpLeft >(list, shift<UpLeft >(pawns & (~pinned | main_diag(ksq))) & pos.bb(Them) & checkmask);

    if constexpr (GenT == ALL) {

        Bitboard push  = shift<Up>(pawns & (~pinned | file_bb(ksq))) & ~occupied;
        Bitboard push2 = shift<Up>(push & Rank3) & ~occupied;
        list = make_pawn_moves<NORMAL, Up  >(list, push  & checkmask);
        list = make_pawn_moves<NORMAL, Up*2>(list, push2 & checkmask);
        
        if (Square ep = pos.ep_sq(); ep) {
            for (Bitboard bb = pawn_attacks<Them>(ep) & pawns; bb; clear_lsb(bb)) {
                Square   from     = bsf(bb);
                Bitboard after_ep = occupied ^ square_bb(from, ep - Up, ep);
                if (!(bishop_attacks(ksq, after_ep) & opponent_bq | rook_attacks(ksq, after_ep) & opponent_rq))
                    *list++ = make_move<ENPASSANT>(from, ep);
            }
        }
    }

    const Bitboard target = checkmask & (
        GenT == CAPTURES ? pos.bb(Them) : ~pos.bb(Us)
    );

    for (Bitboard b = pos.bb(Us, KNIGHT) & ~pinned; b; clear_lsb(b)) {
        Square from = bsf(b);
        list = make_moves(list, from, knight_attacks(from) & target);
    }

    const Bitboard bishop_queen = pos.bb(Us, BISHOP) | pos.bb(Us, QUEEN);
    const Bitboard rook_queen   = pos.bb(Us, ROOK) | pos.bb(Us, QUEEN);

    for (Bitboard b = bishop_queen & ~pinned; b; clear_lsb(b)) {
        Square from = bsf(b);
        list = make_moves(list, from, bishop_attacks(from, occupied) & target);
    }

    for (Bitboard b = bishop_queen & pinned; b; clear_lsb(b)) {
        Square from = bsf(b);
        list = make_moves(list, from, bishop_attacks(from, occupied) & target & align_mask(ksq, from));
    }

    for (Bitboard b = rook_queen & ~pinned; b; clear_lsb(b)) {
        Square from = bsf(b);
        list = make_moves(list, from, rook_attacks(from, occupied) & target);
    }

    for (Bitboard b = rook_queen & pinned; b; clear_lsb(b)) {
        Square from = bsf(b);
        list = make_moves(list, from, rook_attacks(from, occupied) & target & align_mask(ksq, from));
    }

    if constexpr (GenT == CAPTURES)
        return make_moves(list, ksq, king_attacks(ksq) & pos.bb(Them) & ~pos.threats<Us>());

    list = make_moves(list, ksq, king_attacks(ksq) & ~(pos.threats<Us>() | pos.bb(Us)));
    list = make_castling<Us>(list, pos, occupied);

    return list;
}

template EMove* generate<WHITE, ALL>(const Position&, EMove*);
template EMove* generate<WHITE, CAPTURES>(const Position&, EMove*);
template EMove* generate<BLACK, ALL>(const Position&, EMove*);
template EMove* generate<BLACK, CAPTURES>(const Position&, EMove*);
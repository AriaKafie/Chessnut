
#ifndef TYPES_H
#define TYPES_H

#pragma warning(disable: 4146)
#pragma warning(disable: 26495)

#include <cstdint>

#ifdef _MSC_VER
    #define ForceInline __forceinline
#else
    #define ForceInline __attribute__((always_inline))
#endif

#define BMI

typedef uint64_t Bitboard;
typedef uint32_t EMove;
typedef int      Direction;
typedef int      File;
typedef int      Rank;
typedef uint16_t MoveType;
typedef uint16_t Move;
typedef uint16_t Score;
typedef int8_t   Square;
typedef uint8_t  Piece;
typedef uint8_t  PieceType;
typedef uint8_t  Color;

constexpr Move NULLMOVE = 0;
constexpr Move NO_MOVE  = 0;

constexpr int MAX_DEPTH = 128;
constexpr int MAX_PLIES = 256;
constexpr int MATE      = 32000; // 30k
constexpr int INFINITE  = MATE + 1; // 32k
constexpr int NO_EVAL   = 0x7fffffff;

constexpr int MATE_IN_MAX_PLY = MATE - MAX_PLIES;
constexpr int MATED_IN_MAX_PLY = -MATE_IN_MAX_PLY;

inline bool is_win(int eval) {
    return eval >= MATE_IN_MAX_PLY;
}

inline bool is_loss(int eval) {
    return eval <= MATED_IN_MAX_PLY;
}

enum GamePhase : uint8_t { MIDGAME, ENDGAME, MOPUP };

enum {
    H1, G1, F1, E1, D1, C1, B1, A1,
    H2, G2, F2, E2, D2, C2, B2, A2,
    H3, G3, F3, E3, D3, C3, B3, A3,
    H4, G4, F4, E4, D4, C4, B4, A4,
    H5, G5, F5, E5, D5, C5, B5, A5,
    H6, G6, F6, E6, D6, C6, B6, A6,
    H7, G7, F7, E7, D7, C7, B7, A7,
    H8, G8, F8, E8, D8, C8, B8, A8,
    SQUARE_NB = 64, NO_SQ = 64
};

enum : uint8_t {
    NO_PIECE, PIECE_TYPE_NB = 6, PIECE_NB = 16,
      PAWN =        2,   KNIGHT,   BISHOP,   ROOK,   QUEEN,   KING,
    W_PAWN =     PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING
};

enum { WHITE, BLACK, COLOR_NB };

enum : uint16_t {
    NORMAL           = 0 << 12,
    PROMOTION        = 1 << 12,
    ENPASSANT        = 2 << 12,
    CASTLING         = 3 << 12,
    KNIGHT_PROMOTION = PROMOTION + ((KNIGHT - KNIGHT) << 14),
    BISHOP_PROMOTION = PROMOTION + ((BISHOP - KNIGHT) << 14),
    ROOK_PROMOTION   = PROMOTION + ((ROOK   - KNIGHT) << 14),
    QUEEN_PROMOTION  = PROMOTION + ((QUEEN  - KNIGHT) << 14)
};

enum {
    FILE_H,
    FILE_G,
    FILE_F,
    FILE_E,
    FILE_D,
    FILE_C,
    FILE_B,
    FILE_A,
    FILE_NB
};

enum {
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8,
    RANK_NB
};

enum {
    NORTH      =  8,
    EAST       = -1,
    SOUTH      = -8,
    WEST       =  1,
    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_WEST = SOUTH + WEST,
};

constexpr void set_score(EMove& em, Score score) {
    em += score << 16;
}

constexpr Score score_of(EMove em) {
    return em >> 16;
}

constexpr Direction relative_direction(Color c, Direction d) {
    return c == WHITE ? d : -d;
}

constexpr Square relative_square(Square sq, Color c) {
    return c == WHITE ? sq : sq ^ 63;
}

constexpr Piece make_piece(Color c, PieceType pt) {
    return pt + (c << 3);
}

constexpr PieceType piece_type_of(Piece p) {
    return p & 7;
}

constexpr Color color_of(Piece p) {
    return p >> 3;
}

constexpr Move make_move(Square from, Square to) {
    return from + (to << 6);
}

template<MoveType T>
constexpr Move make_move(Square from, Square to) {
    return T + from + (to << 6);
}

inline bool is_ok(Square s) {
    return s >= H1 && s <= A8;
}

constexpr Square from_sq(Move m) {
    return m & 0x3f;
}

constexpr Square to_sq(Move m) {
    return (m >> 6) & 0x3f;
}

constexpr Move from_to(Move m) {
    return m & 0xfff;
}

constexpr MoveType type_of(Move m) {
    return m & 0x3000;
}

constexpr PieceType promotion_type_of(Move m) {
    return (m >> 14) + KNIGHT;
}

inline File file_of(Square s) {
    return s & 7;
}

inline Rank rank_of(Square s) {
    return s >> 3;
}

inline Square make_square(Rank r, File f) {
    return r * 8 + f;
}

#endif

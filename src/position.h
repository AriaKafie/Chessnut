
#ifndef POSITION_H
#define POSITION_H

#include <cstring>
#include <string>

#include "bitboard.h"
#include "types.h"

#define STARTPOS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"

namespace Zobrist {
    constexpr uint64_t Side = 0xeeb3b2fe864d41e5ull;
    inline uint64_t hash[B_KING + 1][SQUARE_NB];
    inline uint64_t enpassant[SQUARE_NB];
    inline uint64_t castling[1 << 4];
}

typedef struct {
    uint64_t  key;
    uint8_t   castling_rights;
    Square    ep_sq;
    Piece     captured;
    Color     side_to_move;
    GamePhase gamephase;
    Bitboard  threats_bb;
} StateInfo;

constexpr int RT_SIZE = 1024;

typedef struct {
    uint64_t key;
    int      occurrences;
} RTEntry;

class Position {

public:
    static void init();

    Position(const std::string& fen = STARTPOS);
    Position(const Position& other);
    Position& operator=(const Position&) = delete;
    ~Position();

    void set(const std::string& fen);
    std::string fen() const;
    std::string to_string() const;

    Bitboard  bb(Piece pc) const;
    Bitboard  bb(Color c, PieceType pt) const;
    Bitboard  occupied_bb() const;
    Piece     piece_on(Square sq) const;
    PieceType piece_type_on(Square sq) const;

    uint64_t  key() const;
    uint8_t   castling_rights() const;
    Square    ep_sq() const;
    Bitboard  ep_bb() const;
    Color     side_to_move() const;
    bool      white_to_move() const;
    bool      black_to_move() const;
    bool      midgame() const;
    bool      endgame() const;
    bool      mopup() const;

    template<Color STM>
    bool      legal(Move m) const;
    bool      is_capture(Move m) const;
    bool      is_quiet(Move m) const;
    template<Color STM>
    Bitboard  checkers() const;
    template<Color Us>
    Bitboard  threats() const;
    bool      is_repetition() const;
    template<Color Us>
    Bitboard  pinned() const;
    template<Color C>
    Square    king_sq() const;

    void commit_move(Move m);
    template<Color Us>
    void do_move(Move m);
    template<Color Us>
    void undo_move(Move m);
    void do_null_move();
    void undo_null_move();

private:
    template<Color JustMoved>
    void update_castling_rights();
    template<Color Us>
    void update_threats() const;
    void set_gamephase();

    void rt_push();
    void rt_pop();
    void rt_clear();

    StateInfo  state_stack[MAX_PLIES];
    StateInfo* state_ptr = state_stack;
    RTEntry*   repetition_table;
    Bitboard   bitboards[16];
    Piece      board[SQUARE_NB];
};

inline Position::Position(const std::string& fen) {
    repetition_table = new RTEntry[RT_SIZE];
    set(fen);
}

inline Position::Position(const Position& other) {

    repetition_table = new RTEntry[RT_SIZE];

    memcpy(state_stack, other.state_stack, sizeof(StateInfo) * MAX_PLIES);
    memcpy(repetition_table, other.repetition_table, sizeof(RTEntry) * RT_SIZE);
    memcpy(bitboards, other.bitboards, sizeof(Bitboard) * 16);
    memcpy(board, other.board, sizeof(Piece) * SQUARE_NB);

    state_ptr = state_stack + (other.state_ptr - other.state_stack);
}

inline Position::~Position() {
    delete[] repetition_table;
}

inline Bitboard Position::bb(Piece pc) const {
    return bitboards[pc];
}

inline Bitboard Position::bb(Color c, PieceType pt) const {
    return bitboards[make_piece(c, pt)];
}

inline Bitboard Position::occupied_bb() const {
    return bitboards[WHITE] | bitboards[BLACK];
}

inline Piece Position::piece_on(Square sq) const {
    return board[sq];
}

inline PieceType Position::piece_type_on(Square sq) const {
    return piece_type_of(board[sq]);
}

inline uint64_t Position::key() const {
    return state_ptr->key;
}

inline uint8_t Position::castling_rights() const {
    return state_ptr->castling_rights;
}

inline Square Position::ep_sq() const {
    return state_ptr->ep_sq;
}

inline Bitboard Position::ep_bb() const {
    return square_bb(state_ptr->ep_sq);
}

inline Color Position::side_to_move() const {
    return state_ptr->side_to_move;
}

inline bool Position::white_to_move() const { return state_ptr->side_to_move == WHITE; }
inline bool Position::black_to_move() const { return state_ptr->side_to_move == BLACK; }

inline bool Position::midgame() const { return state_ptr->gamephase == MIDGAME; }
inline bool Position::endgame() const { return state_ptr->gamephase == ENDGAME; }
inline bool Position::mopup  () const { return state_ptr->gamephase == MOPUP;   }

inline bool Position::is_capture(Move m) const {
    return piece_on(to_sq(m));
}

inline bool Position::is_quiet(Move m) const {
    return !piece_on(to_sq(m));
}

template<Color Us>
Bitboard Position::pinned() const {

    Square ksq = king_sq<Us>();

    const Bitboard opponent_bq = bb(!Us, BISHOP) | bb(!Us, QUEEN);
    const Bitboard opponent_rq = bb(!Us, ROOK) | bb(!Us, QUEEN);
    const Bitboard occupied    = occupied_bb();

    Bitboard pinned_bb = 0;

    for (Bitboard pinners = bishop_xray(ksq, occupied) & opponent_bq | rook_xray(ksq, occupied) & opponent_rq; pinners; clear_lsb(pinners))
        pinned_bb |= between_bb(ksq, bsf(pinners));

    return pinned_bb;
}

template<Color Us>
Bitboard Position::threats() const {
    if (!state_ptr->threats_bb)
        update_threats<Us>();
    return state_ptr->threats_bb;
}

template<Color Us>
void Position::update_threats() const {

    constexpr Color Them     = !Us;
    const Bitboard  occupied = occupied_bb() ^ bb(Us, KING);
    Bitboard&       thr      = state_ptr->threats_bb;

    thr = pawn_attacks<Them>(bb(Them, PAWN));
    
    for (Bitboard knights = bb(Them, KNIGHT); knights; clear_lsb(knights))
        thr |= knight_attacks(bsf(knights));

    for (Bitboard bq = bb(Them, BISHOP) | bb(Them, QUEEN); bq; clear_lsb(bq))
        thr |= bishop_attacks(bsf(bq), occupied);

    for (Bitboard rq = bb(Them, ROOK) | bb(Them, QUEEN); rq; clear_lsb(rq))
        thr |= rook_attacks(bsf(rq), occupied);

    thr |= king_attacks(king_sq<Them>());
}

template<Color STM>
Bitboard Position::checkers() const {

    Square ksq = king_sq<STM>();

    return
        pawn_attacks<STM>(ksq)             &  bb(!STM, PAWN)
      | knight_attacks(ksq)                &  bb(!STM, KNIGHT)
      | bishop_attacks(ksq, occupied_bb()) & (bb(!STM, BISHOP) | bb(!STM, QUEEN))
      | rook_attacks(ksq, occupied_bb())   & (bb(!STM, ROOK) | bb(!STM, QUEEN));
}

template<Color C>
Square Position::king_sq() const {
    return bsf(bitboards[make_piece(C, KING)]);
}

template<Color JustMoved>
void Position::update_castling_rights() {
    state_ptr->key ^= Zobrist::castling[state_ptr->castling_rights];
    constexpr Bitboard Mask = JustMoved == WHITE ? square_bb(A1, E1, H1, A8, H8) : square_bb(A8, E8, H8, A1, H1);
#ifdef BMI
    state_ptr->castling_rights &= CastleMasks[JustMoved][pext(bitboards[JustMoved], Mask)];
#else
    constexpr Bitboard Magic = JustMoved == WHITE ? 0x4860104020003061ull : 0x1080000400400c21ull;
    state_ptr->castling_rights &= CastleMasks[JustMoved][(bitboards[JustMoved] & Mask) * Magic >> 59];
#endif
    state_ptr->key ^= Zobrist::castling[state_ptr->castling_rights];
}

template<Color Us>
void Position::do_move(Move m)
{
    constexpr Color Them = !Us;

    constexpr Piece Pawn  = make_piece(Us, PAWN);
    constexpr Piece Rook  = make_piece(Us, ROOK);
    constexpr Piece Queen = make_piece(Us, QUEEN);
    constexpr Piece King  = make_piece(Us, KING);

    constexpr Direction Up = relative_direction(Us, NORTH);

    const Square from    = from_sq(m);
    const Square to      = to_sq(m);
    const Piece  pc      = piece_on(from);
    const Piece  capture = piece_on(to);

    memcpy(state_ptr + 1, state_ptr, offsetof(StateInfo, threats_bb));
    state_ptr++;

    state_ptr->captured = capture;

    state_ptr->key ^= Zobrist::enpassant[state_ptr->ep_sq];
    state_ptr->ep_sq = 0;

    if (Bitboard pawns;
        pc == Pawn
        && (to - from) == Up*2
        && (pawns = PawnAttacks[Us][from + Up] & bb(Them, PAWN))) {

        const Square   ksq      = king_sq<Them>();
        const Square   ep       = from + Up;
        const Bitboard occupied = occupied_bb() ^ square_bb(from, ep) ^ pawns;
        const Bitboard bq       = bb(Us, BISHOP) | bb(Us, QUEEN);
        const Bitboard rq       = bb(Us, ROOK) | bb(Us, QUEEN);
        const Bitboard checks   =
            (bishop_attacks(ksq, occupied) & bq) |
            (rook_attacks(ksq, occupied) & rq);
        
        bool discovery = checks & align_mask(ksq, from);

        if (!discovery && popcount(pawns) > popcount(checks)) {
            state_ptr->ep_sq = ep;
            state_ptr->key ^= Zobrist::enpassant[ep];
        }
    }

    state_ptr->threats_bb = 0;

    Bitboard zero_to = ~square_bb(to);
    Bitboard from_to =  square_bb(from, to);

    switch (type_of(m))
    {
    case NORMAL:
        state_ptr->key ^= Zobrist::hash[pc][from]
                       ^  Zobrist::hash[pc][to]
                       ^  Zobrist::hash[capture][to]
                       ^  Zobrist::Side;

        bitboards[capture] &= zero_to;
        bitboards[Them]    &= zero_to;
        bitboards[pc]      ^= from_to;
        bitboards[Us]      ^= from_to;

        board[to]   = pc;
        board[from] = NO_PIECE;

        update_castling_rights<Us>();

        rt_push();

        return;
    case PROMOTION:
    {
        Piece promotion = make_piece(Us, promotion_type_of(m));

        state_ptr->key ^= Zobrist::hash[Pawn][from]
                       ^  Zobrist::hash[promotion][to]
                       ^  Zobrist::hash[capture][to]
                       ^  Zobrist::Side;

        bitboards[capture]   &= zero_to;
        bitboards[Them]      &= zero_to;
        bitboards[Pawn]      ^= square_bb(from);
        bitboards[promotion] ^= ~zero_to;
        bitboards[Us]        ^= from_to;

        board[to]   = promotion;
        board[from] = NO_PIECE;
        
        update_castling_rights<Us>();

        rt_push();

        return;
    }
    case CASTLING:
    {
        Move rook_move = Us == WHITE
            ? to == G1
                ? make_move(H1, F1)
                : make_move(A1, D1)
            : to == G8
                ? make_move(H8, F8)
                : make_move(A8, D8);

        Square   rfrom    = from_sq(rook_move);
        Square   rto      = to_sq(rook_move);
        Bitboard rfrom_to = square_bb(rfrom, rto);

        state_ptr->key ^= Zobrist::hash[King][from]
                       ^  Zobrist::hash[King][to]
                       ^  Zobrist::hash[Rook][rfrom]
                       ^  Zobrist::hash[Rook][rto]
                       ^  Zobrist::Side;

        bitboards[King] ^= from_to;
        bitboards[Rook] ^= rfrom_to;
        bitboards[Us]   ^= from_to ^ rfrom_to;

        board[from]  = NO_PIECE;
        board[rfrom] = NO_PIECE;
        board[to]    = King;
        board[rto]   = Rook;

        update_castling_rights<Us>();

        rt_push();

        return;
    }
    case ENPASSANT:
        constexpr Piece EnemyPawn = make_piece(Them, PAWN);

        Square capsq = to - Up;

        state_ptr->key ^= Zobrist::hash[Pawn][from]
                       ^  Zobrist::hash[Pawn][to]
                       ^  Zobrist::hash[EnemyPawn][capsq]
                       ^  Zobrist::Side;

        bitboards[Pawn]      ^= from_to;
        bitboards[EnemyPawn] ^= square_bb(capsq);
        bitboards[Us]        ^= from_to;
        bitboards[Them]      ^= square_bb(capsq);

        board[from]  = NO_PIECE;
        board[to]    = Pawn;
        board[capsq] = NO_PIECE;
        
        rt_push();

        return;
    }
}

template<Color Us>
void Position::undo_move(Move m)
{
    rt_pop();

    constexpr Color Them = !Us;

    constexpr Piece Pawn  = make_piece(Us, PAWN);
    constexpr Piece Rook  = make_piece(Us, ROOK);
    constexpr Piece Queen = make_piece(Us, QUEEN);
    constexpr Piece King  = make_piece(Us, KING);

    Piece captured = state_ptr--->captured;

    Square from = from_sq(m), to = to_sq(m);

    Bitboard to_bb      = square_bb(to);
    Bitboard from_to    = square_bb(from, to);
    Bitboard capture_bb = to_bb * bool(captured);

    switch (type_of(m))
    {
    case NORMAL:
        bitboards[board[to]] ^= from_to;
        bitboards[Us]        ^= from_to;
        bitboards[captured]  ^= capture_bb;
        bitboards[Them]      ^= capture_bb;

        board[from] = board[to];
        board[to]   = captured;

        return;
    case PROMOTION:
        bitboards[board[to]] ^= to_bb;
        bitboards[Pawn]      ^= square_bb(from);
        bitboards[Us]        ^= from_to;
        bitboards[captured]  ^= capture_bb;
        bitboards[Them]      ^= capture_bb;

        board[to]   = captured;
        board[from] = Pawn;

        return;
    case CASTLING:
    {
        Move rook_move = Us == WHITE
            ? to == G1
                ? make_move(H1, F1)
                : make_move(A1, D1)
            : to == G8
                ? make_move(H8, F8)
                : make_move(A8, D8);

        Square   rfrom    = from_sq(rook_move), rto = to_sq(rook_move);
        Bitboard rfrom_to = square_bb(rfrom, rto);

        bitboards[King] ^= from_to;
        bitboards[Rook] ^= rfrom_to;
        bitboards[Us]   ^= from_to ^ rfrom_to;

        board[to]    = NO_PIECE;
        board[rto]   = NO_PIECE;
        board[from]  = King;
        board[rfrom] = Rook;

        return;
    }
    case ENPASSANT:
        constexpr Piece EnemyPawn = make_piece(Them, PAWN);

        Square capsq = to + (Us == WHITE ? SOUTH : NORTH);

        bitboards[Pawn]      ^= from_to;
        bitboards[Us]        ^= from_to;
        bitboards[EnemyPawn] ^= square_bb(capsq);
        bitboards[Them]      ^= square_bb(capsq);

        board[to]    = NO_PIECE;
        board[from]  = Pawn;
        board[capsq] = EnemyPawn;

        return;
    }
}

#endif

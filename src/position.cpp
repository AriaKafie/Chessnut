
#include "position.h"

#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>

#include "evaluation.h"
#include "movelist.h"
#include "transpositiontable.h"
#include "uci.h"

template<Color SideToMove>
bool Position::legal(Move m) const {
    return MoveList<SideToMove>(*this).contains(m);
} template bool Position::legal<WHITE>(Move) const;
  template bool Position::legal<BLACK>(Move) const;

void Position::init() {

    std::mt19937_64 rng(221564671644);

    for (Piece pc : { W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
                      B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING }) {
        for (Square sq = H1; sq <= A8; sq++) {
            Zobrist::hash[pc][sq] = rng();
        }
    }

    for (Square sq = H1; sq <= A8; sq++)
        Zobrist::enpassant[sq] = sq ? rng() : 0;

    for (uint8_t castling_rights = 0; castling_rights <= 0xf; castling_rights++)
        Zobrist::castling[castling_rights] = rng();
}

void Position::set(const std::string& fen) {

    const std::string piece_to_char = "  PNBRQK  pnbrqk";

    state_ptr = state_stack;

    memset(bitboards, 0, sizeof(Bitboard) * 16);
    memset(board, 0, sizeof(Piece) * SQUARE_NB);

    std::stringstream ss(fen);
    std::string pieces, color, castling, enpassant;
    ss >> pieces >> color >> castling >> enpassant;

    Square sq = A8;

    for (char c : pieces) {
        
        if (std::isdigit(c))
            sq -= c - '0';

        else if (size_t pc = piece_to_char.find(c); pc != std::string::npos) {
            board[sq] = pc;
            bitboards[pc] |= square_bb(sq);
            bitboards[color_of(pc)] |= square_bb(sq);
            sq--;
        }
    }

    memset(state_ptr, 0, sizeof(StateInfo));

    state_ptr->side_to_move = color == "w" ? WHITE : BLACK;

    for (char c : castling)
        if (size_t rights = std::string("qkQK").find(c); rights != std::string::npos)
            state_ptr->castling_rights |= 1 << rights;

    if (enpassant != "-")
        state_ptr->ep_sq = uci_to_square(enpassant);

    for (Square sq = H1; sq <= A8; sq++)
        state_ptr->key ^= Zobrist::hash[piece_on(sq)][sq];

    if (black_to_move())
        state_ptr->key ^= Zobrist::Side;

    state_ptr->key ^= Zobrist::castling[state_ptr->castling_rights];
    state_ptr->key ^= Zobrist::enpassant[state_ptr->ep_sq];

    rt_clear();
    rt_push();

    set_gamephase();
}

std::string Position::to_string() const {

    std::stringstream ss;

    for (Rank r = RANK_8; r >= RANK_1; r--) {

        ss << "\n+---+---+---+---+---+---+---+---+\n";

        for (File f = FILE_A; f >= FILE_H; f--)
            ss << "| " << "  PNBRQK  pnbrqk"[board[make_square(r, f)]] << " ";

        ss << "| " << r + 1;
    }

    ss << "\n+---+---+---+---+---+---+---+---+\n"
       << "  a   b   c   d   e   f   g   h\n\n"
       << "Fen: " << fen() << "\n"
       << "Key: " << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << key() << "\n";

    return ss.str();
}

void Position::do_null_move() {

    memcpy(state_ptr + 1, state_ptr, sizeof(StateInfo));
    state_ptr++;

    state_ptr->captured = NO_PIECE;

    state_ptr->key ^= Zobrist::Side;

    state_ptr->key ^= Zobrist::enpassant[(state_ptr - 1)->ep_sq];
    state_ptr->ep_sq = 0;
}

void Position::undo_null_move() {
    state_ptr--;
}

bool Position::is_repetition() const {
    RTEntry *r = &repetition_table[state_ptr->key & (RT_SIZE - 1)];
    return r->key == state_ptr->key && r->occurrences > 1;
}

void Position::rt_push() {

    RTEntry *r = &repetition_table[state_ptr->key & (RT_SIZE - 1)];

    if (r->key == state_ptr->key) {
        r->occurrences++;
    } else if (r->occurrences == 0) {
        r->key = state_ptr->key;
        r->occurrences = 1;
    }
}

void Position::rt_pop() {
    RTEntry *r = &repetition_table[state_ptr->key & (RT_SIZE - 1)];
    if (r->key == state_ptr->key)
        r->occurrences--;
}

void Position::rt_clear() {
    memset(repetition_table, 0, RT_SIZE * sizeof(RTEntry));
}

std::string Position::fen() const {

    std::stringstream fen;

    for (Rank r = RANK_8; r >= RANK_1; r--) {
        for (File f = FILE_A; f >= FILE_H; f--) {

            if (Piece pc = piece_on(make_square(r, f)))
                fen << "  PNBRQK  pnbrqk"[pc];

            else {
                int empty = 0;

                for (File fi = f++; fi >= FILE_H && !piece_on(make_square(r, fi)); fi--)
                    empty++, f--;

                fen << empty;
            }
        }

        if (r) fen << "/";
    }

    fen << " " << "wb"[side_to_move()] << " ";

    if (!state_ptr->castling_rights)
        fen << "-";

    else {
        if (state_ptr->castling_rights & 8) fen << "K";
        if (state_ptr->castling_rights & 4) fen << "Q";
        if (state_ptr->castling_rights & 2) fen << "k";
        if (state_ptr->castling_rights & 1) fen << "q";
    }

    fen << " " << (state_ptr->ep_sq ? square_to_uci(state_ptr->ep_sq) : "-");

    return fen.str();
}

void Position::commit_move(Move m) {

    if (is_capture(m) || piece_type_on(from_sq(m)) == PAWN || type_of(m) != NORMAL)
        rt_clear();

    white_to_move() ? do_move<WHITE>(m)
                    : do_move<BLACK>(m);

    memcpy(state_stack, state_ptr, sizeof(StateInfo));
    state_ptr = state_stack;
    state_ptr->side_to_move = !state_ptr->side_to_move;

    set_gamephase();
}

void Position::set_gamephase() {

    Color us = state_ptr->side_to_move, them = !us;

#define piece_count(pc) popcount(bitboards[pc])

    int friendly_material = 3 * piece_count(make_piece(us, KNIGHT)) +
                            3 * piece_count(make_piece(us, BISHOP)) +
                            5 * piece_count(make_piece(us, ROOK))   +
                            9 * piece_count(make_piece(us, QUEEN));
        
    int enemy_material    = 3 * piece_count(make_piece(them, KNIGHT)) +
                            3 * piece_count(make_piece(them, BISHOP)) +
                            5 * piece_count(make_piece(them, ROOK))   +
                            9 * piece_count(make_piece(them, QUEEN));

    bool pawns = bitboards[W_PAWN] || bitboards[B_PAWN];

    if (!pawns && (enemy_material < 5 && friendly_material >= 5 || friendly_material < 5 && enemy_material >= 5))
        state_ptr->gamephase = MOPUP;
    else if (friendly_material < 10 || friendly_material < 17 && !bitboards[make_piece(us, QUEEN)])
        state_ptr->gamephase = ENDGAME;
    else
        state_ptr->gamephase = MIDGAME;
        
#undef piece_count
}


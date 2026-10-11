
#ifndef UCI_H
#define UCI_H

#include <string>

#include "position.h"
#include "types.h"

constexpr std::string_view f2c = "hgfedcba";
constexpr std::string_view r2c = "12345678";

Move uci_to_move(const Position& pos, const std::string& uci);

namespace UCI { void loop(); }

inline std::string square_to_uci(Square sq) {
    return std::string() + f2c[file_of(sq)] + r2c[rank_of(sq)];
}

inline std::string move_to_uci(Move m) {
    std::string uci = square_to_uci(from_sq(m))
                    + square_to_uci(to_sq(m));
    if (type_of(m) == PROMOTION)
        uci += "   nbrq"[promotion_type_of(m)];
    return uci;
}

inline Square uci_to_square(const std::string& uci) {
    return make_square(r2c.find(uci[1]), f2c.find(uci[0]));
}

#endif

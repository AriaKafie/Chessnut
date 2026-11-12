
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <algorithm>

#include "position.h"
#include "search.h"
#include "types.h"

enum {
    TT, GENERATE, SORT, READY
}; typedef int Stage;

enum {
    ALL, CAPTURES
}; typedef int GenType;

constexpr int MAX_MOVES    = 128;
constexpr int MAX_CAPTURES = 32;

template<Color, GenType>
EMove* generate(const Position& pos, EMove* list);

template<Color Us, GenType GenT = ALL>
class MoveList {

public:
    MoveList() = delete;
    MoveList(const Position& p) :
        pos(p),
        last(generate<Us, GenT>(pos, moves)) {}
    MoveList(const Position& p,
             Move            ttm,
             SearchInfo*     si);

    Move   next_move();
    EMove* begin() { return moves; }
    EMove* end() { return last; }
    size_t size() const { return last - moves; }
    bool   contains(Move m) { return std::find(begin(), end(), m) != end(); }

private:
    void sort();

    const Position& pos;
    EMove           moves[GenT == ALL ? MAX_MOVES : MAX_CAPTURES];
    EMove*          last;
    EMove*          cur;
    SearchInfo*     search_info;
    Stage           stage;
    Move            ttmove;
};

template<Color Us, GenType GenT>
MoveList<Us, GenT>::MoveList(const Position& p,
                             Move            ttm,
                             SearchInfo*     si) :
    last(moves),
    cur(moves),
    pos(p),
    ttmove(ttm),
    search_info(si) {

    if (ttmove) {
        *last++ = ttmove;
        stage = TT;
    } else {
        last = generate<Us, GenT>(pos, moves);
        stage = SORT;
    }
}

template<Color Us, GenType GenT>
Move MoveList<Us, GenT>::next_move() {

    switch (stage) {

    case TT:
        stage++;
        return *cur++;

    case GENERATE:
        stage++;
        last = generate<Us, GenT>(pos, moves);
        [[fallthrough]];

    case SORT:
        stage++;
        sort();
        [[fallthrough]];

    case READY:
    default:
        return cur < last ? *cur++ : NULLMOVE;
    }
}

#endif

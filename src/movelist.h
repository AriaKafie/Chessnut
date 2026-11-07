
#ifndef MOVELIST_H
#define MOVELIST_H

#include <algorithm>

#include "position.h"
#include "search.h"
#include "types.h"

enum {
    TT, GENERATE, GENERATED
}; typedef int Stage;

enum {
    ALL, CAPTURES
}; typedef int GenType;

constexpr int MAX_MOVES    = 128;
constexpr int MAX_CAPTURES = 32;

template<Color, GenType>
EMove* generate(const Position& pos, EMove* list);

//template<GenType Type>
//struct ListInfo {
//    EMove*      cur;
//    SearchInfo* search_info;
//    Stage       stage;
//    Move        ttmove;
//};
//
//template<>
//struct ListInfo<CAPTURES> {};
//
//template<GenType Type>
//class test {
//    EMove moves[32];
//    EMove *last;
//    const Position& pos;
//    ListInfo<Type> info;
//};

template<Color Us, GenType Type = ALL>
class MoveList {

public:
    MoveList() = delete;
    MoveList(const Position& p) :
        pos(p),
        last(generate<Us, Type>(pos, moves)) {}
    MoveList(const Position& p,
             Move            ttm,
             SearchInfo*     si);

    Move   next_move();
    EMove* begin() { return moves; }
    EMove* end() { return last; }
    size_t size() const { return last - moves; }
    bool   contains(Move m) { return std::find(begin(), end(), m) != end(); }
    void   sort();

private:
    Move next();
    
    const Position& pos;
    EMove           moves[Type == ALL ? MAX_MOVES : MAX_CAPTURES];
    EMove*          last;
    EMove*          cur;
    SearchInfo*     search_info;
    Stage           stage;
    Move            ttmove;
};

template<Color Us, GenType Type>
MoveList<Us, Type>::MoveList(const Position& p,
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
        last = generate<Us, Type>(pos, moves);
        sort();
        stage = GENERATED;
    }
}

template<Color Us, GenType Type>
Move MoveList<Us, Type>::next() {
    return cur < last ? *cur++ : NULLMOVE;
}

template<Color Us, GenType Type>
Move MoveList<Us, Type>::next_move() {

    switch (stage) {

    case TT:
        stage++;
        return next();

    case GENERATE:
        stage++;
        last = generate<Us, Type>(pos, moves);
        sort();
        return next();

    case GENERATED:
    default:
        return next();
    }
}

#endif

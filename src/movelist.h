
#ifndef MOVELIST_H
#define MOVELIST_H

#include <algorithm>

#include "position.h"
#include "search.h"
#include "types.h"

typedef int Stage;
enum Stages {
    TT, GENERATE, GENERATED
};

const int MAX_MOVES    = 128;
const int MAX_CAPTURES = 32;

template<Color Us, Color Them = !Us>
class MoveList {

public:
    MoveList() = delete;
    MoveList(const Position& p);
    MoveList(const Position& p,
             Move            ttm,
             SearchInfo*     si);

    Move   next_move();
    EMove* begin() { return moves; }
    EMove* end() { return last; }
    size_t size() const { return last - moves; }
    bool   contains(Move m) { return std::find(begin(), end(), m) != end(); }

private:
    void generate();
    void sort();
    Move next();
    
    EMove           moves[MAX_MOVES];
    EMove*          last = moves;
    EMove*          cur = moves;
    SearchInfo*     search_info;
    const Position& pos;
    Bitboard        seen_by_enemy;
    Stage           stage;
    Move            ttmove;
};

template<Color Us, Color Them>
MoveList<Us, Them>::MoveList(const Position& p) : pos(p) {
    generate();
}

template<Color Us, Color Them>
MoveList<Us, Them>::MoveList(const Position& p,
                             Move            ttm,
                             SearchInfo*     si) :
    pos(p),
    ttmove(ttm),
    search_info(si) {

    if (ttmove) {
        *last++ = ttmove;
        stage = TT;
    } else {
        generate();
        sort();
        stage = GENERATED;
    }
}

template<Color Us, Color Them>
Move MoveList<Us, Them>::next() {
    return cur < last ? *cur++ : NULLMOVE;
}

template<Color Us, Color Them>
Move MoveList<Us, Them>::next_move() {

    switch (stage) {

    case TT:
        stage++;
        return next();

    case GENERATE:
        stage++;
        last = moves;
        generate();
        sort();
        return next();

    case GENERATED:
    default:
        return next();
    }
}

template<Color Us, Color Them = !Us>
class CaptureList {

public:
    CaptureList() = delete;
    CaptureList(const Position& p);

    EMove* begin() { return moves; }
    EMove* end() { return last; }
    size_t size() const { return last - moves; }
    void   sort();

private:
    EMove           moves[MAX_CAPTURES], *last = moves;
    const Position& pos;
    Bitboard        seen_by_enemy;
};

#endif

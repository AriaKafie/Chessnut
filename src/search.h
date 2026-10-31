
#ifndef SEARCH_H
#define SEARCH_H

#include <chrono>

#include "position.h"
#include "types.h"

enum NodeType { ROOT, PV, NONPV };

typedef struct {
    int  static_ev;
    int  ply;
    Move killers[2];
} SearchInfo;

inline uint64_t unix_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

namespace Search {
    void set_verbose(bool b);
    void init();
    void go(Position pos, uint64_t thinktime = 0);
    void count_nodes(int depth);
}

#endif

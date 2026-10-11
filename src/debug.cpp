
#include "debug.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <vector>
#include <random>
#include <thread>

#include "evaluation.h"
#include "movegen.h"
#include "moveordering.h"
#include "position.h"
#include "transpositiontable.h"
#include "search.h"
#include "uci.h"

template<Color STM>
static std::string pv(Position& pos)
{
    if (pos.is_repetition())
        return "";

    std::string line;
    Move best = TranspositionTable::lookup_move(pos.key());

    if (best == NO_MOVE)
        return "";

    bool legal = false;

    MoveList<STM> moves(pos);
    for (Move m : moves)
        if (m == best) legal = true;

    if (!legal) return "";

    else {
        pos.do_move<STM>(best);
        line = move_to_uci(best) + " " + pv<!STM>(pos);
        pos.undo_move<STM>(best);
    }

    return line;
}

std::string Debug::pv(Position& pos) {
    return pos.white_to_move() ? ::pv<WHITE>(pos)
                               : ::pv<BLACK>(pos);
}

template<bool Root, Color SideToMove>
uint64_t PerfT(Position& pos, int depth)
{
    if (depth == 0)
        return 1;

    MoveList<SideToMove> moves(pos);

    if (depth == 1 && !Root)
        return moves.size();

    uint64_t count, nodes = 0;
    
    for (Move m : moves)
    {
        pos.do_move<SideToMove>(m);
        count = PerfT<false, !SideToMove>(pos, depth - 1);
        pos.undo_move<SideToMove>(m);

        nodes += count;

        if (Root)
            std::cout << move_to_uci(m) << ": " << count << std::endl;
    }

    return nodes;
}

void Debug::perft(Position pos, std::istringstream& is)
{
    if (int depth; is >> depth)
    {
        auto start = std::chrono::steady_clock::now();
        uint64_t result = pos.white_to_move() ? PerfT<true, WHITE>(pos, depth)
                                              : PerfT<true, BLACK>(pos, depth);
        auto end   = std::chrono::steady_clock::now();

        double elapsed = std::chrono::duration<double>(end - start).count();

        printf(R"(
Nodes searched: %llu

===========================
Total time (s) : %.3f
Nodes searched : %llu
Nodes/second   : %llu)""\n\n",
        result, elapsed, result, uint64_t(result / elapsed));
    }
    else
    {
        std::string        line, token;
        std::istringstream suite(perft_suite);
        bool               failed = false;

        while (std::getline(suite, line))
        {
            pos.set(line.substr(0, line.find(';')));

            std::istringstream is(line.substr(line.find(';')));

            for (uint64_t depth = is.str()[2] - '0', expected; is >> token >> expected; depth++)
            {
                std::cout << "Perft " << depth << " " << pos.fen() << std::endl;

                uint64_t result = pos.white_to_move() ? PerfT<false, WHITE>(pos, depth)
                                                      : PerfT<false, BLACK>(pos, depth);

                if (result != expected)
                {
                    failed = true;
                    std::cout << "ERROR\n" << std::endl;
                    break;
                }

                if (is.eof())
                    std::cout << "OK\n" << std::endl;
            }
        }

        std::cout << (failed ? "FAILED\n" : "ALL OK\n") << std::endl;
    }
}

std::string rep_table_to_string() {

    /*extern RTEntry repetition_table[];

    std::ostringstream os;
    std::string s = "+------------------+------+----+\n";

    os << s << "| key              | loc  |  # |\n" << s;

    for (int i = 0; i < RT_SIZE; i++) {
        if (RTEntry& ri = repetition_table[i]; ri.occurrences) {
            os << "| " << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << ri.key;
            os << " | " << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << i;
            os << " | " << std::setw(2) << std::setfill(' ') << std::dec << (int(ri.occurrences)) << " |\n";
        }
    }

    return os.str() + s;*/
    return "";
}

/*Bitboard negative_spans(Color c, Bitboard pawns)
{
    Bitboard spans = 0;

    for (;pawns; clear_lsb(pawns))
    {
        Square s = bsf(pawns);
        spans |= (file_bb(s) | file_bb(s + EAST) & ~FILE_ABB | file_bb(s + WEST) & ~FILE_HBB) & mask(s, relative_direction(c, SOUTH));
    }

    return ~spans;
}*/

static void go() {
    std::cout << "position_class" << std::endl;

    /*Position pos;
    SearchInfo si = {};
    MoveList<WHITE> ml(pos, 0, &si);

    std::sort(ml.begin(), ml.end(), [](EMove a, EMove b) { return score_of(a) > score_of(b); });

    for (EMove m : ml)
        std::cout << move_to_uci(m) << ": " << score_of(m) << std::endl;*/
}

void Debug::go() {::go();}

Move* get_moves(const Position& pos, Move *list)
{
#define populate(c) \
    { MoveList<c> moves(pos); for (Move m : moves) *list++ = m; return list; }

    if (pos.white_to_move()) populate(WHITE) else populate(BLACK);
#undef populate
}

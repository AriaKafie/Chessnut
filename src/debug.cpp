
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
#include "movelist.h"
#include "position.h"
#include "transpositiontable.h"
#include "search.h"
#include "uci.h"

template<Color STM>
static std::string pv(Position& pos, int ply)
{
    if (pos.is_repetition() || ply > MAX_DEPTH)
        return "";

    std::string line;
    Move best = TranspositionTable::lookup_move(pos.key());

    if (best == NO_MOVE)
        return "";

    if (!pos.legal<STM>(best))
        return "";

    else {
        pos.do_move<STM>(best);
        line = move_to_uci(best) + " " + pv<!STM>(pos, ply + 1);
        pos.undo_move<STM>(best);
    }

    return line;
}

std::string Debug::pv(Position& pos) {
    return pos.white_to_move() ? ::pv<WHITE>(pos, 0)
                               : ::pv<BLACK>(pos, 0);
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

void save_tt(const std::string& filename) {

    extern TTEntry transposition_table[TT_SIZE];

    std::ofstream fs(filename, std::ios::binary | std::ios::trunc);
    if (!fs) {
        std::cerr << "Failed to open " << filename << " for writing" << std::endl;
        std::exit(1);
    }

    fs.write(reinterpret_cast<const char*>(transposition_table),
        sizeof(transposition_table));

    if (!fs) {
        std::cerr << "Failed to write " << filename << std::endl;
        std::exit(1);
    }
}

void load_tt(std::istringstream& args) {

    extern TTEntry transposition_table[TT_SIZE];

    std::string filename = "tt.bin";
    if (!args.eof())
        args >> filename;

    std::cout << "Loading " << filename << "...\n";

    std::ifstream fs(filename, std::ios::binary);
    if (!fs) {
        std::cerr << "Failed to open " << filename << " for reading\n";
        return;
    }

    constexpr std::streamsize size = sizeof(transposition_table);

    fs.seekg(0, std::ios::end);
    if (fs.tellg() != size) {
        std::cerr << filename << " has an incorrect size\n";
        return;
    }

    fs.seekg(0, std::ios::beg);
    fs.read(reinterpret_cast<char*>(transposition_table), size);

    if (!fs) {
        std::cerr << "Failed to read " << filename << std::endl;
        return;
    }

    std::cout << "Loaded " << size << " bytes.\n";
}

static void go() {
    std::cout << "setoption_verbose" << std::endl;
}

void Debug::go() {::go();}

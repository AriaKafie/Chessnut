
#include "uci.h"

#include <iostream>
#include <sstream>

#include "debug.h"
#include "movelist.h"
#include "position.h"
#include "search.h"
#include "transpositiontable.h"

void position(Position& pos, std::istringstream& is) {

    std::string fen, token;
    is >> token;

    if (token == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
        is >> token;
    }

    else if (token == "fen")
        for (;is >> token && token != "moves"; fen += token + " ");

    pos.set(fen);
    
    for (Move m; is >> token && (m = uci_to_move(pos, token)); pos.commit_move(m));
}

void go(const Position& pos, std::istringstream& is) {

    std::string token;
    if (!(is >> token)) { Search::go(pos); return; }

    if (int movetime; token == "movetime" && is >> movetime)
        Search::go(pos, movetime);

    else if (int depth; token == "nodes" && is >> depth)
        Search::count_nodes(depth);

    else {

        is.str(is.str());
        is.clear();

        uint64_t wtime = 10'000, winc = 100, btime = 10'000, binc = 100;

        while (is >> token) {
            if (token == "wtime") is >> wtime;
            if (token == "winc")  is >> winc;
            if (token == "btime") is >> btime;
            if (token == "binc")  is >> binc;
        }

        uint64_t movetime = pos.white_to_move() ? wtime / 20 + winc / 2
                                                : btime / 20 + binc / 2;
        Search::go(pos, movetime);
    }
}

void setoption(std::istringstream& is) {
    
    std::string token;
    std::string name;
    std::string value;

    is >> token;

    while (is >> token && token != "value")
        name += (name.empty() ? "" : " ") + token;

    while (is >> token)
        value += (value.empty() ? "" : " ") + token;

    if (name == "Verbose")
        Search::set_verbose(value == "true");
}

void UCI::loop()
{
    Position pos(STARTPOS);

    std::string cmd, token;

    do
    {
        std::getline(std::cin, cmd);

        std::istringstream is(cmd);
        is >> token;

        if      (token == "uci")     std::cout << "id name Chessnut\n"
                                               << "id author Aria Kafie\n"
                                               << "option name Verbose type check default true\n"
                                               << "uciok"         << std::endl;
        else if (token == "isready") std::cout << "readyok"       << std::endl;
        else if (token == "d")       std::cout << pos.to_string() << std::endl;
        else if (token == "fen")     std::cout << pos.fen()       << std::endl;
        else if (token == "bmi")
#ifdef BMI
            std::cout << "yes" << std::endl;
#else
            std::cout << "no"  << std::endl;
#endif
        else if (token == "ucinewgame")
            TranspositionTable::clear();

        else if (token == "position")  position(pos, is);
        else if (token == "go")        go(pos, is);
        else if (token == "perft")     Debug::perft(pos, is);
        else if (token == "debug")     Debug::go();
        else if (token == "save")      save_tt();
        else if (token == "load")      load_tt(is);
        else if (token == "setoption") setoption(is);
        else if (token == "moves")
            for (Move m; is >> token && (m = uci_to_move(pos, token)); pos.commit_move(m));
        
    } while (cmd != "quit");
}

Move uci_to_move(const Position& pos, const std::string& uci) {

#define utm(C)                         \
    [&]() {                            \
        MoveList<C> ml(pos);           \
        for (Move m : ml)              \
            if (move_to_uci(m) == uci) \
                return m;              \
        return NULLMOVE;               \
    }()
    
    return pos.white_to_move() ? utm(WHITE)
                               : utm(BLACK);
#undef utm
}

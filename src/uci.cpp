
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

    if ((is >> std::ws).eof()) { Search::go(pos); return; }

    std::string token;
    is       >> token;

    if (int movetime; token == "movetime" && is >> movetime)
        Search::go(pos, movetime);

    else if (int depth; token == "nodes" && is >> depth)
        Search::count_nodes(depth);

    else {

        is.str(is.str()), is.clear();

        int wtime = 100, winc = 0, btime = 100, binc = 0;

        for (std::string token; is >> token;) {
            if (token == "wtime") is >> wtime;
            if (token == "winc")  is >> winc;
            if (token == "btime") is >> btime;
            if (token == "binc")  is >> binc;
        }

        Search::go(pos, pos.white_to_move() ? (wtime + winc) / 15 : (btime + binc) / 15);
    }
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
        else if (token == "save")      save_tt(is);
        else if (token == "load")      load_tt(is);
        else if (token == "noverbose") Search::noverbose();
        else if (token == "moves")
            for (Move m; is >> token && (m = uci_to_move(pos, token)); pos.commit_move(m));
        
    } while (cmd != "quit");
}

Move uci_to_move(const Position& pos, const std::string& uci) {

    for (EMove moves[MAX_MOVES], *m = pos.white_to_move()
        ? (EMove*)memcpy(moves, MoveList<WHITE>(pos).begin(), sizeof(EMove) * MAX_MOVES)
        : (EMove*)memcpy(moves, MoveList<BLACK>(pos).begin(), sizeof(EMove) * MAX_MOVES); *m; m++) {
         
        if (move_to_uci(*m) == uci)
            return *m;
    }

    return NULLMOVE;
}

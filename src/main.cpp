
#include "bitboard.h"
#include "movelist.h"
#include "position.h"
#include "search.h"
#include "uci.h"

int main() {

    Bitboards::init();
    Search::init();
    Position::init();
    MoveGen::init();

    UCI::loop();
}

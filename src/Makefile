
HEADERS = bitboard.h debug.h evaluation.h movelist.h position.h search.h transpositiontable.h types.h uci.h

FLAGS = -fpermissive -std=c++17 -march=native -w -O3 -c

OBJS = bitboard.o debug.o main.o movegen.o moveordering.o position.o search.o transpositiontable.o uci.o

Chessnut: $(OBJS)
	g++ $(OBJS) -o Chessnut

bitboard.o: bitboard.cpp $(HEADERS)
	g++ $(FLAGS) bitboard.cpp

debug.o: debug.cpp $(HEADERS)
	g++ $(FLAGS) debug.cpp

main.o: main.cpp $(HEADERS)
	g++ $(FLAGS) main.cpp

movegen.o: movegen.cpp $(HEADERS)
	g++ $(FLAGS) movegen.cpp

moveordering.o: moveordering.cpp $(HEADERS)
	g++ $(FLAGS) moveordering.cpp

position.o: position.cpp $(HEADERS)
	g++ $(FLAGS) position.cpp

search.o: search.cpp $(HEADERS)
	g++ $(FLAGS) search.cpp

transpositiontable.o: transpositiontable.cpp $(HEADERS)
	g++ $(FLAGS) transpositiontable.cpp

uci.o: uci.cpp $(HEADERS)
	g++ $(FLAGS) uci.cpp

clean:
	rm -rf *.o *~ Chessnut

debug:
	g++ -fpermissive -std=c++17 -march=native -w -g -odebug *.cpp

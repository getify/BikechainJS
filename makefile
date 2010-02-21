##  BikechainJS (makefile)
##	v0.0.1 (c) Kyle Simpson
##	MIT License

engine	: engine.o
	g++ -o engine -lv8 src/obj/engine.o
	
engine.o : engine.cpp
	g++ -c src/engine.cpp -Isrc/includes/ -osrc/obj/engine.o

clean:
	rm src/obj/*.o; strip engine

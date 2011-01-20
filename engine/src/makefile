##  BikechainJS (makefile)
##	v0.0.1.9 (c) Kyle Simpson
##	MIT License


engine	: engine.o
	g++ -o engine -lv8 engine.o
	
engine.o : engine.cpp
	g++ -c engine.cpp -I/path/to/V8/include -I./

install	: engine
	strip ./engine
	cp ./engine ../engine
	chmod 750 ../engine

clean:
	rm *.o

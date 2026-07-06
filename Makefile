all: mash 

clean:
	rm -f mash 

mash: main.cpp
	g++ -o mash main.cpp -Wall

mash_debug: main.cpp
	g++ -g -o mash_debug main.cpp -Wall

run: mash 
	./mash

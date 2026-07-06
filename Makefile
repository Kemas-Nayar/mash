all: mash 

clean:
	rm -f mash 

mash: bashell.cpp
	g++ -o mash bashell.cpp -Wall

mash_debug: bashell.cpp
	g++ -g -o mash_debug bashell.cpp -Wall

run: mash 
	./mash

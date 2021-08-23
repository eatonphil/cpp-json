main: *.cpp ./include/*.hpp
	clang++ -g -Wall -std=c++2a -I./include *.cpp -o $@

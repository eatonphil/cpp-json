main: *.cpp ./include/*.hpp
	clang++ -g -Wall -std=c++2a -I./include *.cpp -o $@


.PHONY: format
format: *.cpp ./include/*.hpp
	clang-format -i $^


.PHONY: hello

hello: main.cpp
	g++ -pthread -Wall -Werror -o "main" "main.cpp"
run: main
	./main


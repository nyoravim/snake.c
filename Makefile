CC := clang

snake: snake.o
	$(CC) -lcurses -o $@ $^

snake.o: snake.c
	$(CC) -c -o $@ $^

.PHONY: clean
clean:
	rm snake snake.o

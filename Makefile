CC := clang

snake: snake.o
	$(CC) -lcurses -o $@ $^

snake.o: snake.c
	$(CC) -c -o $@ $^

.PHONY: snake

clean:
	rm snake snake.o

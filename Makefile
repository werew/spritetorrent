


FLAGS= -I include -Wall -Werror

tracker: src/tracker.c src/network.c
	gcc $(FLAGS) $^ -o $@

clean:
	rm -f tracker


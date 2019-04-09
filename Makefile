rawserv: main.c Makefile
	gcc $< -Wall -Wextra -Werror -o $@

clean:
	rm -f rawserv

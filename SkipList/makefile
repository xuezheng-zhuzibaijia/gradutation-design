all:main
main:main.c skip_list.c skip_list.h
	cc -std=gnu99 -o main main.c skip_list.c -lpmemobj -lpmem
clean:
	rm -f *.save *.jpg main skipfile

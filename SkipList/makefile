all:main reader
main:main.c skip_list.c skip_list.h
	cc -std=gnu99 -o main main.c skip_list.c -lpmemobj -lpmem
reader:reader.c skip_list.c skip_list.h
	cc -std=gnu99 -o reader reader.c skip_list.c -lpmemobj -lpmem
clean:
	rm -f *.save *.jpg main  reader skipfile *.dot

all:writer reader display
writer:writer.c btree.c
	cc -std=gnu99 -o writer writer.c btree.c -lpmemobj -lpmem
reader:reader.c btree.c
	cc -std=gnu99 -o reader reader.c btree.c -lpmemobj -lpmem
display:display.c btree.h
	cc -std=gnu99 -o display display.c -lpmemobj -lpmem
clean:
	rm -f writer reader display treefile  *.c.save *.jpg; 

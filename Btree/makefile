all:writer reader
writer:writer.c btree.c
	cc -std=gnu99 -o writer writer.c btree.c -lpmemobj -lpmem
reader:reader.c btree.c
	cc -std=gnu99 -o reader reader.c btree.c -lpmemobj -lpmem
clean:
	rm -f writer reader treefile  *.c.save *.jpg; 

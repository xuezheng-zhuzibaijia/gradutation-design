all:writer reader delete display
writer:writer.c btree.c
	cc -std=gnu99 -o writer writer.c btree.c -lpmemobj -lpmem
reader:reader.c btree.c
	cc -std=gnu99 -o reader reader.c btree.c -lpmemobj -lpmem
delete:delete_test.c btree.c
	cc -std=gnu99 -o delete delete_test.c btree.c -lpmemobj -lpmem
display:display.c btree.c
	cc -std=gnu99 -o display display.c btree.c -lpmemobj -lpmem
clean:
	rm -f writer reader delete display treefile  *.c.save *.jpg; 

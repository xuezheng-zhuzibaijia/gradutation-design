avltest:
	gcc -std=gnu99 -o avl_test avl_test.c avl.c
palm_test:
	gcc -std=gnu99 -o palm_test palm_test.c palm.c btree.c avl.c -lpthread -lpmem -lpmemobj
clean: 
	rm -f *.save avl_test palm_test

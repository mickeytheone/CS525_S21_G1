.PHONY: all
all: test_assign4_1 

test_assign4_1: test_assign4_1.c storage_mgr.c dberror.c buffer_mgr_stat.c buffer_mgr.c rm_serializer.c record_mgr.c expr.c btree_mgr.c
	gcc -o test_assign4_1 -Wall -g test_assign4_1.c storage_mgr.c dberror.c buffer_mgr_stat.c buffer_mgr.c rm_serializer.c record_mgr.c expr.c btree_mgr.c

.PHONY: clean
clean:
	rm test_assign4_1 

.PHONY: all
all: test_assign3_1 test_expr

test_assign3_1: test_assign3_1.c storage_mgr.c dberror.c buffer_mgr_stat.c buffer_mgr.c rm_serializer.c record_mgr.c expr.c
	gcc -o test_assign3_1 -Wall -g test_assign3_1.c storage_mgr.c dberror.c buffer_mgr_stat.c buffer_mgr.c rm_serializer.c record_mgr.c expr.c
	
test_expr: test_expr.c storage_mgr.c dberror.c buffer_mgr_stat.c buffer_mgr.c rm_serializer.c record_mgr.c expr.c
	gcc -o test_expr -Wall -g test_expr.c storage_mgr.c dberror.c buffer_mgr_stat.c buffer_mgr.c rm_serializer.c record_mgr.c expr.c

.PHONY: clean
clean:
	rm test_assign3_1 test_expr

#ifndef REPL_INCLUDED
	#define REPL_INCLUDED

	#include <stdbool.h>
	#include "lisp_machine.h"

	#define RUNTIME_LINES 10
	#define MAX_PRINT_EXPR_LENGTH 80
	#define PRINT_EXPR_PADDING 20  // Used to help protect against the imperfect function of print_list_helper
	#define MAX_PRINT_STACK_DEPTH 20

	extern bool quiet_flag;
	extern bool runtime_info_flag;
	extern bool verbose_flag;
	
	extern Lisp_Machine * machine;

	void process_args(int argc, char * argv[]);
	void print_runtime_info();
	void print_runtime_stack();

	void print_list(Cell *cell);
	int print_list_helper(Cell *list, char *string, int *index, bool is_in_list);

#endif
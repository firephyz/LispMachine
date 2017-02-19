#ifndef REPL_INCLUDED
	#define REPL_INCLUDED

	#include <stdbool.h>
	#include "lisp_machine.h"

	#define RUNTIME_LINES 10
	#define MAX_PRINT_EXPR_LENGTH 80
	#define MAX_PRINT_STACK_DEPTH 20

	extern bool quiet_flag;
	extern bool runtime_info_flag;
	extern bool verbose_flag;
	
	extern Lisp_Machine * machine;

	void process_args(int argc, char * argv[]);
	void print_list(Cell *cell);
	void print_runtime_info();

#endif
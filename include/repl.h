#ifndef REPL_INCLUDED
	#define REPL_INCLUDED

	#include <stdbool.h>
	#include "lisp_machine.h"

	extern bool quiet_flag;
	
	extern Lisp_Machine * machine;

	void process_args(int argc, char * argv[]);
	void print_list(Cell *cell);

#endif
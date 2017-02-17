/* To-Do
 *
 *
 * Fix the make_expression function so that there is less redundancy in code (and make for elegant)
 */


#define _POSIX_C_SOURCE 1
#include "repl.h"
#include "lisp_machine.h"
#include "expr_parser.h"
#include "stack.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool quiet_flag;
Lisp_Machine * machine;

int main(int argc, char * argv[]) {

	// Init variables and the machine
	process_args(argc, argv);

	if(!quiet_flag) {
		printf("\n");
		printf("********************\n");
		printf("* LISP Interpreter *\n");
		printf("********************\n\n");
	}

	machine = init_machine();

	if(!quiet_flag) {
		printf("Starting session...\n\n");
	}
	else {
		printf("\n");
	}

	// Begin execution of the machine
	execute();

	print_list(machine->result);

	destroy_machine(machine);
}

void print_list(Cell *list) {

	// Handles the case when it is only the empty list
	if(list == machine->nil) {
		printf("()\n");
		return;
	}

	Cell * cell = list;
	bool is_going_up = false;

	Stack s;
	MAKE_STACK(s, Cell *);
	PUSH(s, Cell *, cell);
	printf("(");

	while(s.n != 0 || is_going_up) {
		// If we have already traversed down, then we must go over one
		if(is_going_up) {
			// Go across the current list to the next element
			if(cell->cdr != machine->nil) {
				printf(" ");
				cell = cell->cdr;
				PUSH(s, Cell *, cell);
				is_going_up = false;
			}
			// Print the end of a list
			else {
				printf(")");

				if(s.n != 0) {
					POP(s, Cell *, cell);
				}
				else {
					is_going_up = false;
				}
			}
		}
		// Print the symbol if it exists
		else if(cell->car->is_atom) {
			char * symbol = get_symbol_name(cell->car);
			printf("%s", symbol);
			free(symbol);
			POP(s, Cell *, cell);
			is_going_up = true;
		}
		// Traverse down
		else {
			printf("(");
			cell = cell->car;
			PUSH(s, Cell *, cell);
		}
	}

	printf("\n");
}

void process_args(int argc, char * argv[]) {

	quiet_flag = false;

	for(int i = 1; i < argc; ++i) {
		if(strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
			quiet_flag = true;
		}
		else {
			fprintf(stderr, "Unrecognized command line option '%s'.\n", argv[i]);
			fprintf(stderr, "Exiting...\n");
			exit(EXIT_FAILURE);
		}
	}
}
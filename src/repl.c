/* To-Do
 *
 * Consider lexical vs dynamics scoping
 *
 * Finish adding number support
 *
 * Add input and output
 *
 * Add function definitions?
 *
 * Think about how the machine will compile expressions given as strings
 *
 * Fix the make_expression function so that there is less redundancy in code (and make for elegant)
 */


#include "repl.h"
#include "lisp_machine.h"
#include "expr_parser.h"
#include "stack.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool quiet_flag;
bool runtime_info_flag;
bool verbose_flag;

Lisp_Machine * machine;

int main(int argc, char * argv[]) {

	// Init variables and the machine
	process_args(argc, argv);

	if(!quiet_flag) {
		printf("\n");
		printf(" > ********************\n");
		printf(" > *  LISP Emulator   *\n");
		printf(" > ********************\n\n");
	}

	machine = init_machine();

	if(!quiet_flag) {
		printf(" > Starting session...\n\n");
	}

	if(runtime_info_flag) {
		for(int i = 0; i < RUNTIME_LINES + MAX_PRINT_STACK_DEPTH; ++i) {
			printf("\n");
		}
	}

	// Begin execution of the machine
	execute();

	printf(" > ");
	print_list(machine->result);

	destroy_machine(machine);
}

void print_runtime_info(char * func) {

	// Clear previous print
	printf("\033[%dA", RUNTIME_LINES + MAX_PRINT_STACK_DEPTH);
	for(int i = 0; i < RUNTIME_LINES + MAX_PRINT_STACK_DEPTH; ++i) {
		for(int j = 0; j < MAX_PRINT_EXPR_LENGTH + 15; ++j) {
			printf(" ");
		}
		printf("\n");
	}

	// Print runtime info
	printf("\033[%dA", RUNTIME_LINES + MAX_PRINT_STACK_DEPTH);
	printf("In Use: %-10d\n", machine->mem_used);
	printf("Stack Depth: %-10d\n", machine->sys_stack_size);
	printf("\n");
	printf("Func: %-30s\n", func);	
	printf("Arg 0: ");
	print_list(machine->args[0]);
	printf("Arg 1: ");
	print_list(machine->args[1]);
	printf("Arg 2: ");
	print_list(machine->args[2]);
	printf("Arg 3: ");
	print_list(machine->args[3]);
	printf("Result: ");
	print_list(machine->result);
	printf("\n");

	print_runtime_stack();
}

void print_runtime_stack() {

	int index = 0;
	Cell * stack = machine->sys_stack;
	while(index < MAX_PRINT_STACK_DEPTH) {

		if(stack == machine->nil) {
			for(int i = 0; i < MAX_PRINT_STACK_DEPTH - index; ++i) {
				printf("\n");
			}
			break;
		}

		if(stack->type == SYS_RETURN_RECORD) {
			switch((int)(uintptr_t)stack->car) {
				case SYS_REPL:
					printf("%s\n", "repl");
					break;
				case SYS_EVAL_0:
					printf("%s\n", "eval");
					break;
				case SYS_EVAL_1:
					printf("%s\n", "eval");
					break;
				case SYS_APPLY_0:
					printf("%s\n", "apply");
					break;
				case SYS_APPLY_1:
					printf("%s\n", "apply");
					break;
				case SYS_APPLY_2:
					printf("%s\n", "apply");
					break;
				case SYS_EVLIS_0:
					printf("%s\n", "evlis");
					break;
				case SYS_EVLIS_1:
					printf("%s\n", "evlis");
					break;
				case SYS_EVIF_0:
					printf("%s\n", "evif");
					break;
				case SYS_EVIF_1:
					printf("%s\n", "evif");
					break;
				case SYS_CONENV_0:
					printf("%s\n", "conenv");
					break;
				case SYS_LOOKUP_0:
					printf("%s\n", "lookup");
					break;
			}
			++index;
		}
		
		stack = stack->cdr;
	}
}

void print_list(Cell *list) {

	char string[MAX_PRINT_EXPR_LENGTH + 1];

	// NULL is true, machine->nil is false
	if(list == NULL) {
		string[0] = 'T';
		string[1] = '\n';
		string[2] = '\0';
	}
	else if(list->is_atom) {

		// Handles the case when it is only the empty list
		if(list == machine->nil) {
			string[0] = '(';
			string[1] = ')';
			string[2] = '\n';
			string[3] = '\0';
		}
		// Else must be a normal symbol
		else {
			char * symbol = get_symbol_name(list);
			strncpy(string, symbol, MAX_PRINT_EXPR_LENGTH);
			string[strlen(symbol)] = '\n';
			string[strlen(symbol) + 1] = '\0';
			free(symbol);
		}
	}
	else {
		Cell * cell = list;
		bool is_going_up = false;

		Stack s;
		MAKE_STACK(s, Cell *);
		PUSH(s, Cell *, cell);

		int index = 0;
		string[index] = '(';
		++index;

		while(s.n != 0 || is_going_up) {

			if(index > MAX_PRINT_EXPR_LENGTH - 5) {
				string[MAX_PRINT_EXPR_LENGTH - 5] = '.';
				string[MAX_PRINT_EXPR_LENGTH - 4] = '.';
				string[MAX_PRINT_EXPR_LENGTH - 3] = '.';
				string[MAX_PRINT_EXPR_LENGTH - 2] = '\n';
				string[MAX_PRINT_EXPR_LENGTH - 1] = '\0';
				printf("%s", string);
				return;
			}

			// If we have already traversed down, then we must go over one
			if(is_going_up) {
				// Go across the current list to the next element
				if(cell->cdr != machine->nil) {
					string[index] = ' ';
					++index;
					cell = cell->cdr;
					PUSH(s, Cell *, cell);
					is_going_up = false;
				}
				// Print the end of a list
				else {
					string[index] = ')';
					++index;

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
				strncpy(string + index, symbol, MAX_PRINT_EXPR_LENGTH - index);
				index += strlen(symbol);
				free(symbol);
				POP(s, Cell *, cell);
				is_going_up = true;
			}
			// Traverse down
			else {
				string[index] = '(';
				++index;
				cell = cell->car;
				PUSH(s, Cell *, cell);
			}
		}

		string[index] = '\n';
		++index;
	}

	printf("%s", string);
}

void process_args(int argc, char * argv[]) {

	quiet_flag = false;
	runtime_info_flag = false;

	for(int i = 1; i < argc; ++i) {
		if(strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
			quiet_flag = true;
		}
		else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--show-runtime-info") == 0) {
			runtime_info_flag = true;
		}
		else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			verbose_flag = true;
		}
		else {
			fprintf(stderr, "Unrecognized command line option '%s'.\n", argv[i]);
			fprintf(stderr, "Exiting...\n");
			exit(EXIT_FAILURE);
		}
	}

	if(verbose_flag && quiet_flag) {
		fprintf(stderr, "Conflicting flags: '%s', '%s'\n", "--quiet", "--verbose");
		fprintf(stderr, "Exiting...\n");
		exit(EXIT_FAILURE);
	}
}
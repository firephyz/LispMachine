/* To-Do
 *
 * Implement more emulation facilities like memory accesses, cycle count, etc.
 *
 * Consider lexical vs dynamics scoping
 *
 * Fix the evaluator so that SYSCALL's are tail recursive.
 *
 * Add function definitions?
 *
 * Implement make_expression in software of the machine instead of as an external function
 *
 * Fix the make_expression function so that there is less redundancy in code (and make for elegant)
 *
 * Fix the print_list_helper function to better protect against buffer overflows
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
		printf(" => ********************\n");
		printf(" => *  LISP Emulator   *\n");
		printf(" => ********************\n\n");
	}

	machine = init_machine();

	if(!quiet_flag) {
		printf(" => Starting session...\n\n");
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
	printf("\n");

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
				case SYS_EVAL:
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
				case SYS_EVIF:
					printf("%s\n", "evif");
					break;
				case SYS_CONENV:
					printf("%s\n", "conenv");
					break;
				default:
					printf("UNKNOWN: %d\n", (int)(intptr_t)stack->car);
					break;
			}
			++index;
		}
		
		stack = stack->cdr;
	}
}

void print_list(Cell *list) {

	char string[MAX_PRINT_EXPR_LENGTH + 1];

	int index = 0;

	print_list_helper(list, string, &index, false);
	string[index] = '\0';

	printf("%s\n", string);
}

// Returns 1 if the string gets too long, 0 otherwise
// Crudely protects against overflowing the buffer but it isn't a perfect solution. Printing of parens,
// spaces and periods in between recursive calls could in theory overflow the buffer.
int print_list_helper(Cell *list, char *string, int *index, bool is_in_list) {

	// Check if we are past the end of the buffer
	if(*index >= MAX_PRINT_EXPR_LENGTH - PRINT_EXPR_PADDING) {
		string[*index - 3] = '.';
		string[*index - 2] = '.';
		string[*index - 1] = '.';

		return 1;
	}

	// Remember, NULL means true. I really need to change that...
	if(list == NULL) {
		string[*index] = 'T';
		++*index;
		return 0;
	}

	if(!list->is_atom) {
		// Executed when we reach the end of list of cells
		if(list->cdr == machine->nil) {
			// Executed when we reach the end of a list
			if(is_in_list) {
				if(print_list_helper(list->car, string, index, false)) {
					return 1;
				}
			}
			// Could have been just a list
			// of one element in which case we need this special case.
			else {
				string[*index] = '(';
				++*index;

				if(print_list_helper(list->car, string, index, false)) {
					return 1;
				}

				string[*index] = ')';
				++*index;
			}
		}
		// Handle lists
		else {
			bool car_is_atom = list->car->is_atom;
			bool cdr_is_pair = !(list->cdr == NULL) && !list->cdr->is_atom; // NULL means true. Thus is an atom
			bool is_dotted_pair = car_is_atom & (!cdr_is_pair);

			// We are in the middle of a list
			if(is_in_list) {
				if(is_dotted_pair) {
					if(print_list_helper(list->car, string, index, false)) {
						return 1;
					}

					string[*index] = ' ';
					string[*index + 1] = '.';
					string[*index + 2] = ' ';
					*index += 3;

					if(print_list_helper(list->cdr, string, index, cdr_is_pair)) {
						return 1;
					}
				}
				else {
					if(print_list_helper(list->car, string, index, false)) {
						return 1;
					}

					string[*index] = ' ';
					++*index;

					if(print_list_helper(list->cdr, string, index, cdr_is_pair)) {
						return 1;
					}
				}
			}
			// We are at the start of a list
			else {
				if(is_dotted_pair) {
					string[*index] = '(';
					++*index;

					if(print_list_helper(list->car, string, index, false)) {
						return 1;
					}

					string[*index] = ' ';
					string[*index + 1] = '.';
					string[*index + 2] = ' ';
					*index += 3;

					if(print_list_helper(list->cdr, string, index, cdr_is_pair)) {
						return 1;
					}

					string[*index] = ')';
					++*index;
				}
				else {
					string[*index] = '(';
					++*index;

					if(print_list_helper(list->car, string, index, false)) {
						return 1;
					}

					string[*index] = ' ';
					++*index;

					if(print_list_helper(list->cdr, string, index, cdr_is_pair)) {
						return 1;
					}

					string[*index] = ')';
					++*index;
				}
			}
		}
	}
	// Handle printing of the null symbol
	else if(list == machine->nil) {
		string[*index] = '(';
		string[*index + 1] = ')';
		*index += 2;
	}
	// Print a symbol
	else {
		char * name = get_symbol_name(list);
		strncpy(string + *index, name, MAX_PRINT_EXPR_LENGTH - *index);
		*index += strlen(name);
		free(name);
	}

	return 0;
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
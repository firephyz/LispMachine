#include "lisp_machine.h"
#include "expr_parser.h"
#include "repl.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int chars_per_pointer = sizeof(uintptr_t) / sizeof(char);
Lisp_Machine * machine;

Lisp_Machine * init_machine() {

	machine = malloc(sizeof(Lisp_Machine));
	machine->is_running = true;
	machine->mem_used = 0;
	machine->mem_free = NUM_OF_CELLS;

	if(verbose_flag) {
		printf("Initializing machine...\n");
	}

	// Create and link the memory cells
	machine->memory_block = calloc(sizeof(Cell) * NUM_OF_CELLS, sizeof(Cell));
	machine->free_mem = machine->memory_block;
	for(int i = 0; i < NUM_OF_CELLS - 1; ++i) {
		machine->free_mem[i].cdr = &machine->free_mem[i + 1];
	}

	// Setup the nil atom
	machine->nil = get_free_cell();
	machine->nil->car = machine->nil;
	machine->nil->cdr = machine->nil;
	machine->nil->is_atom = true;

	// Initialize the supported instruction lists
	// null, false and true are pseudo system symbols. They get
	// translated to something else during parsing
	init_instr_list("* + - / < = > and atom? car cdr cons charat eq? eval false if in join lambda mod not null or out quit quote substr true");

	// Initialize the machine system environment
	machine->sys_stack = machine->nil;
	machine->sys_stack_size = 0;

	machine->memory_access_count = 0;
	machine->cycle_count = 0;

	if(verbose_flag) {
		printf("Machine initialized!\n");
	}

	return machine;
}

void destroy_machine(Lisp_Machine *machine) {

	free(machine->instr_memory_block);
	free(machine->instructions);
	free(machine->memory_block);
	free(machine);
}

Cell * get_free_cell() {

	++machine->mem_used;
	--machine->mem_free;

	Cell * new_cell = machine->free_mem;
	machine->free_mem = cdr(machine->free_mem);
	new_cell->cdr = NULL;

	return new_cell;
}

void store_cell(Cell * cell) {
	cell->car = machine->free_mem;
	machine->free_mem = cell;
}

// Pushes the given number of arguments from the machine->args registers
// onto the system stack. Tops it with the calling function record.
void push_system_args(int arg_count) {

	// If all the function call needs is a return address, then we will signal that.
	// Then, this stack frame can simply be replaced by then
	// if(arg_count == 0) {
	// 	machine->needs_return_address = false;
	// }
	// else {
	// 	machine->needs_return_address = true;
	// }

	// Push arguments still needed for calling function
	for(int i = 0; i < arg_count; ++i) {
		Cell * cell = get_free_cell();
		cell->car = machine->args[arg_count - i - 1];
		cell->cdr = machine->sys_stack;
		machine->sys_stack = cell;
		++machine->sys_stack_size;
	}

	// Record calling function
	Cell * cell = get_free_cell();
	cell->type = SYS_RETURN_RECORD;
	cell->car = (Cell *)(intptr_t)machine->calling_func;
	cell->cdr = machine->sys_stack;
	machine->sys_stack = cell;
	++machine->sys_stack_size;
}

// Pops the calling function and then pops the arguments in the registers
void pop_system_args() {

	// Restore the calling function
	machine->calling_func = (uint8_t)(intptr_t)machine->sys_stack->car;
	machine->sys_stack = machine->sys_stack->cdr;
	--machine->sys_stack_size;

	int arg_count = 0;
	// Restore arguments
	while(machine->sys_stack->type != SYS_RETURN_RECORD && machine->sys_stack != machine->nil) {
		machine->args[arg_count] = machine->sys_stack->car;
		machine->sys_stack = machine->sys_stack->cdr;
		--machine->sys_stack_size;

		++arg_count;
	}
}

// Implements the various system evalution functions using gotos so that
// we don't use the normal stack with normal function calls. We must use
// our own machine stack built from cons cells.

// The calling convention is as follows:
// - Calling function pushes the arguments needed for later onto the stack.
// - Calling function pushes the return function onto the stack
// - Calling function sets up arguments for next function
// - Jump to the next function
// - Once next function is complete, it pops the stack.
// - Called function returns according to the machine->calling_func register set by the previous pop
void execute() {

	machine->calling_func = SYS_REPL;
	push_system_args(0);

/*
	machine->args[0] = make_expression("				\
		((lambda (adder a b)							\
		   (adder a b))									\
		 (lambda (a b)									\
		   (if (eq? b null)								\
		       a 										\
		       (adder (cons (quote o) a) (cdr b))))		\
		 (quote (o))									\
		 (quote (o o o)))									\
	");
*/

	// machine->args[0] = make_expression("		\
	// 	((lambda (fact x result)				\
	// 	   (fact x result))						\
	// 	 (lambda (x result)						\
	// 	   (if (< x 2)							\
	// 	       result							\
	// 	       (fact (- x 1) (* result x))))	\
	// 	 10 1)									\
	// 	");
	// machine->args[0] = make_expression("((lambda (func)			\
	// 	                                   (func (eval (in))))	\
	// 	                                 (lambda (x) (func (eval (in)))))");
	machine->args[0] = make_expression("((lambda (x) (out x)) \">\")");
	machine->args[1] = make_expression("()");
	machine->args[2] = machine->nil;
	machine->args[3] = machine->nil;
	machine->result = machine->nil;

/***********************************************************
 ************************* Eval ****************************
 ***********************************************************/
sys_eval:
	if(machine->args[0]->is_atom) {
		if(machine->args[0]->type == SYS_GENERAL) {
			machine->args[0] = machine->args[0];
			machine->args[1] = machine->args[1];
			SYSCALL(sys_lookup);
		}
		else if(machine->args[0]->type == SYS_SYM_STRING) {
			machine->result = machine->args[0];
			goto sys_execute_return;
		}
		else {
			switch(machine->args[0]->type) {
				case SYS_SYM_NULL:
					machine->result = machine->nil;
					break;
				case SYS_SYM_FALSE:
					machine->result = machine->nil;
					break;
				case SYS_SYM_TRUE:
					machine->result = NULL;
					break;
				case SYS_SYM_NUM:
					machine->result = machine->args[0];
					break;
			}
			goto sys_execute_return;
		}
	}
	else {
		switch(machine->args[0]->car->type) {
			case SYS_SYM_IF:

				machine->args[3] = machine->args[1];
				machine->args[2] = machine->args[0]->cdr->cdr->cdr->car;
				machine->args[1] = machine->args[0]->cdr->cdr->car;
				machine->args[0] = machine->args[0]->cdr->car;

				SYSCALL(sys_evif);
			case SYS_SYM_LAMBDA:
				machine->result = machine->args[0];
				goto sys_execute_return;
			case SYS_SYM_QUOTE:
				machine->result = machine->args[0]->cdr->car;
				goto sys_execute_return;
			default:
				// Push args for later access
				machine->calling_func = SYS_EVAL;
				push_system_args(2);
				// Setup args for evlis
				machine->args[0] = machine->args[0]->cdr;
				machine->args[1] = machine->args[1];
				machine->args[2] = machine->nil;
				machine->args[3] = machine->nil;

				SYSCALL(sys_evlis);

				// SYS_EVAL
				sys_eval_evlis_continue:

				// Set up args for sys_apply
				machine->args[0] = machine->args[0]->car;
				machine->args[2] = machine->args[1];
				machine->args[1] = machine->result;
				machine->args[3] = machine->nil;

				SYSCALL(sys_apply);
		}
	}


/***********************************************************
 ************************* Apply ***************************
 ***********************************************************/

sys_apply:
	
	if(machine->args[0]->is_atom) {
		// Arithmetic operation with multiple args
		if(machine->args[0]->type >= SYS_SYM_MULT && machine->args[0]->type <= SYS_SYM_DIV) {

			machine->args[0] = machine->args[0];
			machine->args[1] = machine->args[1];
			machine->args[2] = get_free_cell();
			machine->args[3] = machine->nil;

			// Setup the starting arguments based on the operation
			switch(machine->args[0]->type) {
				case SYS_SYM_MULT:
					machine->args[2]->car = (Cell *)1;
					break;
				case SYS_SYM_ADD:
					machine->args[2]->car = (Cell *)0;
					break;
				case SYS_SYM_SUB:
					machine->args[2]->car = machine->args[1]->car->car;
					machine->args[1] = machine->args[1]->cdr;
					break;
				case SYS_SYM_DIV:
					machine->args[2]->car = machine->args[1]->car->car;
					machine->args[1] = machine->args[1]->cdr;
					break;
			}

			machine->args[2]->cdr = NULL;
			machine->args[2]->is_atom = true;
			machine->args[2]->type = SYS_SYM_NUM;

			SYSCALL(sys_evarth);
		}
		else {
			switch(machine->args[0]->type) {
				case SYS_SYM_CAR:
					machine->result = machine->args[1]->car->car;
					goto sys_execute_return;
				case SYS_SYM_CDR:
					machine->result = machine->args[1]->car->cdr;
					goto sys_execute_return;
				case SYS_SYM_CONS:
					machine->result = cons(machine->args[1]->car, machine->args[1]->cdr->car);
					goto sys_execute_return;
				case SYS_SYM_EQ:
					machine->result = eq(machine->args[1]->car, machine->args[1]->cdr->car);
					goto sys_execute_return;
				case SYS_SYM_ATOM:
					machine->result = atom(machine->args[1]->car);
					goto sys_execute_return;
				case SYS_SYM_QUIT:
					machine->result = make_expression("HALT");
					printf(" => Program requested the machine to quit execution. Quiting...\n");
					goto sys_execute_done;
				case SYS_SYM_LESS:
					if(machine->args[1]->car->car < machine->args[1]->cdr->car->car) {
						machine->result = NULL;
					}
					else {
						machine->result = machine->nil;
					}
					goto sys_execute_return;
				case SYS_SYM_EQUAL:
					if(machine->args[1]->car->car == machine->args[1]->cdr->car->car) {
						machine->result = NULL;
					}
					else {
						machine->result = machine->nil;
					}
					goto sys_execute_return;
				case SYS_SYM_GREAT:
					if(machine->args[1]->car->car > machine->args[1]->cdr->car->car) {
						machine->result = NULL;
					}
					else {
						machine->result = machine->nil;
					}
					goto sys_execute_return;
				case SYS_SYM_MOD:
					machine->args[0] = machine->args[0];
					machine->args[1] = machine->args[1];
					machine->args[2] = get_free_cell();
					machine->args[3] = machine->nil;

					machine->args[2]->car = machine->args[1]->car->car;
					machine->args[1] = machine->args[1]->cdr;
					
					machine->args[2]->cdr = NULL;
					machine->args[2]->is_atom = true;
					machine->args[2]->type = SYS_SYM_NUM;

					SYSCALL(sys_evarth);
				case SYS_SYM_AND:
					if(machine->args[1]->car == NULL && machine->args[1]->cdr->car == NULL) {
						machine->result = NULL;
					}
					else {
						machine->result = machine->nil;
					}
					goto sys_execute_return;
				case SYS_SYM_OR:
					if(machine->args[1]->car == NULL || machine->args[1]->cdr->car == NULL) {
						machine->result = NULL;
					}
					else {
						machine->result = machine->nil;
					}
					goto sys_execute_return;
				case SYS_SYM_NOT:
					if(machine->args[1]->car == NULL) {
						machine->result = machine->nil;
					}
					else {
						machine->result = NULL;
					}
					goto sys_execute_return;
				case SYS_SYM_JOIN:
					printf("JOIN");
					goto sys_execute_return;
				case SYS_SYM_SUBSTR:
					printf("SUBSTR");
					goto sys_execute_return;
				case SYS_SYM_CHARAT:
					machine->args[0] = machine->args[1]->car;
					machine->args[1] = machine->args[1]->cdr->car;
					machine->args[2] = make_num("0");
					machine->args[3] = machine->nil;

					SYSCALL(sys_charat);
				case SYS_SYM_IN:
					printf(" <= ");
					char * string = malloc(sizeof(char) * INPUT_BUFFER_LENGTH);
					fgets(string, INPUT_BUFFER_LENGTH, stdin);
					machine->result = make_expression(string);
					free(string);
					goto sys_execute_return;
				case SYS_SYM_OUT:
					printf(" => ");
					print_list(machine->args[1]->car);
					machine->result = machine->nil;
					goto sys_execute_return;
				case SYS_SYM_EVAL:
					machine->calling_func = SYS_APPLY_0;
					push_system_args(0);

					machine->args[0] = machine->args[1]->car;
					machine->args[1] = machine->args[2];
					machine->args[2] = machine->nil;
					machine->args[3] = machine->nil;

					SYSCALL(sys_eval);

					// SYS_APPLY_0
					sys_apply_eval_cont:

					printf(" > ");
					print_list(machine->result);
					printf("\n");
					goto sys_execute_return;
				default:
					machine->calling_func = SYS_APPLY_1;
					push_system_args(3);

					machine->args[0] = machine->args[0];
					machine->args[1] = machine->args[2];
					machine->args[2] = machine->nil;
					machine->args[3] = machine->nil;

					SYSCALL(sys_eval);

					// SYS_APPLY_1
					sys_apply_eval_continue:

					machine->args[0] = machine->result;
					machine->args[1] = machine->args[1];
					machine->args[2] = machine->args[2];
					machine->args[3] = machine->nil;

					SYSCALL(sys_apply);
			}
		}
	}
	else {
		machine->calling_func = SYS_APPLY_2;
		push_system_args(3);

		machine->args[0] = machine->args[0]->cdr->car;
		machine->args[1] = machine->args[1];
		machine->args[2] = machine->args[2];
		machine->args[3] = machine->nil;

		SYSCALL(sys_conenv);

		// SYS_APPLY_2
		sys_apply_conenv_continue:

		machine->args[0] = machine->args[0]->cdr->cdr->car;
		machine->args[1] = machine->result;
		machine->args[2] = machine->nil;
		machine->args[3] = machine->nil;

		SYSCALL(sys_eval);
	}

/***********************************************************
 ************************* Evlis ***************************
 ***********************************************************/

sys_evlis:

	if(machine->args[0] == machine->nil) {
		machine->result = machine->nil;
		goto sys_execute_return;
	}
	else {
		machine->calling_func = SYS_EVLIS_0;
		push_system_args(2);

		machine->args[0] = machine->args[0]->car;
		machine->args[1] = machine->args[1];
		machine->args[2] = machine->nil;
		machine->args[3] = machine->nil;

		SYSCALL(sys_eval);

		// SYS_EVLIS_0
		sys_evlis_eval_continue:

		// Save the result so push 3 args
		machine->calling_func = SYS_EVLIS_1;
		machine->args[2] = machine->result;
		push_system_args(3);

		machine->args[0] = machine->args[0]->cdr;
		machine->args[1] = machine->args[1];
		machine->args[2] = machine->nil;
		machine->args[3] = machine->nil;

		SYSCALL(sys_evlis);

		// SYS_EVLIS_1
		sys_evlis_evlis_continue:
		
		machine->result = cons(machine->args[2], machine->result);
		goto sys_execute_return;
	}

/***********************************************************
 ************************* Evif ****************************
 ***********************************************************/

sys_evif:
	
	machine->calling_func = SYS_EVIF;
	push_system_args(4);

	machine->args[0] = machine->args[0];
	machine->args[1] = machine->args[3];
	machine->args[2] = machine->nil;
	machine->args[3] = machine->nil;

	SYSCALL(sys_eval);

	// SYS_EVIF_0
	sys_evif_eval_continue:

	if(machine->result != machine->nil) {
		machine->args[0] = machine->args[1];
		machine->args[1] = machine->args[3];
		machine->args[2] = machine->nil;
		machine->args[3] = machine->nil;

		SYSCALL(sys_eval);
	}
	else {
		machine->args[0] = machine->args[2];
		machine->args[1] = machine->args[3];
		machine->args[2] = machine->nil;
		machine->args[3] = machine->nil;

		SYSCALL(sys_eval);
	}

/***********************************************************
 ************************* Evarth **************************
 ***********************************************************/

 sys_evarth:
 	if(machine->args[1] == machine->nil) {
 		machine->result = machine->args[2];
 		goto sys_execute_return;
 	}
 	else {
 		machine->args[0] = machine->args[0];
 		switch(machine->args[0]->type) {
 			case SYS_SYM_MULT:
 				machine->args[2]->car = (Cell *)((uintptr_t)machine->args[2]->car * (uintptr_t)machine->args[1]->car->car);
 				break;
 			case SYS_SYM_ADD:
 				machine->args[2]->car = (Cell *)((uintptr_t)machine->args[2]->car + (uintptr_t)machine->args[1]->car->car);
 				break;
 			case SYS_SYM_SUB:
 				machine->args[2]->car = (Cell *)((uintptr_t)machine->args[2]->car - (uintptr_t)machine->args[1]->car->car);
 				break;
 			case SYS_SYM_DIV:
 				machine->args[2]->car = (Cell *)((uintptr_t)machine->args[2]->car / (uintptr_t)machine->args[1]->car->car);
 				break;
 			case SYS_SYM_MOD:
 				machine->args[2]->car = (Cell *)((uintptr_t)machine->args[2]->car % (uintptr_t)machine->args[1]->car->car);
 				break;
 		}
 		machine->args[1] = machine->args[1]->cdr;

 		SYSCALL(sys_evarth);
 	}

/***********************************************************
 ************************* Conenv **************************
 ***********************************************************/

sys_conenv:

	if(machine->args[0] == machine->nil) {
		machine->result = machine->args[2];
		goto sys_execute_return;
	}
	else {
		machine->calling_func = SYS_CONENV;
		machine->args[3] = cons(machine->args[0]->car, machine->args[1]->car);
		push_system_args(4);

		machine->args[0] = machine->args[0]->cdr;
		machine->args[1] = machine->args[1]->cdr;
		machine->args[2] = machine->args[2];
		machine->args[3] = machine->nil;

		SYSCALL(sys_conenv);

		// SYS_CONENV_0
		sys_conenv_conenv_continue:

		machine->result = cons(machine->args[3], machine->result);
		goto sys_execute_return;
	}

/***********************************************************
 ************************* Lookup **************************
 ***********************************************************/

sys_lookup:

	if(machine->args[1] == machine->nil) {
		printf(" => Symbol not found: %s\n", get_symbol_name(machine->args[0]));

		machine->args[0] = make_expression("(quit)");
		machine->args[1] = machine->nil;
		machine->args[2] = machine->nil;
		machine->args[3] = machine->nil;

		SYSCALL(sys_eval);
	}
	else {
		// NULL is true in this machine
		if(eq(machine->args[1]->car->car, machine->args[0]) == NULL) {
			machine->result = machine->args[1]->car->cdr;
			goto sys_execute_return;
		}
		else {
			machine->args[0] = machine->args[0];
			machine->args[1] = machine->args[1]->cdr;
			machine->args[2] = machine->nil;
			machine->args[3] = machine->nil;

			SYSCALL(sys_lookup);
		}
	}

/***********************************************************
 ************************* Charat **************************
 ***********************************************************/

// Will deal with each byte at a type. Once we go to another cell,
// we will just subtract BYTES_PER_CAR from the target index and reset
// the index counter to zero. This way, we don't need division.
sys_charat:

	if(machine->args[1]->car == machine->args[2]->car) {
		machine->result = get_free_cell();
		// Extract the character at the requested location
		machine->result->car = (Cell *)(uintptr_t)(((int)(uintptr_t)machine->args[0]->car >> ((int)(uintptr_t)machine->args[1]->car)) & 0xFF);
		machine->result->type = SYS_SYM_CHAR;
		goto sys_execute_return;
	}
	else {
		if((int)(uintptr_t)machine->args[2]->car == chars_per_pointer - 1) {
			machine->args[0] = machine->args[0]->cdr;
			machine->args[1]->car = (Cell *)(uintptr_t)((int)(uintptr_t)machine->args[1]->car - chars_per_pointer);
			machine->args[2]->car = (Cell *)(uintptr_t)0;
		}
		else {
			machine->args[0] = machine->args[0];
			machine->args[1] = machine->args[1];
			machine->args[2]->car = (Cell *)(uintptr_t)((int)(uintptr_t)machine->args[2]->car + 1);
		}

		machine->args[3] = machine->nil;

		SYSCALL(sys_charat);
	}

/***********************************************************
 ************************* Return **************************
 ***********************************************************/

sys_execute_return:
	pop_system_args();
	switch(machine->calling_func) {
		case SYS_EVAL:
			goto sys_eval_evlis_continue;
		case SYS_APPLY_0:
			goto sys_apply_eval_cont;
		case SYS_APPLY_1:
			goto sys_apply_eval_continue;
		case SYS_APPLY_2:
			goto sys_apply_conenv_continue;
		case SYS_EVLIS_0:
			goto sys_evlis_eval_continue;
		case SYS_EVLIS_1:
			goto sys_evlis_evlis_continue;
		case SYS_EVIF:
			goto sys_evif_eval_continue;
		case SYS_CONENV:
			goto sys_conenv_conenv_continue;
		case SYS_REPL:
			goto sys_execute_done;
	}

sys_execute_done:
	return;
}

/*
 * Machine specific LISP functions
 */

Cell * car(Cell * cell) {
	return cell->car;
}

Cell * cdr(Cell * cell) {
	return cell->cdr;
}

Cell * cons(Cell * cell1, Cell * cell2) {

	Cell * new_cell = get_free_cell();

	new_cell->car = cell1;
	new_cell->cdr = cell2;

	return new_cell;
}

Cell * quote(Cell * cell) {
	return cell;
}

Cell * atom(Cell * cell) {

	if(cell->is_atom) {
		return NULL;
	}
	else {
		return machine->nil;
	}
}

// Really only implements an if statement, not cond
Cell * if_cond(Cell * pred, Cell * true_value, Cell * false_value) {

	if(pred == machine->nil) {
		return false_value;
	}
	else return true_value;
}

Cell * eq(Cell * cell1, Cell * cell2) {

	// Compare two nils
	if(cell1 == machine->nil && cell2 == machine->nil) {
		return NULL;
	}
	// Compare two symbols that aren't nil
	else if (cell1 != machine->nil && cell2 != machine->nil) {

		if(cell1->is_atom && cell2->is_atom) {

			while(true) {
				char * name1 = (char *)cell1;
				char * name2 = (char *)cell2;

				if(strcmp(name1, name2) == 0) {
					// Both beginnings are the same. But we have more cells to examine
					if(cdr(cell1) != machine->nil && cdr(cell2) != machine->nil) {
						cell1 = cell1->cdr;
						cell2 = cell2->cdr;
					}
					//	Both symbols are the same and don't have a cdr. They are eq
					else if(cdr(cell1) == machine->nil && cdr(cell2) == machine->nil) {
						return NULL;
					}
					else {
						return machine->nil;
					}
				}
				// The strings aren't the same
				else {
					return machine->nil;
				}
			}
		}
		else {
			// Somehow report that the given arguments are not atomic
		}
	}
	// We are comparing nil with a different symbol. They aren't eq
	else {
		return machine->nil;
	}

	// Should never reach this. Make compiler happy
	return NULL;
}

// Creates an array with all the names of the system functions.
// Expects a string containing all the function names
// each seperated by some whitespace. Must have no whitespace 
// surrounding the whole string
void init_instr_list(char * funcs) {

	int func_count = 1;
	bool parsing_white_space = false;
	for(int i = 0; i < strlen(funcs); ++i) {
		if(funcs[i] == ' ' || funcs[i] == '\t') {
			if(!parsing_white_space) {
				++func_count;
				parsing_white_space = true;
			}
		}
		else {
			parsing_white_space = false;
		}
	}

	char (*memory_block)[INSTR_MAX_LENGTH + 1] = malloc(sizeof(char) * (INSTR_MAX_LENGTH + 1) * func_count);
	char **instructions = malloc(sizeof(char *) * func_count);

	int func_index = 0;
	int string_index = 0;
	for(int i = 0; i < strlen(funcs); ++i) {
		if(funcs[i] == ' ' || funcs[i] == '\t') {
			if(!parsing_white_space) {
				++func_index;
				parsing_white_space = true;
			}

			string_index = 0;
		}
		else {
			parsing_white_space = false;
			memory_block[func_index][string_index] = funcs[i];
			++string_index;
		}
	}

	for(int i = 0; i < func_count; ++i) {
		instructions[i] = (char *)&memory_block[i];
	}

	machine->instr_memory_block = memory_block;
	machine->instructions = instructions;
	machine->num_of_instrs = func_count;
}
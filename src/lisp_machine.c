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
	init_instr_list("* + - / and atom? car cdr cons eq? false if lambda not null or quit quote true");

	// Initialize the machine system environment
	machine->sys_stack = machine->nil;
	machine->sys_stack_size = 0;

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


	//machine->args[0] = make_expression("((lambda (x) x)(if (atom? (quote (a))) (quote b) (quote c)))");
	// machine->args[0] = make_expression("				\
	// 	((lambda (adder a b)							\
	// 	   (adder a b))									\
	// 	 (lambda (a b)									\
	// 	   (if (eq? b null)								\
	// 	       a 										\
	// 	       (adder (cons (quote o) a) (cdr b))))		\
	// 	 (quote (o))									\
	// 	 (quote (o o o)))									\
	// ");
	machine->args[0] = make_expression("(+ 1 1)");
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
			machine->calling_func = SYS_EVAL_1;
			push_system_args(0);
			machine->args[0] = machine->args[0];
			machine->args[1] = machine->args[1];
			SYSCALL(sys_lookup);
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
		}
	}
	else {
		switch(machine->args[0]->car->type) {
			case SYS_SYM_IF:
				// Push calling function
				machine->calling_func = SYS_EVAL_1;
				push_system_args(0);
				// Setup arguments for next function
				machine->args[3] = machine->args[1];
				machine->args[2] = machine->args[0]->cdr->cdr->cdr->car;
				machine->args[1] = machine->args[0]->cdr->cdr->car;
				machine->args[0] = machine->args[0]->cdr->car;

				SYSCALL(sys_evif);
			case SYS_SYM_LAMBDA:
				machine->result = machine->args[0];
				break;
			case SYS_SYM_QUOTE:
				machine->result = machine->args[0]->cdr->car;
				break;
			default:
				// Push args for later access
				machine->calling_func = SYS_EVAL_0;
				push_system_args(2);
				// Setup args for evlis
				machine->args[0] = machine->args[0]->cdr;
				machine->args[1] = machine->args[1];
				machine->args[2] = machine->nil;
				machine->args[3] = machine->nil;

				SYSCALL(sys_evlis);
// SYS_EVAL_0
sys_eval_evlis_continue:
				machine->calling_func = SYS_EVAL_1;
				push_system_args(0);
				// Set up args for sys_apply
				machine->args[0] = machine->args[0]->car;
				machine->args[2] = machine->args[1];
				machine->args[1] = machine->result;
				machine->args[3] = machine->nil;

				SYSCALL(sys_apply);
		}
	}

	// Return from sys_eval
// SYS_EVAL_1
sys_eval_return:
	pop_system_args();
	switch(machine->calling_func) {
		case SYS_APPLY_0:
			goto sys_apply_eval_continue;
		case SYS_APPLY_2:
			goto sys_apply_return;
		case SYS_EVLIS_0:
			goto sys_evlis_eval_continue;
		case SYS_EVIF_0:
			goto sys_evif_eval_continue;
		case SYS_EVIF_1:
			goto sys_evif_return;
		case SYS_REPL:
			goto sys_execute_done;
	}

/***********************************************************
 ************************* Apply ***************************
 ***********************************************************/

sys_apply:
	
	if(machine->args[0]->is_atom) {
		switch(machine->args[0]->type) {
			case SYS_SYM_CAR:
				machine->result = machine->args[1]->car->car;
				break;
			case SYS_SYM_CDR:
				machine->result = machine->args[1]->car->cdr;
				break;
			case SYS_SYM_CONS:
				machine->result = cons(machine->args[1]->car, machine->args[1]->cdr->car);
				break;
			case SYS_SYM_EQ:
				machine->result = eq(machine->args[1]->car, machine->args[1]->cdr->car);
				break;
			case SYS_SYM_ATOM:
				machine->result = atom(machine->args[1]->car);
				break;
			case SYS_SYM_QUIT:
				machine->result = make_expression("HALT");
				printf("Program requested the machine to quit execution. Quiting...\n");
				return;
			case SYS_SYM_ADD:;
				Cell * result = get_free_cell();
				result->car = (Cell *)((uintptr_t)machine->args[0]->cdr->car + (uintptr_t)machine->args[0]->cdr->cdr->car);
				result->cdr = NULL;
				result->type = SYS_SYM_NUM;
				result->is_atom = true;
				machine->result = result;
				break;
			case SYS_SYM_SUB:
			case SYS_SYM_MULT:
			case SYS_SYM_DIV:
			case SYS_SYM_AND:
			case SYS_SYM_OR:
			case SYS_SYM_NOT:
			default:
				machine->calling_func = SYS_APPLY_0;
				push_system_args(3);

				machine->args[0] = machine->args[0];
				machine->args[1] = machine->args[2];
				machine->args[2] = machine->nil;
				machine->args[3] = machine->nil;

				SYSCALL(sys_eval);

// SYS_APPLY_0
sys_apply_eval_continue:
				machine->calling_func = SYS_APPLY_2;
				push_system_args(0);

				machine->args[0] = machine->result;
				machine->args[1] = machine->args[1];
				machine->args[2] = machine->args[2];
				machine->args[3] = machine->nil;

				SYSCALL(sys_apply);
		}
	}
	else {
		machine->calling_func = SYS_APPLY_1;
		push_system_args(3);
		machine->args[0] = machine->args[0]->cdr->car;
		machine->args[1] = machine->args[1];
		machine->args[2] = machine->args[2];
		machine->args[3] = machine->nil;

		SYSCALL(sys_conenv);

// SYS_APPLY_1
sys_apply_conenv_continue:
		machine->calling_func = SYS_APPLY_2;
		push_system_args(0);

		machine->args[0] = machine->args[0]->cdr->cdr->car;
		machine->args[1] = machine->result;
		machine->args[2] = machine->nil;
		machine->args[3] = machine->nil;

		SYSCALL(sys_eval);
	}


	// Return from sys_apply
// SYS_APPLY_2
sys_apply_return:
	pop_system_args();
	switch(machine->calling_func) {
		case SYS_EVAL_1:
			goto sys_eval_return;
		case SYS_APPLY_2:
			goto sys_apply_return;
	}

/***********************************************************
 ************************* Evlis ***************************
 ***********************************************************/

sys_evlis:

	if(machine->args[0] == machine->nil) {
		machine->result = machine->nil;
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
	}

	pop_system_args();
	switch(machine->calling_func) {
		case SYS_EVAL_0:
			goto sys_eval_evlis_continue;
		case SYS_EVLIS_1:
			goto sys_evlis_evlis_continue;
	}

/***********************************************************
 ************************* Evif ****************************
 ***********************************************************/

sys_evif:
	
	machine->calling_func = SYS_EVIF_0;
	push_system_args(4);

	machine->args[0] = machine->args[0];
	machine->args[1] = machine->args[3];
	machine->args[2] = machine->nil;
	machine->args[3] = machine->nil;

	SYSCALL(sys_eval);

// SYS_EVIF_0
sys_evif_eval_continue:

	machine->calling_func = SYS_EVIF_1;
	push_system_args(0);

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

// SYS_EVIF_1
sys_evif_return:
	
	pop_system_args();
	switch(machine->calling_func) {
		case SYS_EVAL_1:
			goto sys_eval_return;
	}

/***********************************************************
 ************************* Conenv **************************
 ***********************************************************/

sys_conenv:

	if(machine->args[0] == machine->nil) {
		machine->result = machine->args[2];
	}
	else {
		machine->calling_func = SYS_CONENV_0;
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
	}

	pop_system_args();
	switch(machine->calling_func) {
		case SYS_APPLY_1:
			goto sys_apply_conenv_continue;
		case SYS_CONENV_0:
			goto sys_conenv_conenv_continue;
	}

/***********************************************************
 ************************* Lookup **************************
 ***********************************************************/

sys_lookup:

	// NULL is true in this machine
	if(eq(machine->args[1]->car->car, machine->args[0]) == NULL) {
		machine->result = machine->args[1]->car->cdr;
	}
	else {
		machine->calling_func = SYS_LOOKUP_0;
		push_system_args(0);

		machine->args[0] = machine->args[0];
		machine->args[1] = machine->args[1]->cdr;
		machine->args[2] = machine->nil;
		machine->args[3] = machine->nil;

		SYSCALL(sys_lookup);
	}

// SYS_LOOKUP_0
sys_lookup_return:

	pop_system_args();
	switch(machine->calling_func) {
		case SYS_EVAL_1:
			goto sys_eval_return;
		case SYS_LOOKUP_0:
			goto sys_lookup_return;
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
					if(cdr(cell1) != NULL && cdr(cell2) != NULL) {
						cell1 = cell1->cdr;
						cell2 = cell2->cdr;
					}
					//	Both symbols are the same and don't have a cdr. They are eq
					else if(cdr(cell1) == NULL && cdr(cell2) == NULL) {
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
#include "lisp_machine.h"
#include "repl.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int chars_per_pointer = sizeof(uintptr_t) / sizeof(char);
bool verbose_flag;
Lisp_Machine * machine;

Lisp_Machine * init_machine() {

	Lisp_Machine * machine = malloc(sizeof(Lisp_Machine));
	machine->is_running = true;

	if(verbose_flag) {
		printf("Initializing machine...\n");
	}

	// Create and link the memory cells
	machine->mem = calloc(sizeof(Cell) * NUM_OF_CELLS, sizeof(Cell));
	for(int i = 0; i < NUM_OF_CELLS - 1; ++i) {
		machine->mem[i].cdr = &machine->mem[i + 1];
	}

	machine->prog = machine->mem;
	machine->free_mem = machine->mem->cdr;
	machine->prog->cdr = NULL;

	machine->nil = malloc(sizeof(Cell));
	machine->nil->car = NULL;
	machine->nil->cdr = NULL;
	machine->nil->is_atom = true;

	if(verbose_flag) {
		printf("Machine initialized!\n");
	}

	return machine;
}

void destroy_machine(Lisp_Machine *machine) {

	free(machine->mem);
	free(machine);
}

Cell * get_free_cell() {

	Cell * new_cell = machine->free_mem;
	machine->free_mem = cdr(machine->free_mem);
	new_cell->cdr = NULL;

	return new_cell;
}

void store_cell(Cell * cell) {
	cell->car = machine->free_mem;
	machine->free_mem = cell;
}

void execute(Lisp_Machine * lm) {

	Cell * cell = lm->prog;

	while(machine->is_running) {
		switch(cell->type) {
			case SYS_CAR:
			case SYS_CDR:
			case SYS_CONS:
			case SYS_COND:
			case SYS_ATOM:
			case SYS_EQ:
			case SYS_QUOTE:
			case SYS_HALT:
			default:
				break;
		}
	}
}

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
		return machine->mem;
	}
	else {
		return machine->nil;
	}
}

// Really only implements an if statement, not cond
Cell * cond(Cell * pred, Cell * true_value, Cell * false_value) {

	if(pred == machine->nil) {
		return false_value;
	}
	else return true_value;
}

Cell * eq(Cell * cell1, Cell * cell2) {

	if(cell1 == machine->nil && cell2 == machine->nil) {
		return machine->mem;
	}
	else if (cell1 != machine->nil && cell2 != machine->nil) {

		if(cell1->is_atom && cell2->is_atom) {

			while(true) {
				uintptr_t copy1 = (uintptr_t)cell1;
				uintptr_t copy2 = (uintptr_t)cell2;

				char name1[chars_per_pointer];
				char name2[chars_per_pointer];

				for(int i = chars_per_pointer - 1; i >= 0; --i) {
					name1[i] = (char)(copy1);
					name2[i] = (char)(copy2);

					copy1 = copy1 >> 1;
					copy2 = copy2 >> 2;
				}

				if(strcmp(name1, name2) == 0) {
					if(cdr(cell1) != NULL && cdr(cell2) != NULL) {
						cell1 = cdr(cell1);
						cell2 = cdr(cell2);
					}
					else if(cdr(cell1) == NULL && cdr(cell2) == NULL) {
						return machine->mem;
					}
					else {
						return machine->nil;
					}
				}
				else {
					return machine->nil;
				}
			}
		}
		else {
			// Somehow report that the given arguments are not atomic
		}
	}
	else {
		return machine->nil;
	}

	// Should never reach this. Make compiler happy
	return NULL;
}
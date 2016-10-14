#include "lisp_machine.h"
#include <memory.h>
#include <stdlib.h>

#define REPL_BUFFER_SIZE 1024

LispMachine machine;

void initMachine() {
	machine.store = NULL;
	machine.trash = NULL;
	machine.topExpr = NULL;
	machine.running = 1;

	// Init the machine memory
	machine.store = malloc(sizeof(Cell) * HEAP_SIZE_IN_CELLS);
	// Link all memory cells consecutively together to begin
	for(int i = 0; i < HEAP_SIZE_IN_CELLS; ++i) {
		setCell(machine.store + i, NIL, NULL, machine.store + i + 1);
	}
}

void executeMachine() {

}

Cell * cons(Cell *cell1, Cell *cell2) {
	// Get the next availble cell from the store
	Cell *cell = machine.store;
	machine.store = machine.store->cdr; // Update store

	cell->car = cell1;
	cell->cdr = cell2;

	return cell;
}
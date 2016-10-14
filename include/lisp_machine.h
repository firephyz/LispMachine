#ifndef LISP_MACHINE_INCLUDED
	#define LISP_MACHINE_INCLUDED

	#define HEAP_SIZE_IN_CELLS 65536

	#define NUMBER 		0
	#define SYMBOL 		1
	#define LIST		2
	#define FUNCTION	3
	#define NIL			4

	typedef struct lisp_machine_struct LispMachine;
	typedef struct cell_struct Cell;

	Cell * cons(Cell *cell1, Cell *cell2);
	void initMachine();

	#include "repl.h"

	struct lisp_machine_struct {
		Cell *store;
		Cell *trash;
		Cell *topExpr;

		int running;
		char replBuffer[REPL_BUFFER_SIZE];
	};

	// Struct for a cons cell
	struct cell_struct {
		Cell *car;
		Cell *cdr;

		/*
		 *  Holds various data about the given cell.
		 *  Bit 0 for marks the cell
		 *  for garbage collection.
		 *  Bits 1-3 define the type of data in the car.
		 */
		int8_t metaData;
	};

#endif
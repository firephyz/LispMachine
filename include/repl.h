#ifndef REPL_INCLUDED
	#define REPL_INCLUDED

	#define REPL_BUFFER_SIZE 1024

	#include "lisp_machine.h"
	#include <stdint.h>

	void setCell(Cell *target, int8_t cellType, Cell *car, Cell *cdr);
	void storeToken(char *string);
	void setTrashBit(Cell *cell, int8_t value);
	uint8_t getTrashBit(Cell *cell);
	void setCellType(Cell *cell, int8_t type);
	int8_t getCellType(Cell *cell);

#endif
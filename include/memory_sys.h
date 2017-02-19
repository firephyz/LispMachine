#ifndef MEMORY_SYS_INCLUDED
	#define MEMORY_SYS_INCLUDED

	#include "lisp_machine.h"
	#include <stdint.h>
	#include <stdbool.h>

	extern Lisp_Machine * machine;

	Cell * getCar(Cell * cell);
	void setCar(Cell *cell, Cell * value);
	Cell * getCdr(Cell * cell);
	void setCdr(Cell *cell, Cell * value);
	bool getIsAtom(Cell * cell);
	void setIsAtom(Cell * cell, bool value);
	uint8_t getType(Cell * cell);
	void setType(uint8_t value);

	void putCellAway(Cell * cell); // Marks the cell as not fetched

	void manageMetaData(Cell * cell);

#endif
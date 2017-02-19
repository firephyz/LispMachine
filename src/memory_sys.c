#include "memory_sys.h"
#include "lisp_machine.h"

Cell * getCar(Cell * cell) {
	manageMetaData(cell);
	return cell->car;
}

void setCar(Cell *cell, Cell * value) {
	manageMetaData(cell);
	cell->car = value;
}

Cell * getCdr(Cell * cell) {
	manageMetaData(cell);
	return cell->cdr;
}

void setCdr(Cell *cell, Cell * value) {
	manageMetaData(cell);
	cell->cdr = value;
}

bool getIsAtom(Cell * cell) {
	manageMetaData(cell);
	return cell->is_atom;
}

void setIsAtom(Cell * cell, bool value) {
	manageMetaData(cell);
	cell->is_atom = value;
}

uint8_t getType(Cell * cell) {
	manageMetaData(cell);
	return cell->type;
}

void putCellAway(Cell * cell) {
	cell->is_fetched = false;
}

void manageMetaData(Cell * cell) {
	if(!cell->is_fetched) {
		cell->is_fetched = true;
		machine->memory_access_count += 1;
	}
}
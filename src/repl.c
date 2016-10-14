#include "lisp_machine.h"
#include "expr_parser.h"
#include <stdio.h>

LispMachine machine;

int main() {

	// Set up memory and the like
	initMachine();

	// _Present the repl
	while(machine.running) {
		printf(">>> ");
		fgets(machine.replBuffer, REPL_BUFFER_SIZE, stdin);

		parseExpr(machine.replBuffer);
	}
}

void storeToken(char *string) {

}

void setCell(Cell *target, int8_t cellType, Cell *car, Cell *cdr) {
	setCellType(target, cellType);
	target->car = car;
	target->cdr = cdr;
}

/*
 *  Functions for manipulating the Cells and their data.
 */
void setTrashBit(Cell *cell, int8_t value) {
	cell->metaData = cell->metaData | value;
}
uint8_t getTrashBit(Cell *cell) {
	return cell->metaData & 0x01;
}
void setCellType(Cell *cell, int8_t type) {
	cell->metaData = cell->metaData | (type << 1);
}
int8_t getCellType(Cell *cell) {
	return (cell->metaData >> 1) & 0x07;
}
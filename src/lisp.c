#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <memory.h>

#define REPL_BUFFER_SIZE 1024
#define HEAP_SIZE_IN_CELLS 65536

#define NUMBER  0
#define SYMBOL  1
#define LIST	2
#define OPCODE	3
#define NIL		4

typedef struct cell_struct Cell;
typedef struct cell_list_data ListData;

void parseExpr(char *string);
void storeToken(char *string);
int nextToken(int startIndex);
int endOfToken(int startingIndex);
int findMatchingParens(int firstIndex);
Cell * cons(Cell *cell1, Cell *cell2);
void setTrashBit(Cell *cell, int8_t value);
uint8_t getTrash(Cell *cell);
void setCellType(Cell *cell, int8_t type);
int8_t getCellType(Cell *cell);

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

Cell *heap;
Cell *store;
Cell *trash;
Cell *topExpr;

int running;
char replBuffer[REPL_BUFFER_SIZE];

int main() {

	running = 1;

	// Init the cell heap
	heap = malloc(sizeof(Cell) * HEAP_SIZE_IN_CELLS);
	for(int i = 0; i < HEAP_SIZE_IN_CELLS; ++i) {
		Cell cell = heap[i];
		setCellType(&cell, NIL);
		cell.car = NULL;
		cell.cdr = &cell + 1;
	}
	trash = NULL;
	store = heap;
	topExpr = NULL;

	// _Present the repl
	while(running) {
		printf(">>> ");
		fgets(replBuffer, REPL_BUFFER_SIZE, stdin);

		parseExpr(replBuffer);
	}
}

void parseExpr(char *string) {

	int index = 0;

	if(string[index] == '(') {

	}
	// This occurs if we don't have a list
	// Thus we must have a number, symbol or opcode
	else {
		int endIndex = endOfToken(index);
		int length = endIndex - index;
		char subString[length + 1];
		memcpy(subString, string + index, length);
		subString[length - 1] = '\000';

		// Store this token into the main s-expression
		storeToken(subString);
	}
}

void storeToken(char *string) {

}

// GIven a starting index, find the next closest token
int nextToken(int startIndex) {

	int index = startIndex;
	char currentChar = replBuffer[index];

	// Skip past the current token
	while(currentChar != ' ') {
		++index;
		currentChar = replBuffer[index];
	}

	// Skip past the white space
	while(currentChar == ' ') {
		++index;
		currentChar = replBuffer[index];
	}

	return index;
}

// Given an index inside a token, find the index of the following space
int endOfToken(int startingIndex) {

	int index = startingIndex;
	char currentChar = replBuffer[index];

	while(currentChar != ' ') {
		++index;
		currentChar = replBuffer[index];

		if(currentChar == '\n') return index;
	}

	return index;
}

int findMatchingParens(int firstIndex) {
	int index = firstIndex + 1;
	int count = 1;

	while(count != 0) {
		if (replBuffer[index] == '(') {
			++count;
		}
		else if (replBuffer[index] == ')') {
			-- count;
		}

		++index;

		if(index >= REPL_BUFFER_SIZE) {
			return -1;
		}
	}

	return index;
}

/*
 *  Lisp machine opcodes
 */
Cell * cons(Cell *cell1, Cell *cell2) {
	Cell *cell = store;
	store = store->cdr;

	cell->car = cell1;
	cell->cdr = cell2;

	return cell;
}

/*
 *  Functions for manipulating the Cells and their data.
 */
void setTrashBit(Cell *cell, int8_t value) {
	cell->metaData = cell->metaData | value;
}
uint8_t getTrash(Cell *cell) {
	return cell->metaData & 0x01;
}
void setCellType(Cell *cell, int8_t type) {
	cell->metaData = cell->metaData | (type << 1);
}
int8_t getCellType(Cell *cell) {
	return (cell->metaData >> 1) & 0x07;
}
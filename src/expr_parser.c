#include "expr_parser.h"
#include "lisp_machine.h"
#include <memory.h>

extern LispMachine machine;

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

// GIven a starting index, find the next closest token
int nextToken(int startIndex) {

	int index = startIndex;
	char currentChar = machine.replBuffer[index];

	// Skip past the current token
	while(currentChar != ' ') {
		++index;
		currentChar = machine.replBuffer[index];
	}

	// Skip past the white space
	while(currentChar == ' ') {
		++index;
		currentChar = machine.replBuffer[index];
	}

	return index;
}

// Given an index inside a token, find the index of the following space
int endOfToken(int startingIndex) {

	int index = startingIndex;
	char currentChar = machine.replBuffer[index];

	while(currentChar != ' ') {
		++index;
		currentChar = machine.replBuffer[index];

		if(currentChar == '\n') return index;
	}

	return index;
}

int findMatchingParens(int firstIndex) {
	int index = firstIndex + 1;
	int count = 1;

	while(count != 0) {
		if (machine.replBuffer[index] == '(') {
			++count;
		}
		else if (machine.replBuffer[index] == ')') {
			-- count;
		}

		++index;

		if(index >= REPL_BUFFER_SIZE) {
			return -1;
		}
	}

	return index;
}
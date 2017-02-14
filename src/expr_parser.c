#include "expr_parser.h"
#include "lisp_machine.h"
#include "repl.h"
#include "stack.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

Lisp_Machine * machine;

// @expr_type - Used to tell the make_symbol function what expression we are evaluating (eval, apply, evlis, etc...)
//					This is important when determining symbol types for system function arguments.
Cell * make_expression(char *expr) {

	Stack s;
	MAKE_STACK(s, Cell *);

	Cell root = {NULL, '\0', NULL, false, 0};
	Cell * cell = &root;

	Tokenizer * tk = make_tokenizer(expr);
	char * token = tokenizer_next(tk);
	while(token != NULL) {
		switch(token[0]) {
			case '(':
				// Handles the NIL symbol. (Annoyingly reuses almost all the code in the default case. Must fix)
				if(token[1] == ')') {
					if(cell->car == NULL) {
						cell->car = machine->nil;
						token = tokenizer_next(tk);
					}
					else {
						POP(s, Cell *, cell);
						cell->cdr = get_free_cell();
						PUSH(s, Cell *, cell->cdr);
						cell = cell->cdr;
					}
				}
				// Makes a new sub list
				else if(cell->car == NULL) {
					cell->car = get_free_cell();
					PUSH(s, Cell *, cell->car);
					cell = cell->car;
					token = tokenizer_next(tk);
				}
				// Moves along the right of a list to place the sub list
				else {
					POP(s, Cell *, cell);
					cell->cdr = get_free_cell();
					PUSH(s, Cell *, cell->cdr);
					cell = cell->cdr;
				}
				break;
			case ')':
				POP(s, Cell *, cell);
				cell->cdr = machine->nil;
				token = tokenizer_next(tk);
				break;
			default:
				if(cell->car == NULL) {
					cell->car = make_symbol(token);
					token = tokenizer_next(tk);
				}
				else {
					POP(s, Cell *, cell);
					cell->cdr = get_free_cell();
					PUSH(s, Cell *, cell->cdr);
					cell = cell->cdr;
				}
				break;
		}
	}

	// The stack is not empty so we encountered too few parenthesis
	if(s.n != 0) {
		return NULL;
	}

	destroy_tokenizer(tk);

	return root.car;
}

Tokenizer * make_tokenizer(char * string) {
	Tokenizer * tk = malloc(sizeof(Tokenizer));
	tk->string = string;
	tk->string_length = strlen(string);
	tk->index = 0;
	tk->max_token_size = 32;
	tk->token = malloc(sizeof(char) * tk->max_token_size);
	return tk;
}

void destroy_tokenizer(Tokenizer * tk) {
	free(tk->token);
	free(tk);
}

char * tokenizer_next(Tokenizer * tk) {

	if(tk->index >= tk->string_length) {
		return NULL;
	}

	switch(tk->string[tk->index]) {
		case '(':
			if(tk->string[tk->index + 1] == ')') {
				tk->index += 2;
				tk->token[0] = '(';
				tk->token[1] = ')';
				tk->token[2] = '\0';
			}
			else {
				++tk->index;
				tk->token[0] = '(';
				tk->token[1] = '\0';
			}
			return tk->token;
		case ')':
			++tk->index;
			tk->token[0] = ')';
			tk->token[1] = '\0';
			return tk->token;
		case ' ':
			++tk->index;
			return tokenizer_next(tk);
		case '\t':
			++tk->index;
			return tokenizer_next(tk);
		default:;
			int symbol_length = index_of(tk->string + tk->index, "\t ()");

			if(symbol_length + 1 > tk->max_token_size) {
				tk->token = realloc(tk->token, tk->max_token_size * 2);
				tk->max_token_size *= 2;
			}

			memcpy(tk->token, tk->string + tk->index, symbol_length);
			tk->token[symbol_length] = '\0';

			tk->index += symbol_length;
			return tk->token;
	}
}

// Finds the index of the closest char in @targets
int index_of(char * string, char * targets) {

	int target_length = strlen(targets);

	for(int i = 0; i < strlen(string); ++i) {
		for(int j = 0; j < target_length; ++j) {
			if(string[i] == targets[j]) {
				return i;
			}
		}
	}

	return -1;
}

// See make_expression function to understand the purpose of @expr_type
Cell * make_symbol(char * name) {

	uint8_t cell_type = determine_symbol_type(name);

	Cell * result;
	Cell * prev_cell;
	int num_of_cells = (strlen(name) + chars_per_pointer - 1) / chars_per_pointer;

	// Iterate through the chain of cells we will use to store the name
	for(int cell_index = 0; cell_index < num_of_cells; ++cell_index) {

		Cell * new_cell = get_free_cell();

		// Do required linking between the cells
		if(cell_index == 0) {
			result = new_cell;
			prev_cell = result;
		}
		else {
			prev_cell->cdr = new_cell;
			prev_cell = new_cell;
		}

		// Split up and copy the given string into the cells
		for(int i = chars_per_pointer - 1; i >= 0; --i) {

			int index = (cell_index * chars_per_pointer) + i;

			if(index >= strlen(name)) {
				new_cell->car = (Cell *)((uintptr_t)car(new_cell) << 8);
				continue;
			}
			else {
				new_cell->car = (Cell *)(((uintptr_t)car(new_cell) << 8) | (uintptr_t)name[index]);
			}
		}
	}

	result->is_atom = true;
	result->type = cell_type;

	return result;
}

// Performs a binary search on the strings in machine->instructions to locate
// a machine instruction.
uint8_t determine_symbol_type(char * name) {

	uint8_t result = SYS_GENERAL;

	int low_index = 0;
	int high_index = machine->num_of_instrs - 1;
	int mid_index = (high_index - low_index) / 2;

	int word_index = 0;

	while(1) {
		char * func_name = machine->instructions[mid_index];

		while(1) {
			char character = func_name[word_index];
			if(name[word_index] > character) {
				// If this condition is true, then the requested symbol
				// isn't present in the list. It must be a general variable.
				if(mid_index + 1 == low_index) {
					return SYS_GENERAL;
				}

				low_index = mid_index + 1;
				mid_index = (high_index + low_index) / 2;
				word_index = 0;
				break;
			}
			else if (name[word_index] < character) {
				// If this condition is true, then the requested symbol
				// isn't present in the list. It must be a general variable.
				if(mid_index - 1 == high_index) {
					return SYS_GENERAL;
				}

				high_index = mid_index - 1;
				mid_index = (high_index + low_index) / 2;
				word_index = 0;
				break;
			}
			else {
				++word_index;

				if(word_index == strlen(func_name)) {
					// The given name is longer than the potential match, we don't have a match.
					if(strlen(name) > word_index) {
						// Mark this is a generic symbol to be looked up in the environment
						return SYS_GENERAL;
					}
					// We have matched the given name to a system function
					// We add 1 because SYS_GENERAL takes up 0; we must start at 1 and go from there
					else {
						return mid_index + 1;
					}
				}
			}
		}
	}

	return result;
}

char * get_symbol_name(Cell * sym) {

	// If the symbol is NIL, just return that to be printed
	if(sym == machine->nil) {
		char * string = malloc(sizeof(char) * 3);
		string[0] = '(';
		string[1] = ')';
		string[2] = '\0';
		return string;
	}

	// Get the number of cells this name takes up
	int cell_count = 1;
	Cell * temp = sym;
	while(temp->cdr != NULL) {
		++cell_count;
		temp = temp->cdr;
	}

	// Allocate enough storage for the entire symbol name
	int string_length = chars_per_pointer * cell_count;
	char * string = malloc(sizeof(char) * string_length + 1);
	int cell_index = 0;

	// Copy symbol name into the result string
	while(sym != NULL) {
		memcpy(string + (cell_index * chars_per_pointer), &sym->car, chars_per_pointer);
		sym = sym->cdr;
		++cell_index;
	}

	string[string_length] = '\0';

	return string;
}
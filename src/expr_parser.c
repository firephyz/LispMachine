#include "expr_parser.h"
#include "lisp_machine.h"
#include "repl.h"
#include "stack.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

void determine_cell_type(char * name, int index, int * cell_type);

Cell * make_expression(char *expr) {

	if(expr[0] != '(') return NULL;
	// Handles the case when only the empty list is given
	else if (expr[0] == '(' && expr[1] == ')') {
		return machine->nil;
	}
	else {
		Stack s;
		MAKE_STACK(s, Cell *);

		Cell * root = get_free_cell();
		Cell * cell = root;
		PUSH(s, Cell *, cell);

		Tokenizer * tk = make_tokenizer(expr + 1);
		char * token = tokenizer_next(tk);
		while(token != NULL) {
			switch(token[0]) {
				case '(':
					// Handles the NIL symbol. (Annoying reuses almost all the code in the default case. Must fix)
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

		return root;
	}
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
		default:;
			int symbol_length = index_of(tk->string + tk->index, " ()");

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

Cell * make_symbol(char * name) {

	// Init the cell type. This will be refined to it's final value as we process the string
	int cell_type = SYS_CAR | SYS_CDR | SYS_CONS | SYS_EQ | SYS_COND | SYS_ATOM | SYS_QUOTE;

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

			// Additionally, classify the given symbol if it is a keyword (cons, eq, car...)
			if(num_of_cells == 1) {
				determine_cell_type(name, index, &cell_type);
			}

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

// Determines if the given symbol is a reserved keyword
void determine_cell_type(char * name, int index, int * cell_type) {
	switch(index) {
		case 0:
			if(name[index] != 'c') {
				*cell_type = *cell_type & (-1 ^ SYS_CONS);
				*cell_type = *cell_type & (-1 ^ SYS_COND);
				*cell_type = *cell_type & (-1 ^ SYS_CDR);
				*cell_type = *cell_type & (-1 ^ SYS_CAR);
			}
			if(name[index] != 'a') {
				*cell_type = *cell_type & (-1 ^ SYS_ATOM);
			}
			if(name[index] != 'q') {
				*cell_type = *cell_type & (-1 ^ SYS_QUOTE);
			}
			if(name[index] != 'e') {
				*cell_type = *cell_type & (-1 ^ SYS_EQ);
			}
			break;
		case 1:
			if(name[index] != 'o') {
				*cell_type = *cell_type & (-1 ^ SYS_CONS);
				*cell_type = *cell_type & (-1 ^ SYS_COND);
			}
			if(name[index] != 't') {
				*cell_type = *cell_type & (-1 ^ SYS_ATOM);
			}
			if(name[index] != 'a') {
				*cell_type = *cell_type & (-1 ^ SYS_CAR);
			}
			if(name[index] != 'd') {
				*cell_type = *cell_type & (-1 ^ SYS_CDR);
			}
			if(name[index] != 'u') {
				*cell_type = *cell_type & (-1 ^ SYS_QUOTE);
			}
			if(name[index] != 'q') {
				*cell_type = *cell_type & (-1 ^ SYS_EQ);
			}
			break;
		case 2:
			if(name[index] != 'n') {
				*cell_type = *cell_type & (-1 ^ SYS_CONS);
				*cell_type = *cell_type & (-1 ^ SYS_COND);
			}
			if(name[index] != 'o') {
				*cell_type = *cell_type & (-1 ^ SYS_ATOM);
				*cell_type = *cell_type & (-1 ^ SYS_QUOTE);
			}
			if(name[index] != 'r') {
				*cell_type = *cell_type & (-1 ^ SYS_CAR);
				*cell_type = *cell_type & (-1 ^ SYS_CDR);
			}
			break;
		case 3:
			if(name[index] != 's') {
				*cell_type = *cell_type & (-1 ^ SYS_CONS);
			}
			if(name[index] != 'm') {
				*cell_type = *cell_type & (-1 ^ SYS_ATOM);
			}
			if(name[index] != 'd') {
				*cell_type = *cell_type & (-1 ^ SYS_COND);
			}
			if(name[index] != 't') {
				*cell_type = *cell_type & (-1 ^ SYS_QUOTE);
			}
			break;
		case 4:
			if(name[index] != 'e') {
				*cell_type = *cell_type & (-1 ^ SYS_QUOTE);
			}
			break;
	}
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
#ifndef EXPR_PARSER_INCLUDED
	#define EXPR_PARSER_INCLUDED

	#include "lisp_machine.h"
	#include <stdint.h>

	extern Lisp_Machine * machine;

	typedef struct tokenizer_t {
		char * string;
		int string_length;
		int index;
		int max_token_size;
		char * token;
	} Tokenizer;

	Cell * make_expression(char *expr);
	int index_of(char * string, char * targets);
	Cell * make_symbol(char * name);
	Cell * make_num(char * digits);
	Cell * make_string(char *string);
	Cell * pack_cell_string(char * string);
	char * get_symbol_name(Cell * sym);
	uint8_t determine_symbol_type(char * name);

	Tokenizer * make_tokenizer(char * string);
	void destroy_tokenizer(Tokenizer * tk);
	char * tokenizer_next(Tokenizer * tk);

#endif
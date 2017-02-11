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

	Cell * make_expression(char *expr, uint8_t expr_type);
	int index_of(char * string, char * targets);
	Cell * make_symbol(char * name, uint8_t expr_type);
	char * get_symbol_name(Cell * sym);
	uint8_t determine_cell_type(char *name, uint8_t expr_type);
	uint8_t determine_symbol_type(char * name);
	uint8_t determine_eval_func(char * name);
	uint8_t determine_eval_arg(char * name, uint8_t expr_type);

	Tokenizer * make_tokenizer(char * string);
	void destroy_tokenizer(Tokenizer * tk);
	char * tokenizer_next(Tokenizer * tk);

#endif
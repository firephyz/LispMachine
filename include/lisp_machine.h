#ifndef LISP_MACHINE_INCLUDED
	#define LISP_MACHINE_INCLUDED

	#include <stdint.h>
	#include <stdbool.h>

	#define NUM_OF_CELLS 65536

	#define SYS_NAN 0
	#define SYS_CAR	1 << 0
	#define SYS_CDR	1 << 1
	#define SYS_CONS 1 << 2
	#define SYS_EQ 1 << 3
	#define SYS_ATOM 1 << 4
	#define SYS_QUOTE 1 << 5
	#define SYS_COND 1 << 6
	#define SYS_HALT 1 << 7

	extern int chars_per_pointer;

	typedef struct cell_t Cell;
	typedef struct lisp_machine_t Lisp_Machine;

	struct cell_t {
		Cell *car;
		char null_byte1; // Used to provide a null terminator if the car is interpreted as a string
		Cell *cdr;
		bool is_atom;
		int type;
	};

	struct lisp_machine_t {
		bool is_running;
		Cell * mem;
		Cell * free_mem;
		Cell * prog;
		Cell * nil;
	};

	Lisp_Machine * init_machine();
	void destroy_machine(Lisp_Machine *machine);
	Cell * get_free_cell();
	void store_cell(Cell * cell);
	void execute(Lisp_Machine * lm);

	Cell * car(Cell * cell);
	Cell * cdr(Cell * cell);
	Cell * cons(Cell * cell1, Cell * cell2);
	Cell * quote(Cell * cell);
	Cell * atom(Cell * cell);
	Cell * eq(Cell * cell1, Cell * cell2);
	Cell * cond(Cell * pred, Cell * true_value, Cell * false_value);

#endif
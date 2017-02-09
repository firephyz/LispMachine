#ifndef LISP_MACHINE_INCLUDED
	#define LISP_MACHINE_INCLUDED

	#include <stdint.h>
	#include <stdbool.h>

	#define NUM_OF_CELLS 65536

	#define SYS_NAN 	0x00

	#define SYS_CAR		0x01
	#define SYS_CDR		0x02
	#define SYS_CONS 	0x04
	#define SYS_EQ 		0x08
	#define SYS_ATOM 	0x10
	#define SYS_QUOTE 	0x20
	#define SYS_COND 	0x40
	#define SYS_LAMBDA 	0x80

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
		int mem_used;
		int mem_free;
		Cell * free_mem;
		Cell * nil;
		Cell * eval_func;
		Cell * apply_func;
		Cell * evlis_func;
		Cell * evif_func;
		Cell * conenv_func;
		Cell * lookup_func;

		Cell * sys_env;
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
	Cell * if_cond(Cell * pred, Cell * true_value, Cell * false_value);

#endif
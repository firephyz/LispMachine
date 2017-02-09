#ifndef STACK_INCLUDED
	#define STACK_INCLUDED

	#include <stdlib.h>

	typedef struct stack_t {
		void *data;
		int n; // number of elements
		int cap; // capacity
	} Stack;

	#define STARTING_SIZE 8

	// Initialize a given stack
	#define MAKE_STACK(stack, type) 								\
	do {															\
		stack.data = malloc(sizeof(type) * STARTING_SIZE); 			\
		stack.n = 0; 												\
		stack.cap = STARTING_SIZE;									\
	} while(0)

	#define RESIZE(stack, type)										\
	do {															\
		void * new_data = malloc(2 * stack.cap);					\
		memcpy(new_data, stack.data, sizeof(type) * stack.n);		\
		free(stack.data);											\
		stack.data = new_data;										\
		stack.cap = 2 * stack.cap;									\
	} while(0)

	// Push a given variable of a given type onto the stack
	#define PUSH(stack, type, var_name)														\
	do {																					\
		if(stack.n == stack.cap) {														\
			RESIZE(stack, type);															\
		}																					\
																							\
		memcpy(stack.data + sizeof(type) * stack.n, &var_name, sizeof(type));				\
																							\
		++stack.n;																			\
																							\
	} while(0)

	// Pop the given stack and store in var_name
	#define POP(stack, type, var_name)											\
	do {																		\
		if(stack.n != 0) {														\
			var_name = *((type *)(stack.data + sizeof(type) * (stack.n - 1)));	\
			--stack.n;															\
		}																		\
	} while(0)

	// Store the top of the stack into var_name
	#define PEEK(stack, type, var_name)											\
	do {																		\
		if(stack.n != 0) {														\
			var_name = *((type *)(stack.data + sizeof(type) * (stack.n - 1)));	\
		}																		\
	} while(0)

	void DESTROY_STACK(Stack *stack);

#endif
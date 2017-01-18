#include "stack.h"
#include "lisp_machine.h"
#include <stdlib.h>

void DESTROY_STACK(Stack *stack) {
	free(stack->data);
}
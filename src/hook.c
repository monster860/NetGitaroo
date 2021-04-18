#include <stdlib.h>

// Note that this does not support either of the first two instructions being a jump with the exception of
// the very first one being a return, with the second being the delay slot. If necessary this could be modified
// to support other configurations
void *CreateHook(void* function, void* hook, int* trampoline_buffer) {
	unsigned int* f_int = (unsigned int*) function;

	if(f_int[0] == 0x03e00008) {// jr ra
		trampoline_buffer[0] = f_int[0];
		trampoline_buffer[1] = f_int[1];
	} else {
		trampoline_buffer[1] = 0x08000000 | (((int) function >> 2) + 2); // insert a jump into 8 bytes into the function
		trampoline_buffer[0] = f_int[0];
		trampoline_buffer[2] = f_int[1];
	}
	f_int[0] = 0x08000000 | ((int) hook >> 2); // insert a jump to the hook
	f_int[1] = 0; // nop
	return trampoline_buffer;
}

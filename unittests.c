#include <stdio.h>
#include "memutils.h"


int main(int argc, char * argv[]) {

	/* BUD */
	int * a;
	// int * b = (int *)bud_malloc(2048);
	// printf("Address Returned: %p\n", b);
	// int * c = (int *)bud_malloc(2048);
	// printf("Address Returned: %p\n", c);	
	// int * d = (int *)bud_malloc(8192);
	// printf("Address Returned: %p\n", d);
	// int * e = (int *)bud_malloc(16384);
	// printf("Address Returned: %p\n", e);
	// int * f = (int *)bud_malloc(65536);
	// printf("Address Returned: %p\n", f);
	for(int i = 0; i < 4; ++i) {
		a = (int *)bud_malloc(16);
		printf("Address Returned: %p\n", a);
		bud_free(a);
	}
	// bud_free(b);
	// bud_free(c);
	// bud_free(d);
	// bud_free(e);
	// bud_free(f);

	/* BIT */
	// int * a;
	// int * b = (int *)bit_malloc(1000);
	// for(int i = 0; i < 2; ++i) {
	// 	a = (int *)bit_malloc(i);
	// 	printf("%p\n", a);
	// 	bit_free(a);
	// }
	// bit_free(b);

	return(0);
} 

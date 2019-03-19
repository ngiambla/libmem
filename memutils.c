#include "memutils.h"


void printdbg(char * strr) {
	#ifdef __DEBUG__ 
	printf("  >>%s\n", strr);	
	#endif 
}	

void printbytes(int isMalloc, int select, int value) {
	#ifdef __BYTES__
		if(isMalloc && select == 0) {
			printf("M-A:0x%08x;\n", value);
			return;
		}

		if(isMalloc && select == 1) {
			printf("M-N:%8d;\n", value);
			return;
		} 

		if(isMalloc && select == 2) {
			printf("M-C:%8d;\n", value);
			return;
		}

		if(!isMalloc) {
			printf("F-A:0x%08x;\n", value);
			return;
		}
	#endif
}

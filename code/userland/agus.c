#include <stdio.h>

int main(){
	char buffer [60];
	int proc [5] = {-1};
	for (int i = 0; i < 5; i++) 
		printf("proc[%d]: %d\n", i, proc[i]);
	buffer[0] = '&';
	while(1) {
		if (buffer[0] == '&') { continue; }
		printf("llego\n");
	}
return 0;
}


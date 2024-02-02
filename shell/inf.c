#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	//if (argc != 3) {
	//	fprintf(stderr, "Usage: inf tag interval\n");
	//} else {
		const char* tag = "test";
            //argv[1];
		int interval = 5;
            //atoi(argv[2]);
		while(1) {
			printf("%s\n", tag);
			sleep(interval);
		}
	//}
}


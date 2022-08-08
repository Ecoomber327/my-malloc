#include "my-malloc.h"
#include <stdint.h>

int main(int argc, char const *argv[]) {
	char *str, *str2, *str3, *str4;
	str = (char *)malloc(100);
	str2 = (char *)malloc(20);
	str4 = (char *)malloc(20);
	free(str2);
	str3 = (char *)malloc(10);
	free(str);
	free(str3);
	free(str4);
	return (EXIT_SUCCESS);
}

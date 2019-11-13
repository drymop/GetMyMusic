#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
	char* pass = getpass("Try getpass:\n");
	printf("%s\n", pass);
}
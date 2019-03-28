#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void swap_word(char *line, char *old, char *new){
	unsigned len_old = strlen(old);
	char *start = strstr(line, old);
	while(line != start){
		printf("%c", *(line++));
	}
	printf("%s", new);
	start += len_old;
	printf("%s", start);
}

int main(){

	char *t = "this is me";
	char *old = "is";
	unsigned len_old = strlen(old);
	char *new1 = "at";
	char *new2 = "";

	swap_word(t, old, new1);

	exit(0);
}
#define _POSIX_C_SOURCE 200809L
#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_input(void) {
char *line = NULL;
size_t sz = 0;
ssize_t n = getline(&line, &sz, stdin);
if (n == -1) {
free(line);
return NULL;
}
if (n > 0 && line[n-1] == '\n') line[n-1] = '\0';
return line;
}
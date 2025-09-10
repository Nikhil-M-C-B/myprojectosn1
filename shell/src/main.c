#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


#include "prompt.h"
#include "input.h"
#include "parser.h"


int main(void) {
char *shell_home = getcwd(NULL, 0); 
if (!shell_home) shell_home = strdup("/");

while (1) {
char *prompt = build_prompt(shell_home);
if (!prompt) { perror("build_prompt"); break; }

printf("%s ", prompt);
fflush(stdout);
free_prompt(prompt);
char *line = read_input();
if (!line) { 
printf("\n");
break;
}

char *p = line;
while (*p && isspace((unsigned char)*p)) p++;
if (*p == '\0') { free(line); continue; }

if (!validate_command(line)) {
printf("Invalid Syntax!\n");
}

free(line);
}
free(shell_home);
return 0;
}
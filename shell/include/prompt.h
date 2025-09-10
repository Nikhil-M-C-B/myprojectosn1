#ifndef PROMPT_H
#define PROMPT_H

char *build_prompt(const char *shell_home);
void free_prompt(char *p);
void show_prompt(void);

#endif
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "prompt.h"
#include "input.h"
#include "parser.h"
#include "builtins.h"

int main(void) {
    char *shell_home = getcwd(NULL, 0);
    if (!shell_home) shell_home = strdup("/");

    /* initialize builtins subsystem with shell home */
    builtins_init(shell_home);

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

        /* skip leading whitespace */
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') { free(line); continue; }

        /* validate syntax (Part A parser) */
        if (!validate_command(line)) {
            printf("Invalid Syntax!\n");
            free(line);
            continue;
        }

        /* store in history unless first atomic name is "log" (record_history handles that) */
        record_history(line);

        /* If builtin, handle and continue */
        if (handle_builtin(line)) {
            free(line);
            continue;
        }

        /* Non-builtin commands: Part C implements execution.
           For Part B we do nothing (no exec* syscalls allowed). */
        /* Optionally, for debugging you can print a placeholder:
           printf("External command (not implemented in Part B).\n");
        */

        free(line);
    }

    builtins_shutdown();
    free(shell_home);
    return 0;
}

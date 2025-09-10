#define _POSIX_C_SOURCE 200809L


#include "prompt.h"


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <stdio.h>


char *build_prompt(const char *shell_home) {
const char *username = "user";
struct passwd *pw = getpwuid(getuid());
if (pw && pw->pw_name) username = pw->pw_name;


char hostname[HOST_NAME_MAX + 1];
if (gethostname(hostname, sizeof(hostname)) != 0) strncpy(hostname, "host", sizeof(hostname));


char cwd[PATH_MAX + 1];
if (!getcwd(cwd, sizeof(cwd))) strncpy(cwd, "?", sizeof(cwd));


char display[PATH_MAX + 2];
if (shell_home && strlen(shell_home) > 0 && strncmp(cwd, shell_home, strlen(shell_home)) == 0) {
const char *rest = cwd + strlen(shell_home);
if (*rest == '\0') snprintf(display, sizeof(display), "~");
else if (*rest == '/') snprintf(display, sizeof(display), "~%s", rest);
else snprintf(display, sizeof(display), "%s", cwd);
} else {
snprintf(display, sizeof(display), "%s", cwd);
}


size_t need = strlen(username) + strlen(hostname) + strlen(display) + 8;
char *res = malloc(need);
if (!res) return NULL;
snprintf(res, need, "<%s@%s:%s>", username, hostname, display);
return res;
}


void free_prompt(char *p) { free(p); }

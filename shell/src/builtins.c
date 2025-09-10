#define _POSIX_C_SOURCE 200809L
#include "builtins.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "parser.h"

/* Globals */
static char shell_home_path[PATH_MAX+1] = {0};
static char history_path[PATH_MAX+1] = {0};
static const int HISTORY_MAX = 15;

/* previous directory for hop - / reveal - */
static char prev_cwd[PATH_MAX+1] = {0};
static int prev_valid = 0;

/* ---------- Helpers ---------- */

static void free_str_array(char **a, size_t n) {
    if (!a) return;
    for (size_t i = 0; i < n; ++i) free(a[i]);
    free(a);
}

/* read history file into array (oldest->newest).
   Always returns a valid pointer (maybe empty). */
static char **read_history_file(size_t *out_n) {
    *out_n = 0;
    FILE *f = fopen(history_path, "r");
    if (!f) {
        return calloc(1, sizeof(char*)); /* empty array */
    }
    char *line = NULL;
    size_t sz = 0;
    size_t n = 0, cap = 0;
    char **arr = NULL;
    while (getline(&line, &sz, f) != -1) {
        if (line[0] && line[strlen(line)-1] == '\n') {
            line[strlen(line)-1] = '\0';
        }
        if (n+1 > cap) {
            cap = cap ? cap * 2 : 8;
            arr = realloc(arr, sizeof(char*) * cap);
        }
        arr[n++] = strdup(line);
    }
    free(line);
    fclose(f);

    *out_n = n;
    if (!arr) arr = calloc(1, sizeof(char*));
    return arr;
}

/* write history back (oldest->newest) */
static int write_history_file(char **arr, size_t n) {
    FILE *f = fopen(history_path, "w");
    if (!f) return -1;
    for (size_t i = 0; i < n; ++i) {
        fprintf(f, "%s\n", arr[i]);
    }
    fclose(f);
    return 0;
}

/* ---------- Init / Shutdown ---------- */

int builtins_init(const char *shell_home) {
    if (shell_home && shell_home[0]) {
        strncpy(shell_home_path, shell_home, sizeof(shell_home_path)-1);
        shell_home_path[sizeof(shell_home_path)-1] = '\0';
    } else {
        const char *env_home = getenv("HOME");
        if (env_home) {
            strncpy(shell_home_path, env_home, sizeof(shell_home_path)-1);
            shell_home_path[sizeof(shell_home_path)-1] = '\0';
        }
    }

    /* build history path */
    if (shell_home_path[0]) {
        size_t len = strlen(shell_home_path);
        if (len < sizeof(history_path) - 12) {
            strcpy(history_path, shell_home_path);
            strcat(history_path, "/.osh_history");
        } else {
            strncpy(history_path, ".osh_history", sizeof(history_path)-1);
            history_path[sizeof(history_path)-1] = '\0';
        }
    } else {
        strncpy(history_path, ".osh_history", sizeof(history_path)-1);
        history_path[sizeof(history_path)-1] = '\0';
    }

    prev_valid = 0;
    prev_cwd[0] = '\0';
    return 0;
}

void builtins_shutdown(void) {
    /* nothing */
}

/* ---------- History recording ---------- */

int record_history(const char *line) {
    if (!line) return -1;

    int tn = 0;
    Token *toks = tokenize(line, &tn);
    if (!toks) return -1;
    if (tn > 0 && toks[0].type == TT_NAME &&
        toks[0].text && strcmp(toks[0].text, "log") == 0) {
        free_tokens(toks, tn);
        return 0; /* don't store log commands */
    }
    free_tokens(toks, tn);

    size_t n = 0;
    char **arr = read_history_file(&n);

    if (n > 0 && arr && arr[n-1] && strcmp(arr[n-1], line) == 0) {
        free_str_array(arr, n);
        return 0; /* skip duplicate */
    }

    char **newarr = realloc(arr, sizeof(char*) * (n + 1));
    if (!newarr) { free_str_array(arr, n); return -1; }
    arr = newarr;
    arr[n] = strdup(line);
    ++n;

    while (n > (size_t)HISTORY_MAX) {
        free(arr[0]);
        for (size_t i = 1; i < n; ++i) arr[i-1] = arr[i];
        --n;
    }

    int rc = write_history_file(arr, n);
    free_str_array(arr, n);
    return rc;
}

/* ---------- hop ---------- */

static int save_prev_and_chdir(const char *target) {
    char oldcwd[PATH_MAX+1] = {0};
    if (!getcwd(oldcwd, sizeof(oldcwd))) oldcwd[0] = '\0';

    if (chdir(target) != 0) return -1;

    if (oldcwd[0]) {
        strncpy(prev_cwd, oldcwd, sizeof(prev_cwd)-1);
        prev_cwd[sizeof(prev_cwd)-1] = '\0';
        prev_valid = 1;
    }
    return 0;
}

static int builtin_hop_tokens(Token *toks, int tn) {
    if (!toks || tn <= 0) return 0;

    if (tn == 1) {
        if (shell_home_path[0] == '\0' || save_prev_and_chdir(shell_home_path) != 0) {
            printf("No such directory!\n");
        }
        return 1;
    }

    for (int i = 1; i < tn; ++i) {
        if (!toks[i].text) continue;
        const char *arg = toks[i].text;

        if (strcmp(arg, "~") == 0) {
            if (save_prev_and_chdir(shell_home_path) != 0) printf("No such directory!\n");
        } else if (strcmp(arg, ".") == 0) {
            /* noop */
        } else if (strcmp(arg, "..") == 0) {
            char oldcwd[PATH_MAX+1] = {0};
            if (!getcwd(oldcwd, sizeof(oldcwd))) oldcwd[0] = '\0';
            if (chdir("..") != 0) {
                printf("No such directory!\n");
            } else if (oldcwd[0]) {
                strncpy(prev_cwd, oldcwd, sizeof(prev_cwd)-1);
                prev_valid = 1;
            }
        } else if (strcmp(arg, "-") == 0) {
            if (!prev_valid) {
                printf("No such directory!\n");
            } else if (save_prev_and_chdir(prev_cwd) != 0) {
                printf("No such directory!\n");
            }
        } else {
            if (save_prev_and_chdir(arg) != 0) printf("No such directory!\n");
        }
    }
    return 1;
}

/* ---------- reveal ---------- */

struct reveal_flags { int show_all; int line_by_line; };

static int cmp_str(const void *a, const void *b) {
    const char *const *pa = a;
    const char *const *pb = b;
    return strcmp(*pa, *pb);
}

static int list_directory(const char *path, struct reveal_flags flags) {
    DIR *d = opendir(path);
    if (!d) return -1;
    struct dirent *ent;
    char **names = NULL;
    size_t n = 0, cap = 0;
    while ((ent = readdir(d)) != NULL) {
        if (!flags.show_all && ent->d_name[0] == '.') continue;
        if (n+1 > cap) {
            cap = cap ? cap * 2 : 32;
            names = realloc(names, sizeof(char*) * cap);
        }
        names[n++] = strdup(ent->d_name);
    }
    closedir(d);
    if (n > 0) qsort(names, n, sizeof(char*), cmp_str);

    if (flags.line_by_line) {
        for (size_t i = 0; i < n; ++i) printf("%s\n", names[i]);
    } else {
        for (size_t i = 0; i < n; ++i) {
            printf("%s", names[i]);
            if (i+1 < n) printf(" ");
        }
        if (n > 0) printf("\n");
    }

    free_str_array(names, n);
    return 0;
}

static int builtin_reveal_tokens(Token *toks, int tn) {
    if (!toks || tn <= 0) return 0;
    struct reveal_flags flags = {0,0};
    const char *arg = NULL;
    int nonflag_count = 0;

    for (int i = 1; i < tn; ++i) {
        if (!toks[i].text) continue;
        const char *tok = toks[i].text;
        if (tok[0] == '-' && tok[1] != '\0') {
            for (size_t j = 1; j < strlen(tok); ++j) {
                if (tok[j] == 'a') flags.show_all = 1;
                else if (tok[j] == 'l') flags.line_by_line = 1;
            }
        } else {
            nonflag_count++;
            if (nonflag_count > 1) {
                printf("reveal: Invalid Syntax!\n");
                return 1;
            }
            arg = tok;
        }
    }

    char target[PATH_MAX+1] = {0};
    if (!arg) {
        if (!getcwd(target, sizeof(target))) {
            printf("No such directory!\n");
            return 1;
        }
    } else if (strcmp(arg, "~") == 0) {
        strncpy(target, shell_home_path, sizeof(target)-1);
    } else if (strcmp(arg, ".") == 0) {
        if (!getcwd(target, sizeof(target))) { printf("No such directory!\n"); return 1; }
    } else if (strcmp(arg, "..") == 0) {
        if (!getcwd(target, sizeof(target))) { printf("No such directory!\n"); return 1; }
        char *p = strrchr(target, '/');
        if (!p || p == target) strcpy(target, "/");
        else *p = '\0';
    } else if (strcmp(arg, "-") == 0) {
        if (!prev_valid) { printf("No such directory!\n"); return 1; }
        strncpy(target, prev_cwd, sizeof(target)-1);
    } else {
        strncpy(target, arg, sizeof(target)-1);
    }

    struct stat st;
    if (stat(target, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("No such directory!\n");
        return 1;
    }

    list_directory(target, flags);
    return 1;
}

/* ---------- log ---------- */

static void print_history(void) {
    size_t n = 0;
    char **arr = read_history_file(&n);
    for (size_t i = 0; i < n; ++i) {
        if (arr[i]) printf("%s\n", arr[i]);
    }
    free_str_array(arr, n);
}

static void purge_history(void) {
    FILE *f = fopen(history_path, "w");
    if (f) fclose(f);
}

static int handle_log_tokens(Token *toks, int tn) {
    /* case: just "log" */
    if (tn == 2 && toks[1].type == TT_EOF) {
        print_history();
        return 1;
    }

    if (tn >= 2 && toks[1].text) {
        if (strcmp(toks[1].text, "purge") == 0) {
            purge_history();
            return 1;
        } else if (strcmp(toks[1].text, "execute") == 0) {
            if (tn < 3 || !toks[2].text) {
                printf("log: Invalid Syntax!\n");
                return 1;
            }
            char *endptr = NULL;
            long idx = strtol(toks[2].text, &endptr, 10);
            if (*endptr != '\0' || idx <= 0) {
                printf("log: Invalid Syntax!\n");
                return 1;
            }

            size_t n = 0;
            char **arr = read_history_file(&n);
            if (!arr || n == 0 || (size_t)idx > n) {
                free_str_array(arr, n);
                printf("log: Invalid Syntax!\n");
                return 1;
            }
            size_t pos = n - (size_t)idx;
            printf("%s\n", arr[pos]);
            free_str_array(arr, n);
            return 1;
        }
    }

    printf("log: Invalid Syntax!\n");
    return 1;
}

/* ---------- Dispatcher ---------- */

int handle_builtin(const char *line) {
    if (!line) return 0;
    int tn = 0;
    Token *toks = tokenize(line, &tn);
    if (!toks) return 0;
    if (tn <= 0) { free_tokens(toks, tn); return 0; }

    int handled = 0;
    if (toks[0].type == TT_NAME && toks[0].text) {
        if (strcmp(toks[0].text, "hop") == 0) {
            handled = builtin_hop_tokens(toks, tn);
        } else if (strcmp(toks[0].text, "reveal") == 0) {
            handled = builtin_reveal_tokens(toks, tn);
        } else if (strcmp(toks[0].text, "log") == 0) {
            handled = handle_log_tokens(toks, tn);
        }
    }
    free_tokens(toks, tn);
    return handled;
}

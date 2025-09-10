#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ---------- Globals ----------
static Token *toks = NULL;
static int ntoks = 0;
static int idx = 0;

// ---------- Helpers ----------
static Token *cur(void) {
    if (idx < ntoks) return &toks[idx];
    return NULL;
}

static void adv(void) {
    if (idx < ntoks) idx++;
}

// ---------- Tokenizer ----------
Token *tokenize(const char *line, int *out_n) {
    int cap = 8;
    int n = 0;
    Token *arr = malloc(sizeof(Token) * cap);
    if (!arr) return NULL;

    const char *p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;

        if (*p == '|') {
            arr[n++] = (Token){TT_PIPE, strdup("|")};
            p++;
        } else if (*p == ';') {
            arr[n++] = (Token){TT_SEMI, strdup(";")};
            p++;
        } else if (*p == '&') {
            arr[n++] = (Token){TT_AMP, strdup("&")};
            p++;
        } else if (*p == '<') {
            arr[n++] = (Token){TT_REDIR_IN, strdup("<")};
            p++;
        } else if (*p == '>') {
            if (*(p+1) == '>') {
                arr[n++] = (Token){TT_REDIR_OUT_APPEND, strdup(">>")};
                p += 2;
            } else {
                arr[n++] = (Token){TT_REDIR_OUT, strdup(">")};
                p++;
            }
        } else {
            const char *start = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' &&
                   *p != '|' && *p != ';' && *p != '&' && *p != '<' && *p != '>') {
                p++;
            }
            int len = p - start;
            char *txt = malloc(len + 1);
            if (!txt) { free_tokens(arr, n); return NULL; }
            memcpy(txt, start, len);
            txt[len] = '\0';
            arr[n++] = (Token){TT_NAME, txt};
        }

        if (n >= cap) {
            cap *= 2;
            arr = realloc(arr, sizeof(Token) * cap);
            if (!arr) return NULL;
        }
    }

    arr[n++] = (Token){TT_EOF, NULL};
    *out_n = n;
    return arr;
}

void free_tokens(Token *toks, int n) {
    for (int i = 0; i < n; i++) {
        free(toks[i].text);
    }
    free(toks);
}

// ---------- Parser ----------
int parse_atomic(void) {
    if (!cur() || cur()->type != TT_NAME) return 0;
    adv(); // consume NAME

    while (cur()) {
        if (cur()->type == TT_NAME) {
            adv();
        } else if (cur()->type == TT_REDIR_IN) {
            adv();
            if (!cur() || cur()->type != TT_NAME) return 0;
            adv();
        } else if (cur()->type == TT_REDIR_OUT || cur()->type == TT_REDIR_OUT_APPEND) {
            adv();
            if (!cur() || cur()->type != TT_NAME) return 0;
            adv();
        } else {
            break;
        }
    }
    return 1;
}

int parse_cmd_group(void) {
    if (!parse_atomic()) return 0;
    while (cur() && cur()->type == TT_PIPE) {
        adv();
        if (!parse_atomic()) return 0;
    }
    return 1;
}

int parse_shell_cmd(void) {
    if (!parse_cmd_group()) return 0;

    while (cur() && (cur()->type == TT_SEMI || cur()->type == TT_AMP)) {
        adv();
        if (!parse_cmd_group()) return 0;
    }

    if (cur() && cur()->type == TT_AMP) {
        adv();
    }

    return (cur() && cur()->type == TT_EOF);
}

// ---------- Validation ----------
int validate_command(const char *line) {
    int n;
    toks = tokenize(line, &n);
    if (!toks) return 0;

    ntoks = n;
    idx = 0;

    int ok = parse_shell_cmd();
    // if (!ok) printf("Invalid Syntax!\n");

    free_tokens(toks, n);
    toks = NULL;
    ntoks = 0;
    idx = 0;

    return ok;
}

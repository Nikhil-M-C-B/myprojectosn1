#ifndef PARSER_H
#define PARSER_H

// ---------- Token definitions ----------
typedef enum {
    TT_NAME,            // command or argument
    TT_REDIR_IN,        // <
    TT_REDIR_OUT,       // >
    TT_REDIR_OUT_APPEND,// >>
    TT_PIPE,            // |
    TT_SEMI,            // ;
    TT_AMP,             // &
    TT_EOF              // end of input
} TokenType;

typedef struct {
    TokenType type;
    char *text;         // dynamically allocated string (if any)
} Token;

// ---------- Functions ----------
Token *tokenize(const char *line, int *out_n);
void free_tokens(Token *toks, int n);

int parse_atomic(void);
int parse_cmd_group(void);
int parse_shell_cmd(void);

int validate_command(const char *line);

#endif

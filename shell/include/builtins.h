#ifndef BUILTINS_H
#define BUILTINS_H

/* Builtins for Part B: hop, reveal, log */

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize builtins subsystem. Pass shell_home (cwd at shell start). */
int builtins_init(const char *shell_home);

/* Handle a line that is a builtin. Returns:
 *   1 if the line was a builtin and handled,
 *   0 if the line is not a builtin (caller should run external commands).
 */
int handle_builtin(const char *line);

/* Record a validated command into persistent history (obeys rules).
 * Return 0 on success, -1 on error (non-fatal).
 */
int record_history(const char *line);

/* Optional cleanup */
void builtins_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* BUILTINS_H */

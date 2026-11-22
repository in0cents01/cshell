#ifndef MYSHELL_PARSER_H
#define MYSHELL_PARSER_H

char **parse_line(char *line);
void free_argv(char **argv);

#endif /* MYSHELL_PARSER_H */

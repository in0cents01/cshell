/* Headless unit tests for the terminal engine. */
#include "../terminal.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (line %d)\n", (msg), __LINE__); \
        failures++; \
    } else { \
        printf("ok: %s\n", (msg)); \
    } \
} while (0)

static const char *cell_text(Buffer *b, int row, int col)
{
    static char buf[8];
    Cell *c = &b->cells[row * b->cols + col];
    if (c->codepoint == 0) c->codepoint = ' ';
    if (c->codepoint < 0x80) {
        buf[0] = (char)c->codepoint;
        buf[1] = 0;
    } else {
        int n = g_unichar_to_utf8((gunichar)c->codepoint, buf);
        buf[n] = 0;
    }
    return buf;
}

static void test_basic_text(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "hello", 5);
    Buffer *b = terminal_screen(t);
    CHECK(b->cur.col == 5, "cursor advanced to col 5");
    CHECK(strcmp(cell_text(b, 0, 0), "h") == 0, "first cell is h");
    CHECK(strcmp(cell_text(b, 0, 4), "o") == 0, "fifth cell is o");
    CHECK(strcmp(cell_text(b, 0, 5), " ") == 0, "sixth cell is blank");
    terminal_free(t);
}

static void test_colors(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "\033[31mred\033[0m", 12);
    Buffer *b = terminal_screen(t);
    Cell *c0 = &b->cells[0];
    CHECK(c0->fg.kind == COLOR_INDEXED && c0->fg.index == 1, "red fg index 1");
    Cell *c3 = &b->cells[3];
    CHECK(c3->fg.kind == COLOR_DEFAULT, "fg reset after SGR 0");
    terminal_free(t);
}

static void test_bold_green(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "\033[1;32mX", 8);
    Buffer *b = terminal_screen(t);
    Cell *c = &b->cells[0];
    CHECK((c->attrs & ATTR_BOLD) != 0, "bold attribute set");
    CHECK(c->fg.kind == COLOR_INDEXED && c->fg.index == 2, "green fg index 2");
    terminal_free(t);
}

static void test_cursor_move(void)
{
    Terminal *t = terminal_new(20, 5, 100);
    terminal_feed(t, "abc\033[2;3Hxy", 11);
    Buffer *b = terminal_screen(t);
    CHECK(b->cur.row == 1 && b->cur.col == 4, "cursor at row 1 col 4 after CUP + xy");
    CHECK(strcmp(cell_text(b, 0, 0), "a") == 0, "row 0 still has a");
    CHECK(strcmp(cell_text(b, 1, 2), "x") == 0, "x written at new cursor");
    terminal_free(t);
}

static void test_erase_line(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "abcdef\033[3G\033[K", 13);
    Buffer *b = terminal_screen(t);
    CHECK(strcmp(cell_text(b, 0, 1), "b") == 0, "col 1 intact");
    CHECK(strcmp(cell_text(b, 0, 2), " ") == 0, "col 2 erased (cursor pos)");
    CHECK(strcmp(cell_text(b, 0, 3), " ") == 0, "col 3 erased");
    CHECK(strcmp(cell_text(b, 0, 5), " ") == 0, "col 5 erased");
    terminal_free(t);
}

static void test_alt_screen(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "main", 4);
    terminal_feed(t, "\033[?1049h", 8);
    CHECK(terminal_is_alt_screen(t), "switched to alt screen");
    terminal_feed(t, "alt!", 4);
    Buffer *b = terminal_screen(t);
    CHECK(strcmp(cell_text(b, 0, 0), "a") == 0, "alt screen has alt text");
    terminal_feed(t, "\033[?1049l", 8);
    CHECK(!terminal_is_alt_screen(t), "back to main screen");
    Buffer *m = terminal_screen(t);
    CHECK(strcmp(cell_text(m, 0, 0), "m") == 0, "main screen restored");
    terminal_free(t);
}

static void test_scrollback(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    /* CRLF line endings, as a real terminal emits */
    terminal_feed(t, "l1\r\nl2\r\nl3\r\nl4\r\nl5", 19);
    Buffer *b = terminal_screen(t);
    CHECK(terminal_scrollback_count(t) >= 2, "scrollback captured lines");
    CHECK(strcmp(cell_text(b, 2, 0), "l") == 0, "bottom line is l5");
    CHECK(strcmp(cell_text(b, 2, 1), "5") == 0, "bottom line is l5 (col 1)");
    terminal_free(t);
}

static void test_utf8(void)
{
    Terminal *t = terminal_new(20, 3, 100);
    const char *s = "héllo";
    terminal_feed(t, s, strlen(s));
    Buffer *b = terminal_screen(t);
    CHECK(strcmp(cell_text(b, 0, 1), "é") == 0, "UTF-8 é decoded");
    CHECK(b->cur.col == 5, "cursor advanced 5 cells");
    terminal_free(t);
}

static void test_tab(void)
{
    Terminal *t = terminal_new(20, 3, 100);
    terminal_feed(t, "a\tb", 3);
    Buffer *b = terminal_screen(t);
    CHECK(b->cur.col == 9, "tab moved to col 9");
    CHECK(strcmp(cell_text(b, 0, 8), "b") == 0, "b at col 8");
    terminal_free(t);
}

static void test_wrap(void)
{
    Terminal *t = terminal_new(5, 2, 100);
    terminal_feed(t, "abcdef", 6);
    Buffer *b = terminal_screen(t);
    CHECK(b->cur.row == 1 && b->cur.col == 1, "wrapped to next line");
    CHECK(strcmp(cell_text(b, 1, 0), "f") == 0, "f on second line");
    terminal_free(t);
}

static void test_bracketed_paste_mode(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    CHECK(!terminal_bracketed_paste(t), "bracketed paste off by default");
    terminal_feed(t, "\033[?2004h", 8);
    CHECK(terminal_bracketed_paste(t), "bracketed paste enabled by CSI ?2004h");
    terminal_feed(t, "\033[?2004l", 8);
    CHECK(!terminal_bracketed_paste(t), "bracketed paste disabled by CSI ?2004l");
    terminal_free(t);
}

static void test_charset_designation(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "\033(0", 3);   /* designate G0, then 'a' */
    terminal_feed(t, "a", 1);
    Buffer *b = terminal_screen(t);
    CHECK(strcmp(cell_text(b, 0, 0), "a") == 0, "designation char not printed");
    CHECK(b->cur.col == 1, "one cell consumed for 'a'");
    terminal_free(t);
}

static void test_rep(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "a\033[5b", 5);
    Buffer *b = terminal_screen(t);
    CHECK(strcmp(cell_text(b, 0, 1), "a") == 0, "REP repeats last char");
    CHECK(b->cur.col == 6, "cursor advanced past repeated chars");
    terminal_free(t);
}

static void test_erase_scrollback(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "l1\r\nl2\r\nl3\r\nl4", 14);
    CHECK(terminal_scrollback_count(t) > 0, "scrollback populated");
    terminal_feed(t, "\033[3J", 4);
    CHECK(terminal_scrollback_count(t) == 0, "ED 3 clears scrollback");
    terminal_free(t);
}

static char last_title[128];

static void on_test_title(Terminal *t, const char *title, void *user_data)
{
    (void)t; (void)user_data;
    snprintf(last_title, sizeof(last_title), "%s", title ? title : "");
}

static void test_title(void)
{
    last_title[0] = '\0';
    Terminal *t = terminal_new(10, 3, 100);
    terminal_set_title_callback(t, on_test_title, NULL);
    terminal_feed(t, "\033]2;hello\007", 11);
    CHECK(strcmp(last_title, "hello") == 0, "OSC 2 title callback fired");
    terminal_free(t);
}

static void test_resize(void)
{
    Terminal *t = terminal_new(10, 3, 100);
    terminal_feed(t, "abcdef", 6);
    terminal_resize(t, 8, 4);
    CHECK(terminal_cols(t) == 8 && terminal_rows(t) == 4, "resize updates geometry");
    Buffer *b = terminal_screen(t);
    CHECK(strcmp(cell_text(b, 0, 0), "a") == 0, "content preserved across resize");
    CHECK(b->cur.col == 0, "cursor reset after resize");
    terminal_free(t);
}

int main(void)
{
    test_basic_text();
    test_colors();
    test_bold_green();
    test_cursor_move();
    test_erase_line();
    test_alt_screen();
    test_scrollback();
    test_utf8();
    test_tab();
    test_wrap();
    test_bracketed_paste_mode();
    test_charset_designation();
    test_rep();
    test_erase_scrollback();
    test_title();
    test_resize();

    if (failures == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", failures);
    return 1;
}

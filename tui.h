#ifndef TUI_H
#define TUI_H

#include <stdio.h>

#define VLINE    "│" 
#define HLINE    "─" 
#define TLCORNER "┌"
#define BLCORNER "└"
#define TRCORNER "┐"
#define BRCORNER "┘"
#define LTEE     "├"
#define RTEE     "┤"
#define BTEE     "┴"
#define TTEE     "┬"
#define PLUS     "─"
#define RGB(t, r, g, b) "\033[38;2;" #r ";" #g ";" #b "m" t "\033[0m"
#define SET_COLOR(r, g, b) printf("\033[38;2;" #r ";" #g ";" #b "m")
#define RESET_VIDEO() printf("\033[0m")
#define INVERT() printf("\033[7m")
#define BOLD() printf("\033[1m")
#define SET_BG(r, g, b) printf("\033[48;2;" #r ";" #g ";" #b "m")

typedef struct {
	unsigned width;
	unsigned height;
	unsigned x;
	unsigned y;
	unsigned has_borders;
} Window;

unsigned get_term_width();
unsigned get_term_height();

unsigned get_cursor_x_position();
unsigned get_cursor_y_position();

void inittui();
void endtui();

char getch();

void printfxy(unsigned x, unsigned y, const char * fmt, ...);

void printfxy_to_window(Window * win, int x, int y, const char * fmt, ...);
void draw_window(Window * win);

int is_echo();
void showecho(int echo);
void showcursor(int show);

#endif

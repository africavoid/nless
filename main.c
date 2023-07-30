/* NeoLess a Less clone with some extra stuff */
/*
	TODO
	- creating a way of scrolling
	- fix draw_buf so it draws only until it runs out of screen space to avoid annoying thing
	- create a cmd line 
	- create way of converting scr->max_y to numbered lines
	- create keypad support
	- create way to ouput directory contents ? maybe ? 
	- make support for dynamic resizing
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <curses.h>

#define DEFAULT_CURSOR_POSITION 0
#define MAX 1024

/* 
	screen information 
	cur vars are current x y positions
	cur_max_y is the current last line
	max_y/x is the max height/width of screen
*/
typedef struct {
	WINDOW *win;
	size_t max_y, max_x;
	size_t cur_y, cur_x;
	size_t old_y, old_x;
	size_t cur_max_y;
}SCR;

/* file information */
typedef struct {
	char *line[MAX];
	size_t line_max_y;
	size_t scroll_start_y;
}BUF;

static void mov_left (SCR *scr, BUF *buf);
static void mov_down (SCR *scr, BUF *buf);
static void mov_up (SCR *scr, BUF *buf);
static void mov_right (SCR *scr, BUF *buf);
static void mov_end (SCR *scr, BUF *buf);
static void cmd_line_init (SCR *scr, BUF *buf);
static void draw_buf (SCR *scr, BUF *buf, size_t start_y);

struct KEY_MAP {
	const int ch;
	void (*function)(SCR *scr, BUF *buf);
};

/* key binds for program */
struct KEY_MAP key_map[] = {
	{ 'h', mov_left },
	{ 'j', mov_down },
	{ 'k', mov_up },
	{ 'l', mov_right },
	{ ':', cmd_line_init },
	{ 'A', mov_end },
};

struct CMD_MAP {
	const char *cmd;
	void (*function)(SCR *scr, BUF *buf);
};

static void mov_left (SCR *scr, BUF *buf)
{
	if (scr->cur_x > 0)
		scr->cur_x--;
	wmove(scr->win, scr->cur_y, scr->cur_x);
}

static void mov_down (SCR *scr, BUF *buf)
{
	if (scr->cur_y > scr->max_y) {
		scrollok(scr->win, TRUE);
		wscrl(scr->win, 1);
		scrollok(scr->win, FALSE);
		draw_buf(scr, buf, scr->cur_y);
		scr->cur_max_y++;
	}

	scr->cur_y++;
	wmove(scr->win, scr->cur_y, scr->cur_x);
}

static void mov_up (SCR *scr, BUF *buf)
{
	
	if (scr->cur_y > 0) {
		if (scr->cur_y-- > scr->max_y) {
			scrollok(scr->win, TRUE);
			wscrl(scr->win, - 1);
			scrollok(scr->win, FALSE);
			draw_buf(scr, buf, scr->cur_y);
		}
		scr->cur_y--;
	}

	wmove(scr->win, scr->cur_y, scr->cur_x);
}

static void mov_right (SCR *scr, BUF *buf)
{
	//if (scr->cur_x < scr->max_x)
		scr->cur_x++;
	wmove(scr->win, scr->cur_y, scr->cur_x);
}

static void mov_end (SCR *scr, BUF *buf)
{
	scr->cur_x = scr->max_x--;
	wmove(scr->win, scr->cur_y, scr->cur_x);
}

// TODO this 
static bool cmd_parser (const char *cmd)
{
	
	return true;
}

static void cmd_line_init (SCR *scr, BUF *buf)
{
	char *cmd = malloc(MAX);
	size_t height = scr->cur_max_y - 1;
	mvwprintw(scr->win, height, 0, ":");
	wrefresh(scr->win);
	echo();
	wscanw(scr->win, cmd);

	if (cmd_parser(cmd) == false) {
		wprintw(scr->win, "%s is not a nless command!", cmd);
		wrefresh(scr->win);
		getch();
	}
	
	/* cleanup */
	mvwprintw(scr->win, height, 0, "%s", buf->line[height]);
	noecho();
	free(cmd);
	wmove(scr->win, scr->old_y, scr->old_x);
}

static void usage (const char *name)
{
	fprintf(stdout, "USAGE:\n");
	fprintf(stdout, "%s [file] (displays the file contents)\n", name);
	fprintf(stdout, "-h (outputs usage)\n");
	fprintf(stdout, "-s (outputs screen dimensions, mainly for debugging)\n");
}

/*
	converts the scr->max_y
	to line numbers
*/
// TODO 
static size_t convert_scr_y (size_t max_y)
{
	size_t line_numbers = 0;
	return line_numbers;
}

/* pretty simple goes to select line number */
static void goto_line (SCR *scr, BUF *buf, size_t line_num)
{
	wmove(scr->win, line_num, scr->cur_x);
}

static void get_dimensions (void)
{
	size_t y, x;
	if (initscr() == NULL)
		fprintf(stderr, "ERROR: Couldnt start curses!\n\a");
	else {
		getmaxyx(stdscr, y, x);
		endwin();
		fprintf(stdout, "y = %zu\nx = %zu\n\a", y, x);
	}
}

static char cmd_line_arg_parser (const char *name, const char *flag)
{
	switch (flag[1]) {
		case 'h':
			usage(name);
			return 'h';
		break;
		case 's':
			get_dimensions();
			return 's';
		break;
	}
	return '\0';
}

/*
	gets the size of a file,
	to allocate enough memory for buf
	TODO worth not using fseek to get size
*/
static size_t get_size (const char *file_name)
{
	FILE *file_pointer;
	size_t file_size = -1;
	
	if ((file_pointer = fopen(file_name, "r")) == NULL)
		return file_size;

	fseek(file_pointer, 0L, SEEK_END);
	file_size = ftell(file_pointer);
	fclose(file_pointer);
	return file_size;
}

/*
	reads file line by line
	sets the line into buf->line
	returns 1 if it fails
*/
static int read_file (BUF *buf, const char *file_name)
{
	FILE *file_pointer;
	char *line = NULL;
	size_t line_size = 0, i;
	ssize_t line_len;
	
	if ((file_pointer = fopen(file_name, "r")) == NULL)
		return -1;	

	for (i = 0; (line_len = getline(&line, &line_size, file_pointer)) > 0; i++) {
		if ((buf->line[i] = strndup(line, MAX)) == NULL)
			return -1;
		buf->line_max_y = i;
	}

	fclose(file_pointer);	
	free(line);
	return 0;
}

/* main loop, takes input and runs a command based on it */
static void input_loop (SCR *scr, BUF *buf)
{
	int ch, i;
	
	while (true) {
		i = 0;
		ch = wgetch(scr->win);
		while (key_map[i].ch != ch && key_map[i].ch != (char) 0)
			i++;

		/* allows for seamless switch */	
		scr->old_y = scr->cur_y;
		scr->old_x = scr->cur_x;

		if (ch == key_map[i].ch)
			key_map[i].function(scr, buf);
	}
}

static void cleanup (SCR *scr, BUF *buf)
{
	endwin();
	free(scr);
	free(buf);
}

/* sets up ncurses */
// TODO make a curses options
static int curses_setup (SCR *scr)
{
	if (initscr() == NULL)
		return -1;

	getmaxyx(stdscr, scr->max_y, scr->max_x);
	scr->win = newwin(scr->max_y, scr->max_x, 0, 0);
	scr->cur_y = scr->cur_x = DEFAULT_CURSOR_POSITION;
	scr->cur_max_y = scr->max_y;
	wmove(scr->win, scr->cur_y, scr->cur_x);
	cbreak();
	noecho();
	curs_set(1);
	refresh();
	return 0;
}

/* draws the file contents to the screen */
static void draw_buf (SCR *scr, BUF *buf, size_t start_y)
{
	size_t i;
	for (i = start_y; i < buf->line_max_y; i++)
		wprintw(scr->win, "%s", buf->line[i]);
	wrefresh(scr->win);
	wmove(scr->win, scr->cur_y, scr->cur_x);
}

int main (int argc, const char **argv)
{
	SCR *scr;
	BUF *buf;
	char flag;
	size_t file_size = -1;
	
	if (argc < 2) {
		fprintf(stdout, "Nothing to do ...\n\a");
		return 0;
	}

	/* getting cmd line args */	
	if (argv[1][0] == '-' && argv[1][2] == '\0') {
		if ((flag = cmd_line_arg_parser(argv[0], argv[1])) == '\0') {
			fprintf(stderr, "ERROR: No such flag, see '-h' for usage!\n\a");
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	} else if (argv[1][0] == '-' && argv[1][1] == '\0' || argv[1][1] == ' ') {
		fprintf(stderr, "ERROR: No such flag, see '-h' for usage!\n\a");
		return EXIT_FAILURE;
	}

	// TODO make multiple file support
	/* general setup from here down */
	if ((file_size = get_size(argv[1])) == -1) {
		fprintf(stderr, "ERROR: Reading file!\n\a");
		return EXIT_FAILURE;
	}
	
	if ((scr = malloc(sizeof(*scr))) == NULL) {
		fprintf(stderr, "ERROR: malloc (scr)\n\a");
		return EXIT_FAILURE;
	}
	
	if ((buf = malloc(sizeof(*buf) + file_size)) == NULL) {
		fprintf(stderr, "ERROR: malloc (buf)\n\a");
		return EXIT_FAILURE;
	}

	if (read_file(buf, argv[1]) == -1) {
		fprintf(stderr, "ERROR: Reading file!\n\a");
		return EXIT_FAILURE;
	}
	
	if (curses_setup(scr) == -1) {
		fprintf(stderr, "ERROR: Setting up curses!\n\a");
		return EXIT_FAILURE;
	}
	
	draw_buf(scr, buf, 0);
	input_loop(scr, buf);
	cleanup(scr, buf);
	return 0;
}

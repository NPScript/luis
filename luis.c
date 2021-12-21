#include "tui.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <magic.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

const char DTC[] = {
 [DT_BLK] =     'b',
 [DT_CHR] =     'c',
 [DT_DIR] =     'd',
 [DT_FIFO] =    'F',
 [DT_LNK] =     'l',
 [DT_REG] =     'f',
 [DT_SOCK] =    's',
 [DT_UNKNOWN] = '?',
};

const char * SUNIT[] = {"B", "KiB", "MiB", "GiB", "TiB"};

magic_t magic_cookie;

int (*current_filter)(const struct dirent *);
int selected_item = 0;
char current_path[PATH_MAX] = {0};

int filter_hidden(const struct dirent * e) {
	return e->d_name[0] != '.';
}

int filter_twodot_dir(const struct dirent * e) {
	return strcmp(e->d_name, "..") && strcmp(e->d_name, ".");
}

int number_of_dir_entries(const char * dpath) {
	struct dirent * entry;
	DIR * dir = opendir(dpath);
	int n = 0;

	while ((entry = readdir(dir)) != NULL)
		n += current_filter(entry);

	closedir(dir);

	return n;
}

void get_selection_name(const char * dpath, char * name) {
	struct dirent ** namelist;
	int n = scandir(dpath, &namelist, current_filter, alphasort);
	strcpy(name, namelist[selected_item]->d_name);

	while (n--)
		free(namelist[n]);
	free(namelist);
}

void write_dir_content_to_win(Window * win, const char * dpath, unsigned select, unsigned offset) {
	struct dirent ** namelist;
	int n = scandir(dpath, &namelist, current_filter, alphasort);

	if (n == 0) {
		SET_COLOR(180, 100, 100);
		printfxy_to_window(win, 0, offset, "empty");
		RESET_VIDEO();
	}

	for (unsigned i = 0; i < n && i < get_term_height(); ++i) {
		if (select == i) {
			SET_BG(60, 103, 84);
			SET_COLOR(0, 0, 0);
		}

		printfxy_to_window(win, 0, i + offset, " %c %s%*s", DTC[namelist[i]->d_type], namelist[i]->d_name, win->width - 5 - strlen(namelist[i]->d_name), "");

		if (select == i)
			RESET_VIDEO();

		free(namelist[i]);
	}

	free(namelist);
}

void switch_filter() {
	current_filter = current_filter == filter_hidden ? filter_twodot_dir : filter_hidden;
}

void select_move(int n) {
	if (selected_item + n >= 0 && selected_item + n < number_of_dir_entries(current_path))
		selected_item += n;
}

void enter_parent() {
	unsigned n = strlen(current_path) - 1;

	if (!n)
		return;

	while (current_path[--n]) {
		if (current_path[n] == '/') {
			current_path[n + 1] = 0;
			break;
		}
	}

	selected_item = 0;

}

void enter_directory() {
	get_selection_name(current_path, current_path + strlen(current_path));
	strcat(current_path, "/");
	int s = selected_item;

	DIR * dir = opendir(current_path);

	if (dir) {
		selected_item = 0;
		closedir(dir);

		struct dirent ** namelist;
		int n = scandir(current_path, &namelist, current_filter, alphasort);;

		for (int i = 0; i < n; ++i)
			free(namelist[i]);
		free(namelist);

		if (n == 0) {
			enter_parent();
			selected_item = s;
		}

		return;
	} else {
		showecho(1);
		showcursor(1);
		char open_with[2048];

		printfxy(1, 1, "open %s with: ", current_path);
		if (fgets(open_with, 256, stdin)) {
			open_with[strlen(open_with) - 1] = 0;
			current_path[strlen(current_path) - 1] = 0;
			endtui();
			system(strcat(strcat(open_with, " "), current_path));
			inittui();
			current_path[strlen(current_path) - 1] = '/';
		}

		showcursor(0);
		showecho(0);
	}

	closedir(dir);
	enter_parent();
	selected_item = s;
}

void help() {
	Window help;
	help.x = get_term_width() / 4;
	help.y = get_term_height() / 4;
	help.width = get_term_width() - 2 * help.x;
	help.height = get_term_height() - 2 * help.y;
	help.has_borders = 1;

	draw_window(&help);

	const char helptext[] = " H : Toggle Hidden\n"
	                        " j : Move down\n"
	                        " k : Move up\n"
	                        " h : Move back\n"
	                        " l : Enter Directory\n"
	                        " o : Open\n"
	                        " ? : Show this dialog\n";

	printfxy_to_window(&help, help.width / 2 - 4, 0, "- Help -");
	printfxy_to_window(&help, 0, 1, "%s", helptext);
	printfxy_to_window(&help, help.width - 16, help.height - 3, "[c]lose window");

	while (getch() != 'c');
}

char * readable_size(long size, char * dest) {
	int n = 0;

	while (size > 1000) {
		size /= 1024;
		++n;
	}

	sprintf(dest, "%d %s", size, SUNIT[n]);

	return dest;
}

void print_preview(Window * win) {
	struct stat selstat;
	char filepath[2048];
	char size[128];
	const char * magic_full;

	strcpy(filepath, current_path);
	get_selection_name(current_path, filepath + strlen(filepath));

	stat(filepath, &selstat);

	magic_full = magic_file(magic_cookie, filepath);

	SET_COLOR(100, 100, 100);
	printfxy_to_window(win, 0, 0, "%s\nLast modified     %sSize              %s\nType:             %s",
	                                filepath + strlen(current_path), ctime(&selstat.st_mtime), readable_size(selstat.st_size, size), magic_full);
	RESET_VIDEO();

	if (strstr(magic_full, "text")) {
		char buffer[4096] = "";
		FILE * fp = fopen(filepath, "r");

		fread(buffer, 4096, 1, fp);

		printfxy_to_window(win, 0, 6, "%s", buffer);

		fclose(fp);
	} else if (strstr(magic_full, "directory")) {
		SET_COLOR(100, 100, 100);
		printfxy_to_window(win, 0, 5, "Content");
		RESET_VIDEO();

		DIR * dir = opendir(filepath);

		if (dir == NULL) {
			SET_COLOR(180, 60, 60);
			printfxy_to_window(win, 0, 7, strerror(errno));
			RESET_VIDEO();
			return;
		}

		write_dir_content_to_win(win, filepath, -1, 7);
	}
}

int main(int argc, char ** argv) {
	Window mainwin;
	Window preview;

	mainwin.x = 1;
	mainwin.y = 2;
	mainwin.has_borders = 1;

	preview.y = 2;
	preview.has_borders = 1;

	if (argc == 2) {
		realpath(argv[1], current_path);

		printf(current_path);

		if (current_path[strlen(current_path) - 1] != '/') {
			current_path[strlen(current_path)] = '/';
		}

		DIR * dir = opendir(current_path);

		if (dir == NULL) {
			if (getcwd(current_path, sizeof(current_path)) == NULL) {
				fprintf(stderr, "getcwd error\n");
				return -1;
			}
			strcat(current_path, "/");
		}

		closedir(dir);

	} else {
		if (getcwd(current_path, sizeof(current_path)) == NULL) {
			fprintf(stderr, "getcwd error\n");
			return -1;
		}
		strcat(current_path, "/");
	}

	magic_cookie = magic_open(MAGIC_MIME_TYPE);

	if (!magic_cookie) {
		fprintf(stderr, "cannot initialize magic library\n");
		return -1;
	}

	if (magic_load(magic_cookie, NULL)) {
		fprintf(stderr, "cannot load magic database: %s\n", magic_error(magic_cookie));
		magic_close(magic_cookie);
		return -1;
	}

	inittui();
	showcursor(0);
	showecho(0);

	char c = 0;
	current_filter = filter_hidden;

	do {

		switch(c) {
			case 'H': switch_filter(); break;
			case 'j': select_move(1); break;
			case 'k': select_move(-1); break;
			case 'l': enter_directory(); break;
			case 'h': enter_parent(); break;
			case 'o': enter_directory(); break;
			case '?': help(); break;
			default: break;
		}

		mainwin.width = get_term_width() / 2;
		mainwin.height = get_term_height() - 1;

		preview.width = get_term_width() - mainwin.width + 1;
		preview.height = mainwin.height;
		preview.x = mainwin.width;

		printfxy(1, 1, "%s%*s[?] for help", current_path, get_term_width() - strlen(current_path) - 12, "");

		SET_COLOR(40, 74, 53);
		draw_window(&preview);
		BOLD();
		SET_COLOR(60, 103, 84);
		draw_window(&mainwin);
		RESET_VIDEO();

		write_dir_content_to_win(&mainwin, current_path, selected_item, 0);

		print_preview(&preview);

	} while ((c = getch()) != 'q');


	showecho(1);
	showcursor(1);

	endtui();
	magic_close(magic_cookie);
	return 0;
}

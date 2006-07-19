#include <ncursesw/ncurses.h>
#include "gntcolors.h"

static struct
{
	short r, g, b;
} colors[GNT_TOTAL_COLORS];

static void
backup_colors()
{
	short i;
	for (i = 0; i < GNT_TOTAL_COLORS; i++)
	{
		color_content(i, &colors[i].r,
				&colors[i].g, &colors[i].b);
	}
}

static void
restore_colors()
{
	short i;
	for (i = 0; i < GNT_TOTAL_COLORS; i++)
	{
		init_color(i, colors[i].r,
				colors[i].g, colors[i].b);
	}
}

void gnt_init_colors()
{
	start_color();
	if (can_change_color())
	{
		backup_colors();

		/* XXX: Do some init_color()s */
		init_color(GNT_COLOR_BLACK, 0, 0, 0);
		init_color(GNT_COLOR_RED, 1000, 0, 0);
		init_color(GNT_COLOR_GREEN, 0, 1000, 0);
		init_color(GNT_COLOR_BLUE, 250, 250, 700);
		init_color(GNT_COLOR_WHITE, 1000, 1000, 1000);
		init_color(GNT_COLOR_GRAY, 699, 699, 699);
		init_color(GNT_COLOR_DARK_GRAY, 256, 256, 256);

		/* Now some init_pair()s */
		init_pair(GNT_COLOR_NORMAL, GNT_COLOR_BLACK, GNT_COLOR_WHITE);
		init_pair(GNT_COLOR_HIGHLIGHT, GNT_COLOR_WHITE, GNT_COLOR_BLUE);
		init_pair(GNT_COLOR_SHADOW, GNT_COLOR_BLACK, GNT_COLOR_DARK_GRAY);

		init_pair(GNT_COLOR_TITLE, GNT_COLOR_WHITE, GNT_COLOR_BLUE);
		init_pair(GNT_COLOR_TITLE_D, GNT_COLOR_WHITE, GNT_COLOR_GRAY);

		init_pair(GNT_COLOR_TEXT_NORMAL, GNT_COLOR_WHITE, GNT_COLOR_BLUE);
		init_pair(GNT_COLOR_HIGHLIGHT_D, GNT_COLOR_BLACK, GNT_COLOR_GRAY);
		init_pair(GNT_COLOR_DISABLED, GNT_COLOR_GRAY, GNT_COLOR_WHITE);
	}
	else
	{
		init_pair(GNT_COLOR_NORMAL, COLOR_BLACK, COLOR_WHITE);
		init_pair(GNT_COLOR_HIGHLIGHT, COLOR_WHITE, COLOR_BLUE);
		init_pair(GNT_COLOR_SHADOW, COLOR_BLACK, COLOR_BLACK);
		init_pair(GNT_COLOR_TITLE, COLOR_WHITE, COLOR_BLUE);
		init_pair(GNT_COLOR_TITLE_D, COLOR_WHITE, COLOR_BLACK);
		init_pair(GNT_COLOR_TEXT_NORMAL, COLOR_WHITE, COLOR_BLUE);
		init_pair(GNT_COLOR_HIGHLIGHT_D, COLOR_CYAN, COLOR_BLACK);
		init_pair(GNT_COLOR_DISABLED, COLOR_YELLOW, COLOR_WHITE);
	}
}

void
gnt_uninit_colors()
{
	restore_colors();
}


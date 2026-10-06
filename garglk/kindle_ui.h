/******************************************************************************
 *                                                                            *
 * This file is part of Gargoyle.                                             *
 *                                                                            *
 * Gargoyle is free software; you can redistribute it and/or modify           *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 2 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Gargoyle is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Gargoyle; if not, write to the Free Software                    *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA *
 *                                                                            *
 *****************************************************************************/

#ifndef KINDLE_UI_H
#define KINDLE_UI_H

#include <gtk/gtk.h>

/* Widgets drawn with cairo in the flat black and white style of the Kindle
 * interface, instead of the bevelled default GTK+ 2 theme. Sizes derive from
 * the screen width so they scale across devices. */

typedef enum
{
    KINDLE_ICON_NONE,
    KINDLE_ICON_BACK,
    KINDLE_ICON_UP,
    KINDLE_ICON_PREVIOUS,
    KINDLE_ICON_NEXT,
    KINDLE_ICON_KEYBOARD,
    KINDLE_ICON_MENU,
    KINDLE_ICON_FOLDER,
    KINDLE_ICON_FILE,
} KindleIcon;

typedef void (*KindleCallback)(gpointer data);

int kindleUnit(void);
PangoFontDescription * kindleFont(double size, gboolean bold);
void kindleDrawIcon(cairo_t * cr, KindleIcon icon, double x, double y, double size);
void kindleRoundedRectangle(cairo_t * cr, double x, double y, double width, double height, double radius);

GtkWidget * kindleButtonNew(const char * label, KindleIcon icon, gboolean framed, KindleCallback clicked, gpointer data);
void kindleButtonSetEmphasis(GtkWidget * button, gboolean emphasis);

GtkWidget * kindleLabelNew(const char * text, double size, gboolean bold);

/* A white bar with a rule on its bottom or top edge, as used for headers and footers. */
GtkWidget * kindleBarNew(GtkWidget * content, gboolean ruleOnTop);

/* Centered modal dialog; returns TRUE when the accept button is pressed.
 * cancel may be NULL for a message with a single button. */
gboolean kindleDialogRun(GtkWindow * parent, const char * message, const char * cancel, const char * accept);

/* Menu dropping from the top right corner, like the Kindle menu.
 * Returns the index of the chosen item, or -1. */
int kindleMenuRun(GtkWindow * parent, int top, const char ** items, int count);

#endif /* KINDLE_UI_H */

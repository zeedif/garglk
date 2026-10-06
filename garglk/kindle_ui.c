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

#include <math.h>
#include <string.h>

#include "glk.h"
#include "garglk.h"
#include "kindle_ui.h"

/* Amazon Ember is the Kindle interface font; the others are fallbacks */
#define KINDLE_FONT_FAMILY "Amazon Ember,Bookerly,Sans"

typedef struct
{
    char * label;
    KindleIcon icon;
    gboolean framed;
    gboolean emphasis;
    gboolean pressed;
    gboolean menuItem;
    gboolean ruleBelow;
    KindleCallback clicked;
    gpointer data;
} KindleButton;

int kindleUnit(void)
{
    return MAX(gdk_screen_get_width(gdk_screen_get_default()) / 36, 12);
}

PangoFontDescription * kindleFont(double size, gboolean bold)
{
    PangoFontDescription * font = pango_font_description_from_string(KINDLE_FONT_FAMILY);

    pango_font_description_set_absolute_size(font, size * kindleUnit() * PANGO_SCALE);
    pango_font_description_set_weight(font, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
    return font;
}

void kindleRoundedRectangle(cairo_t * cr, double x, double y, double width, double height, double radius)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI / 2, 0);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI / 2);
    cairo_arc(cr, x + radius, y + height - radius, radius, M_PI / 2, M_PI);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

static void fillLight(cairo_t * cr)
{
    cairo_pattern_t * ink = cairo_pattern_reference(cairo_get_source(cr));

    cairo_set_source_rgb(cr, 0.88, 0.88, 0.88);
    cairo_fill_preserve(cr);
    cairo_set_source(cr, ink);
    cairo_pattern_destroy(ink);
}

void kindleDrawIcon(cairo_t * cr, KindleIcon icon, double x, double y, double s)
{
    int i;

    cairo_save(cr);
    cairo_set_line_width(cr, MAX(s / 12, 2));
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    switch (icon)
    {
        case KINDLE_ICON_BACK:
        case KINDLE_ICON_PREVIOUS:
            cairo_move_to(cr, x + 0.62 * s, y + 0.2 * s);
            cairo_line_to(cr, x + 0.32 * s, y + 0.5 * s);
            cairo_line_to(cr, x + 0.62 * s, y + 0.8 * s);
            cairo_stroke(cr);
            break;

        case KINDLE_ICON_NEXT:
            cairo_move_to(cr, x + 0.38 * s, y + 0.2 * s);
            cairo_line_to(cr, x + 0.68 * s, y + 0.5 * s);
            cairo_line_to(cr, x + 0.38 * s, y + 0.8 * s);
            cairo_stroke(cr);
            break;

        case KINDLE_ICON_UP:
            cairo_move_to(cr, x + 0.5 * s, y + 0.85 * s);
            cairo_line_to(cr, x + 0.5 * s, y + 0.18 * s);
            cairo_move_to(cr, x + 0.2 * s, y + 0.46 * s);
            cairo_line_to(cr, x + 0.5 * s, y + 0.16 * s);
            cairo_line_to(cr, x + 0.8 * s, y + 0.46 * s);
            cairo_stroke(cr);
            break;

        case KINDLE_ICON_KEYBOARD:
            kindleRoundedRectangle(cr, x + 0.05 * s, y + 0.24 * s, 0.9 * s, 0.54 * s, 0.08 * s);
            cairo_stroke(cr);
            for (i = 0; i < 5; i++)
            {
                cairo_rectangle(cr, x + (0.17 + i * 0.15) * s, y + 0.35 * s, 0.07 * s, 0.07 * s);
                cairo_rectangle(cr, x + (0.17 + i * 0.15) * s, y + 0.48 * s, 0.07 * s, 0.07 * s);
            }
            cairo_fill(cr);
            cairo_move_to(cr, x + 0.3 * s, y + 0.66 * s);
            cairo_line_to(cr, x + 0.7 * s, y + 0.66 * s);
            cairo_stroke(cr);
            break;

        case KINDLE_ICON_MENU:
            for (i = 0; i < 3; i++)
            {
                cairo_new_sub_path(cr);
                cairo_arc(cr, x + 0.5 * s, y + (0.22 + i * 0.28) * s, s / 11, 0, 2 * M_PI);
            }
            cairo_fill(cr);
            break;

        case KINDLE_ICON_FOLDER:
            cairo_move_to(cr, x + 0.08 * s, y + 0.22 * s);
            cairo_line_to(cr, x + 0.38 * s, y + 0.22 * s);
            cairo_line_to(cr, x + 0.47 * s, y + 0.32 * s);
            cairo_line_to(cr, x + 0.92 * s, y + 0.32 * s);
            cairo_line_to(cr, x + 0.92 * s, y + 0.8 * s);
            cairo_line_to(cr, x + 0.08 * s, y + 0.8 * s);
            cairo_close_path(cr);
            fillLight(cr);
            cairo_stroke(cr);
            break;

        case KINDLE_ICON_FILE:
            cairo_move_to(cr, x + 0.22 * s, y + 0.1 * s);
            cairo_line_to(cr, x + 0.6 * s, y + 0.1 * s);
            cairo_line_to(cr, x + 0.78 * s, y + 0.28 * s);
            cairo_line_to(cr, x + 0.78 * s, y + 0.9 * s);
            cairo_line_to(cr, x + 0.22 * s, y + 0.9 * s);
            cairo_close_path(cr);
            cairo_stroke(cr);
            cairo_move_to(cr, x + 0.6 * s, y + 0.1 * s);
            cairo_line_to(cr, x + 0.6 * s, y + 0.28 * s);
            cairo_line_to(cr, x + 0.78 * s, y + 0.28 * s);
            for (i = 0; i < 3; i++)
            {
                cairo_move_to(cr, x + 0.34 * s, y + (0.46 + i * 0.13) * s);
                cairo_line_to(cr, x + 0.66 * s, y + (0.46 + i * 0.13) * s);
            }
            cairo_stroke(cr);
            break;

        case KINDLE_ICON_NONE:
            break;
    }

    cairo_restore(cr);
}

static PangoLayout * buttonLayout(GtkWidget * widget, KindleButton * button)
{
    PangoLayout * layout = gtk_widget_create_pango_layout(widget, button->label);
    PangoFontDescription * font = kindleFont(0.85, button->emphasis);

    pango_layout_set_font_description(layout, font);
    pango_font_description_free(font);
    return layout;
}

static double buttonIconSize(KindleButton * button)
{
    return kindleUnit() * (button->label ? 1.1 : 1.3);
}

static double buttonContentWidth(KindleButton * button, int textWidth)
{
    double width = textWidth;

    if (button->icon != KINDLE_ICON_NONE)
        width += buttonIconSize(button) + (button->label ? kindleUnit() * 0.3 : 0);
    return width;
}

static void buttonSizeRequest(GtkWidget * widget, GtkRequisition * requisition, KindleButton * button)
{
    int textWidth = 0;

    if (button->label)
    {
        PangoLayout * layout = buttonLayout(widget, button);
        pango_layout_get_pixel_size(layout, &textWidth, NULL);
        g_object_unref(layout);
    }

    requisition->width = buttonContentWidth(button, textWidth) + kindleUnit() * (button->label ? 1.6 : 0.9);
    requisition->height = kindleUnit() * 2.1;
}

static gboolean buttonExpose(GtkWidget * widget, GdkEventExpose * event, KindleButton * button)
{
    cairo_t * cr = gdk_cairo_create(widget->window);
    double width = widget->allocation.width;
    double height = widget->allocation.height;
    double ink = button->pressed ? 1 : GTK_WIDGET_IS_SENSITIVE(widget) ? 0 : 0.62;
    double paper = button->pressed ? 0 : 1;
    PangoLayout * layout = NULL;
    int textWidth = 0, textHeight = 0;
    double x;

    cairo_set_source_rgb(cr, paper, paper, paper);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, ink, ink, ink);

    if (button->framed)
    {
        cairo_set_line_width(cr, button->emphasis ? 3 : 2);
        kindleRoundedRectangle(cr, 2, 2, width - 4, height - 4, kindleUnit() * 0.35);
        cairo_stroke(cr);
    }

    if (button->label)
    {
        layout = buttonLayout(widget, button);
        pango_layout_get_pixel_size(layout, &textWidth, &textHeight);
    }

    x = button->menuItem ? kindleUnit() : (width - buttonContentWidth(button, textWidth)) / 2;

    if (button->ruleBelow)
    {
        cairo_set_source_rgb(cr, 0.75, 0.75, 0.75);
        cairo_set_line_width(cr, 1);
        cairo_move_to(cr, kindleUnit() * 0.6, height - 0.5);
        cairo_line_to(cr, width - kindleUnit() * 0.6, height - 0.5);
        cairo_stroke(cr);
        cairo_set_source_rgb(cr, ink, ink, ink);
    }

    if (button->icon != KINDLE_ICON_NONE)
    {
        double size = buttonIconSize(button);
        kindleDrawIcon(cr, button->icon, x, (height - size) / 2, size);
        x += size + (button->label ? kindleUnit() * 0.3 : 0);
    }

    if (layout)
    {
        cairo_move_to(cr, x, (height - textHeight) / 2);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
    }

    cairo_destroy(cr);
    return TRUE;
}

static gboolean buttonPress(GtkWidget * widget, GdkEventButton * event, KindleButton * button)
{
    button->pressed = TRUE;
    gtk_widget_queue_draw(widget);
    return TRUE;
}

static gboolean buttonRelease(GtkWidget * widget, GdkEventButton * event, KindleButton * button)
{
    gboolean inside = event->x >= 0 && event->y >= 0
        && event->x < widget->allocation.width && event->y < widget->allocation.height;

    button->pressed = FALSE;
    gtk_widget_queue_draw(widget);

    if (inside && GTK_WIDGET_IS_SENSITIVE(widget) && button->clicked)
        button->clicked(button->data);
    return TRUE;
}

static void buttonDestroy(GtkWidget * widget, KindleButton * button)
{
    g_free(button->label);
    g_free(button);
}

GtkWidget * kindleButtonNew(const char * label, KindleIcon icon, gboolean framed, KindleCallback clicked, gpointer data)
{
    GtkWidget * widget = gtk_drawing_area_new();
    KindleButton * button = g_new0(KindleButton, 1);

    button->label = label ? g_strdup(label) : NULL;
    button->icon = icon;
    button->framed = framed;
    button->clicked = clicked;
    button->data = data;
    g_object_set_data(G_OBJECT(widget), "kindle-button", button);

    gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(widget, "size-request", G_CALLBACK(buttonSizeRequest), button);
    g_signal_connect(widget, "expose-event", G_CALLBACK(buttonExpose), button);
    g_signal_connect(widget, "button-press-event", G_CALLBACK(buttonPress), button);
    g_signal_connect(widget, "button-release-event", G_CALLBACK(buttonRelease), button);
    g_signal_connect(widget, "destroy", G_CALLBACK(buttonDestroy), button);
    return widget;
}

void kindleButtonSetEmphasis(GtkWidget * widget, gboolean emphasis)
{
    KindleButton * button = g_object_get_data(G_OBJECT(widget), "kindle-button");

    button->emphasis = emphasis;
    gtk_widget_queue_resize(widget);
}

GtkWidget * kindleLabelNew(const char * text, double size, gboolean bold)
{
    GtkWidget * label = gtk_label_new(text);
    PangoFontDescription * font = kindleFont(size, bold);
    GdkColor black = {0, 0, 0, 0};

    gtk_widget_modify_font(label, font);
    gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &black);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    pango_font_description_free(font);
    return label;
}

static void paintWhite(GtkWidget * widget)
{
    GdkColor white = {0, 0xffff, 0xffff, 0xffff};
    gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &white);
}

static gboolean barExpose(GtkWidget * widget, GdkEventExpose * event, gpointer ruleOnTop)
{
    cairo_t * cr = gdk_cairo_create(widget->window);
    double y = ruleOnTop ? 1 : widget->allocation.height - 1;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 0, y);
    cairo_line_to(cr, widget->allocation.width, y);
    cairo_stroke(cr);
    cairo_destroy(cr);
    return FALSE;
}

GtkWidget * kindleBarNew(GtkWidget * content, gboolean ruleOnTop)
{
    GtkWidget * bar = gtk_event_box_new();
    GtkWidget * alignment = gtk_alignment_new(0, 0.5, 1, 0);
    int unit = kindleUnit();

    paintWhite(bar);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), unit / 3, unit / 3, unit / 3, unit / 3);
    gtk_container_add(GTK_CONTAINER(alignment), content);
    gtk_container_add(GTK_CONTAINER(bar), alignment);
    g_signal_connect_after(bar, "expose-event", G_CALLBACK(barExpose), ruleOnTop ? bar : NULL);
    return bar;
}

/* A window whose white panel is framed by three pixels of black */
static GtkWidget * popupNew(GtkWindow * parent, GtkWidget ** panel)
{
    GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget * border = gtk_event_box_new();
    GtkWidget * alignment = gtk_alignment_new(0, 0, 1, 1);
    GdkColor black = {0, 0, 0, 0};

    gtk_window_set_title(GTK_WINDOW(window), KDIALOG);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(window), parent);

    gtk_widget_modify_bg(border, GTK_STATE_NORMAL, &black);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 3, 3, 3, 3);
    *panel = gtk_event_box_new();
    paintWhite(*panel);
    gtk_container_add(GTK_CONTAINER(alignment), *panel);
    gtk_container_add(GTK_CONTAINER(border), alignment);
    gtk_container_add(GTK_CONTAINER(window), border);
    return window;
}

/* Negative coordinates center the window on that axis */
static void popupShow(GtkWidget * window, int x, int y)
{
    GdkScreen * screen = gdk_screen_get_default();
    GtkRequisition size;

    gtk_widget_show_all(window);
    gtk_widget_size_request(window, &size);
    if (x < 0)
        x = (gdk_screen_get_width(screen) - size.width) / 2;
    if (y < 0)
        y = (gdk_screen_get_height(screen) - size.height) / 2;
    gtk_window_move(GTK_WINDOW(window), x, y);
    gtk_grab_add(window);
}

typedef struct
{
    GMainLoop * loop;
    int response;
} PopupState;

static void popupRespond(PopupState * state, int response)
{
    state->response = response;
    if (g_main_loop_is_running(state->loop))
        g_main_loop_quit(state->loop);
}

static void dialogAccept(gpointer data)
{
    popupRespond(data, 1);
}

static void dialogCancel(gpointer data)
{
    popupRespond(data, 0);
}

static gboolean popupDelete(GtkWidget * widget, GdkEvent * event, PopupState * state)
{
    popupRespond(state, -1);
    return TRUE;
}

static int popupRun(GtkWidget * window, PopupState * state)
{
    g_signal_connect(window, "delete-event", G_CALLBACK(popupDelete), state);
    g_main_loop_run(state->loop);
    gtk_grab_remove(window);
    gtk_widget_destroy(window);
    g_main_loop_unref(state->loop);
    return state->response;
}

gboolean kindleDialogRun(GtkWindow * parent, const char * message, const char * cancel, const char * accept)
{
    PopupState state = { g_main_loop_new(NULL, FALSE), 0 };
    int unit = kindleUnit();
    int width = gdk_screen_get_width(gdk_screen_get_default()) * 4 / 5;
    GtkWidget * panel;
    GtkWidget * window = popupNew(parent, &panel);
    GtkWidget * vbox = gtk_vbox_new(FALSE, unit);
    GtkWidget * buttons = gtk_hbox_new(TRUE, unit / 2);
    GtkWidget * label = kindleLabelNew(message, 0.9, FALSE);
    GtkWidget * acceptButton = kindleButtonNew(accept, KINDLE_ICON_NONE, TRUE, dialogAccept, &state);

    gtk_container_set_border_width(GTK_CONTAINER(vbox), unit);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_size_request(label, width - 2 * unit - 6, -1);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, unit / 2);

    if (cancel)
        gtk_box_pack_start(GTK_BOX(buttons), kindleButtonNew(cancel, KINDLE_ICON_NONE, TRUE, dialogCancel, &state), TRUE, TRUE, 0);
    kindleButtonSetEmphasis(acceptButton, TRUE);
    gtk_box_pack_start(GTK_BOX(buttons), acceptButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), buttons, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(panel), vbox);

    popupShow(window, -1, -1);
    return popupRun(window, &state) == 1;
}

typedef struct
{
    PopupState * state;
    int index;
} MenuItem;

static void menuChoose(gpointer data)
{
    MenuItem * item = data;
    popupRespond(item->state, item->index);
}

/* Taps outside the menu close it, as on the Kindle */
static gboolean menuPress(GtkWidget * widget, GdkEventButton * event, PopupState * state)
{
    if (event->x < 0 || event->y < 0 || event->x >= widget->allocation.width || event->y >= widget->allocation.height)
        popupRespond(state, -1);
    return FALSE;
}

int kindleMenuRun(GtkWindow * parent, int top, const char ** items, int count)
{
    PopupState state = { g_main_loop_new(NULL, FALSE), -1 };
    int unit = kindleUnit();
    int screenWidth = gdk_screen_get_width(gdk_screen_get_default());
    int width = MAX(screenWidth / 2, unit * 16);
    MenuItem * menuItems = g_new(MenuItem, count);
    GtkWidget * panel;
    GtkWidget * window = popupNew(parent, &panel);
    GtkWidget * vbox = gtk_vbox_new(FALSE, 0);
    int i, response;

    for (i = 0; i < count; i++)
    {
        GtkWidget * button;
        KindleButton * look;
        menuItems[i].state = &state;
        menuItems[i].index = i;
        button = kindleButtonNew(items[i], KINDLE_ICON_NONE, FALSE, menuChoose, &menuItems[i]);
        look = g_object_get_data(G_OBJECT(button), "kindle-button");
        look->menuItem = TRUE;
        look->ruleBelow = i < count - 1;
        gtk_widget_set_size_request(button, width - 6, unit * 2.6);
        gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    }
    gtk_container_add(GTK_CONTAINER(panel), vbox);

    popupShow(window, screenWidth - width - unit / 3, top);
    g_signal_connect(window, "button-press-event", G_CALLBACK(menuPress), &state);
    gdk_pointer_grab(window->window, TRUE, GDK_BUTTON_PRESS_MASK, NULL, NULL, GDK_CURRENT_TIME);

    response = popupRun(window, &state);
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    g_free(menuItems);
    return response;
}

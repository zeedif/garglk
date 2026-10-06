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
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "glk.h"
#include "garglk.h"
#include "gtk_utils.h"
#include "kindle_browser.h"
#include "kindle_l10n.h"

typedef struct
{
    char * name;
    char * key;
    gboolean directory;
} Entry;

typedef struct
{
    KindleBrowseMode mode;
    char * directory;
    GPtrArray * entries;
    int selected;
    int page;
    int rows;
    double pressX;
    double pressY;
    gboolean accepted;
    char * result;
    GMainLoop * loop;
    KindleCallback extra;
    gpointer extraData;

    GtkWidget * window;
    GtkWidget * up;
    GtkWidget * path;
    GtkWidget * list;
    GtkWidget * name;
    GtkWidget * pager;
    GtkWidget * previous;
    GtkWidget * next;
    GtkWidget * pageLabel;
    GtkWidget * accept;
} Browser;

static double rowHeight(void)
{
    return kindleUnit() * 2.6;
}

static void entryFree(gpointer data)
{
    Entry * entry = data;
    g_free(entry->name);
    g_free(entry->key);
    g_free(entry);
}

static gint entryCompare(gconstpointer a, gconstpointer b)
{
    const Entry * x = *(Entry * const *)a;
    const Entry * y = *(Entry * const *)b;

    if (x->directory != y->directory)
        return x->directory ? -1 : 1;
    return strcmp(x->key, y->key);
}

static int pageCount(Browser * browser)
{
    int rows = MAX(browser->rows, 1);
    return MAX(1, ((int)browser->entries->len + rows - 1) / rows);
}

static void updateControls(Browser * browser)
{
    int pages = pageCount(browser);
    gboolean canAccept;
    char * label;

    if (browser->directory == NULL)
        return;

    if (browser->mode == KINDLE_BROWSE_SAVE)
    {
        char * name = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(browser->name))));
        canAccept = *name != '\0';
        g_free(name);
    }
    else
        canAccept = browser->selected >= 0;
    gtk_widget_set_sensitive(browser->accept, canAccept);

    gtk_widget_set_sensitive(browser->up, strcmp(browser->directory, "/") != 0);

    label = g_strdup_printf("%d / %d", browser->page + 1, pages);
    gtk_label_set_text(GTK_LABEL(browser->pageLabel), label);
    g_free(label);
    gtk_widget_set_sensitive(browser->previous, browser->page > 0);
    gtk_widget_set_sensitive(browser->next, browser->page < pages - 1);
    if (pages > 1)
        gtk_widget_show(browser->pager);
    else
        gtk_widget_hide(browser->pager);

    gtk_widget_queue_draw(browser->list);
}

static void listDirectory(Browser * browser, const char * directory)
{
    GDir * dir = g_dir_open(directory, 0, NULL);
    const char * name;
    char * canonical = realpath(directory, NULL);

    g_free(browser->directory);
    browser->directory = g_strdup(canonical ? canonical : directory);
    free(canonical);

    g_ptr_array_set_size(browser->entries, 0);
    while (dir && (name = g_dir_read_name(dir)) != NULL)
    {
        Entry * entry;
        char * path;

        if (name[0] == '.')
            continue;

        path = g_build_filename(browser->directory, name, NULL);
        entry = g_new(Entry, 1);
        entry->name = g_strdup(name);
        entry->key = g_utf8_collate_key_for_filename(name, -1);
        entry->directory = g_file_test(path, G_FILE_TEST_IS_DIR);
        g_ptr_array_add(browser->entries, entry);
        g_free(path);
    }
    if (dir)
        g_dir_close(dir);
    g_ptr_array_sort(browser->entries, entryCompare);

    browser->selected = -1;
    browser->page = 0;
    gtk_label_set_text(GTK_LABEL(browser->path), browser->directory);
    updateControls(browser);
}

static void finish(Browser * browser, const char * result)
{
    browser->accepted = result != NULL;
    browser->result = result ? g_strdup(result) : NULL;
    g_main_loop_quit(browser->loop);
}

static void onAccept(gpointer data)
{
    Browser * browser = data;
    char * path;

    if (browser->mode == KINDLE_BROWSE_SAVE)
    {
        char * name = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(browser->name))));
        char * question;

        path = g_build_filename(browser->directory, name, NULL);
        question = g_strdup_printf(kindleTr("\"%s\" already exists. Do you want to replace it?"), name);
        if (g_file_test(path, G_FILE_TEST_EXISTS)
                && !kindleDialogRun(GTK_WINDOW(browser->window), question, kindleTr("Cancel"), kindleTr("Replace")))
        {
            g_free(path);
            path = NULL;
        }
        g_free(question);
        g_free(name);
    }
    else
    {
        Entry * entry = g_ptr_array_index(browser->entries, browser->selected);
        path = g_build_filename(browser->directory, entry->name, NULL);
    }

    if (path)
        finish(browser, path);
    g_free(path);
}

static void onCancel(gpointer data)
{
    finish(data, NULL);
}

static void onUp(gpointer data)
{
    Browser * browser = data;
    char * parent = g_path_get_dirname(browser->directory);

    listDirectory(browser, parent);
    g_free(parent);
}

static void turnPage(Browser * browser, int delta)
{
    int page = CLAMP(browser->page + delta, 0, pageCount(browser) - 1);

    if (page != browser->page)
    {
        browser->page = page;
        updateControls(browser);
    }
}

static void onPrevious(gpointer data)
{
    turnPage(data, -1);
}

static void onNext(gpointer data)
{
    turnPage(data, 1);
}

static void onExtra(gpointer data)
{
    Browser * browser = data;
    char * directory = g_strdup(browser->directory);

    browser->extra(browser->extraData);
    listDirectory(browser, directory);
    g_free(directory);
}

static void onNameChanged(GtkEditable * editable, gpointer data)
{
    updateControls(data);
}

static void tapRow(Browser * browser, int index)
{
    Entry * entry;

    if (index < 0 || index >= (int)browser->entries->len)
        return;

    entry = g_ptr_array_index(browser->entries, index);
    if (entry->directory)
    {
        char * path = g_build_filename(browser->directory, entry->name, NULL);
        listDirectory(browser, path);
        g_free(path);
        return;
    }

    /* a second tap on the selected file opens it */
    if (index == browser->selected && browser->mode != KINDLE_BROWSE_SAVE)
    {
        onAccept(browser);
        return;
    }

    browser->selected = index;
    if (browser->mode == KINDLE_BROWSE_SAVE)
        gtk_entry_set_text(GTK_ENTRY(browser->name), entry->name);
    updateControls(browser);
}

static gboolean listPress(GtkWidget * widget, GdkEventButton * event, Browser * browser)
{
    browser->pressX = event->x;
    browser->pressY = event->y;
    return TRUE;
}

/* Swipes turn pages, as e-ink readers do instead of scrolling */
static gboolean listRelease(GtkWidget * widget, GdkEventButton * event, Browser * browser)
{
    double dx = event->x - browser->pressX;
    double dy = event->y - browser->pressY;
    double threshold = kindleUnit() * 2;

    if (fabs(dx) > threshold && fabs(dx) > fabs(dy))
        turnPage(browser, dx < 0 ? 1 : -1);
    else if (fabs(dy) > threshold)
        turnPage(browser, dy < 0 ? 1 : -1);
    else
        tapRow(browser, browser->page * browser->rows + (int)(event->y / rowHeight()));
    return TRUE;
}

static void listAllocate(GtkWidget * widget, GtkAllocation * allocation, Browser * browser)
{
    int rows = MAX(1, (int)(allocation->height / rowHeight()));

    if (rows != browser->rows)
    {
        int first = browser->page * MAX(browser->rows, 1);
        browser->rows = rows;
        browser->page = first / rows;
        updateControls(browser);
    }
}

static gboolean listExpose(GtkWidget * widget, GdkEventExpose * event, Browser * browser)
{
    cairo_t * cr = gdk_cairo_create(widget->window);
    PangoLayout * layout = gtk_widget_create_pango_layout(widget, NULL);
    PangoFontDescription * font = kindleFont(0.95, FALSE);
    double unit = kindleUnit();
    double height = rowHeight();
    double width = widget->allocation.width;
    double iconSize = unit * 1.5;
    int first = browser->page * browser->rows;
    int i;

    pango_layout_set_font_description(layout, font);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    if (browser->entries->len == 0)
    {
        int textWidth, textHeight;
        pango_layout_set_text(layout, kindleTr("This folder is empty"), -1);
        pango_layout_get_pixel_size(layout, &textWidth, &textHeight);
        cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
        cairo_move_to(cr, (width - textWidth) / 2, unit * 3);
        pango_cairo_show_layout(cr, layout);
    }

    for (i = 0; i < browser->rows && first + i < (int)browser->entries->len; i++)
    {
        Entry * entry = g_ptr_array_index(browser->entries, first + i);
        double y = i * height;
        double textX = unit * 1.2 + iconSize;
        int textHeight;

        if (first + i == browser->selected)
        {
            cairo_set_source_rgb(cr, 0.92, 0.92, 0.92);
            kindleRoundedRectangle(cr, unit * 0.3, y + 3, width - unit * 0.6, height - 6, unit * 0.35);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_line_width(cr, 2);
            cairo_stroke(cr);
        }
        else
        {
            cairo_set_source_rgb(cr, 0.75, 0.75, 0.75);
            cairo_set_line_width(cr, 1);
            cairo_move_to(cr, unit * 0.6, y + height - 0.5);
            cairo_line_to(cr, width - unit * 0.6, y + height - 0.5);
            cairo_stroke(cr);
        }

        cairo_set_source_rgb(cr, 0, 0, 0);
        kindleDrawIcon(cr, entry->directory ? KINDLE_ICON_FOLDER : KINDLE_ICON_FILE,
                       unit * 0.7, y + (height - iconSize) / 2, iconSize);
        if (entry->directory)
            kindleDrawIcon(cr, KINDLE_ICON_NEXT, width - unit * 1.8, y + (height - unit) / 2, unit);

        pango_layout_set_width(layout, (width - textX - unit * 2.2) * PANGO_SCALE);
        pango_layout_set_text(layout, entry->name, -1);
        pango_layout_get_pixel_size(layout, NULL, &textHeight);
        cairo_move_to(cr, textX, y + (height - textHeight) / 2);
        pango_cairo_show_layout(cr, layout);
    }

    pango_font_description_free(font);
    g_object_unref(layout);
    cairo_destroy(cr);
    return TRUE;
}

static GtkWidget * headerNew(Browser * browser)
{
    GtkWidget * hbox = gtk_hbox_new(FALSE, kindleUnit() / 2);

    browser->up = kindleButtonNew(NULL, KINDLE_ICON_UP, TRUE, onUp, browser);
    browser->path = kindleLabelNew("", 0.85, TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(browser->path), PANGO_ELLIPSIZE_START);
    gtk_box_pack_start(GTK_BOX(hbox), browser->up, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), browser->path, TRUE, TRUE, 0);
    return kindleBarNew(hbox, FALSE);
}

static GtkWidget * footerNew(Browser * browser, const char * extraLabel)
{
    GtkWidget * hbox = gtk_hbox_new(FALSE, kindleUnit() * 3 / 4);
    GtkWidget * pagerBox = gtk_hbox_new(FALSE, 0);
    const char * cancelLabel = browser->mode == KINDLE_BROWSE_GAME ? kindleTr("Quit") : kindleTr("Cancel");
    const char * acceptLabel = browser->mode == KINDLE_BROWSE_GAME ? kindleTr("Open")
                             : browser->mode == KINDLE_BROWSE_RESTORE ? kindleTr("Restore") : kindleTr("Save");

    if (extraLabel)
        gtk_box_pack_start(GTK_BOX(hbox), kindleButtonNew(extraLabel, KINDLE_ICON_NONE, TRUE, onExtra, browser), FALSE, FALSE, 0);

    browser->previous = kindleButtonNew(NULL, KINDLE_ICON_PREVIOUS, FALSE, onPrevious, browser);
    browser->next = kindleButtonNew(NULL, KINDLE_ICON_NEXT, FALSE, onNext, browser);
    browser->pageLabel = kindleLabelNew("", 0.8, FALSE);
    gtk_box_pack_start(GTK_BOX(pagerBox), browser->previous, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pagerBox), browser->pageLabel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pagerBox), browser->next, FALSE, FALSE, 0);
    browser->pager = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(browser->pager), pagerBox);
    gtk_box_pack_start(GTK_BOX(hbox), browser->pager, TRUE, TRUE, 0);

    browser->accept = kindleButtonNew(acceptLabel, KINDLE_ICON_NONE, TRUE, onAccept, browser);
    kindleButtonSetEmphasis(browser->accept, TRUE);
    gtk_box_pack_end(GTK_BOX(hbox), kindleButtonNew(cancelLabel, KINDLE_ICON_NONE, TRUE, onCancel, browser), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), browser->accept, FALSE, FALSE, 0);

    return kindleBarNew(hbox, TRUE);
}

gboolean kindleBrowse(KindleBrowseMode mode, const char * directory, const char * suggestedName,
                      const char * extraLabel, KindleCallback extra, gpointer extraData,
                      char * buffer, int bufferSize)
{
    Browser browser = {0};
    GdkScreen * screen = gdk_screen_get_default();
    int width = gdk_screen_get_width(screen);
    int height = gdk_screen_get_height(screen);
    GtkWidget * vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget * listBox = gtk_event_box_new();
    GdkColor white = {0, 0xffff, 0xffff, 0xffff};

    browser.mode = mode;
    browser.entries = g_ptr_array_new_with_free_func(entryFree);
    browser.extra = extra;
    browser.extraData = extraData;
    browser.loop = g_main_loop_new(NULL, FALSE);
    browser.rows = 1;

    /* only file names have to be typed */
    if (mode == KINDLE_BROWSE_SAVE)
        height -= height / KBFACTOR;

    browser.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(browser.window), KDIALOG);
    gtk_window_set_decorated(GTK_WINDOW(browser.window), FALSE);
    gtk_widget_set_size_request(browser.window, width, height);
    gtk_window_set_resizable(GTK_WINDOW(browser.window), FALSE);
    gtk_widget_modify_bg(browser.window, GTK_STATE_NORMAL, &white);

    browser.list = gtk_drawing_area_new();
    gtk_widget_add_events(browser.list, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(browser.list, "expose-event", G_CALLBACK(listExpose), &browser);
    g_signal_connect(browser.list, "button-press-event", G_CALLBACK(listPress), &browser);
    g_signal_connect(browser.list, "button-release-event", G_CALLBACK(listRelease), &browser);
    g_signal_connect(browser.list, "size-allocate", G_CALLBACK(listAllocate), &browser);
    gtk_widget_modify_bg(listBox, GTK_STATE_NORMAL, &white);
    gtk_container_add(GTK_CONTAINER(listBox), browser.list);

    gtk_box_pack_start(GTK_BOX(vbox), headerNew(&browser), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), listBox, TRUE, TRUE, 0);

    if (mode == KINDLE_BROWSE_SAVE)
    {
        GtkWidget * alignment = gtk_alignment_new(0, 0, 1, 1);
        PangoFontDescription * font = kindleFont(1.0, FALSE);

        browser.name = gtk_entry_new();
        gtk_widget_modify_font(browser.name, font);
        pango_font_description_free(font);
        gtk_entry_set_text(GTK_ENTRY(browser.name), suggestedName ? suggestedName : "");
        g_signal_connect(browser.name, "changed", G_CALLBACK(onNameChanged), &browser);
        gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), kindleUnit() / 3, kindleUnit() / 3, kindleUnit() / 2, kindleUnit() / 2);
        gtk_container_add(GTK_CONTAINER(alignment), browser.name);
        gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(vbox), footerNew(&browser, extra ? extraLabel : NULL), FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(browser.window), vbox);

    gtk_widget_show_all(browser.window);
    listDirectory(&browser, directory && *directory ? directory : "/mnt/us");
    gtk_window_move(GTK_WINDOW(browser.window), 0, 0);
    gtk_grab_add(browser.window);

    if (mode == KINDLE_BROWSE_SAVE)
    {
        gtk_widget_grab_focus(browser.name);
        openVirtualKeyboard(NULL, NULL);
    }
    else
    {
        closeVirtualKeyboard();
    }

    g_main_loop_run(browser.loop);

    gtk_grab_remove(browser.window);
    gtk_widget_destroy(browser.window);
    g_main_loop_unref(browser.loop);
    g_ptr_array_free(browser.entries, TRUE);
    g_free(browser.directory);

    if (browser.accepted)
        g_strlcpy(buffer, browser.result, bufferSize);
    g_free(browser.result);
    return browser.accepted;
}

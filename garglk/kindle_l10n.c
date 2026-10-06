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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#if __has_include(<openlipc.h>)
#include <openlipc.h>
#else
#include <lipc.h>
#endif

#include "kindle_l10n.h"

typedef struct
{
    char * msgid;
    char * msgstr;
} KindleMessage;

static GArray * catalog = NULL;

static char * poString(const char * line)
{
    GString * out = g_string_new(NULL);
    const char * p = strchr(line, '"');

    if (p == NULL)
        return g_string_free(out, FALSE);

    for (p++; *p != '\0' && *p != '"'; p++)
    {
        if (*p == '\\' && p[1] != '\0')
        {
            p++;
            switch (*p)
            {
                case 'n': g_string_append_c(out, '\n'); break;
                case 't': g_string_append_c(out, '\t'); break;
                default: g_string_append_c(out, *p); break;
            }
        }
        else
        {
            g_string_append_c(out, *p);
        }
    }

    return g_string_free(out, FALSE);
}

static void addMessage(GString * msgid, GString * msgstr)
{
    if (msgid->len > 0 && msgstr->len > 0)
    {
        KindleMessage message = { g_strdup(msgid->str), g_strdup(msgstr->str) };
        g_array_append_val(catalog, message);
    }
    g_string_truncate(msgid, 0);
    g_string_truncate(msgstr, 0);
}

/* Only the subset of the .po syntax produced by msgmerge is needed:
 * msgid/msgstr pairs with continuation lines; comments, contexts and
 * plural forms are skipped. */
static gboolean loadCatalog(const char * path)
{
    FILE * file = fopen(path, "r");
    char line[1024];
    GString * msgid;
    GString * msgstr;
    GString * field = NULL;

    if (file == NULL)
        return FALSE;

    msgid = g_string_new(NULL);
    msgstr = g_string_new(NULL);

    while (fgets(line, sizeof line, file) != NULL)
    {
        char * value = NULL;

        if (g_str_has_prefix(line, "msgid "))
        {
            addMessage(msgid, msgstr);
            field = msgid;
            value = poString(line);
        }
        else if (g_str_has_prefix(line, "msgstr "))
        {
            field = msgstr;
            value = poString(line);
        }
        else if (line[0] == '"' && field != NULL)
        {
            value = poString(line);
        }
        else
        {
            field = NULL;
        }

        if (value != NULL)
        {
            g_string_append(field, value);
            g_free(value);
        }
    }
    addMessage(msgid, msgstr);

    g_string_free(msgid, TRUE);
    g_string_free(msgstr, TRUE);
    fclose(file);
    return TRUE;
}

static char * kindleLanguage(void)
{
    static const char * variables[] = { "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG" };
    char * language = NULL;
    size_t i;

    for (i = 0; i < G_N_ELEMENTS(variables) && language == NULL; i++)
    {
        const char * value = getenv(variables[i]);
        if (value != NULL && *value != '\0' && strcmp(value, "C") != 0 && strcmp(value, "POSIX") != 0)
            language = g_strndup(value, strcspn(value, ":.@"));
    }

    if (language == NULL)
    {
        LIPC * lipc = LipcOpenNoName();
        char * value = NULL;

        if (lipc != NULL)
        {
            if (LipcGetStringProperty(lipc, "com.lab126.keyboard", "language", &value) == LIPC_OK && value != NULL)
            {
                language = g_strndup(value, strcspn(value, ".@"));
                LipcFreeString(value);
            }
            LipcClose(lipc);
        }
    }

    if (language != NULL)
        g_strdelimit(language, "-", '_');

    return language;
}

static void initCatalog(void)
{
    char exepath[1024] = {0};
    char * language;
    char * directory;
    char * path;

    catalog = g_array_new(FALSE, FALSE, sizeof(KindleMessage));

    language = kindleLanguage();
    if (language == NULL || readlink("/proc/self/exe", exepath, sizeof exepath - 1) <= 0)
    {
        g_free(language);
        return;
    }

    directory = g_path_get_dirname(exepath);
    path = g_strdup_printf("%s/l10n/%s.po", directory, language);

    if (!loadCatalog(path) && strchr(language, '_') != NULL)
    {
        *strchr(language, '_') = '\0';
        g_free(path);
        path = g_strdup_printf("%s/l10n/%s.po", directory, language);
        loadCatalog(path);
    }

    g_free(path);
    g_free(directory);
    g_free(language);
}

const char * kindleTr(const char * msgid)
{
    guint i;

    if (catalog == NULL)
        initCatalog();

    for (i = 0; i < catalog->len; i++)
    {
        KindleMessage * message = &g_array_index(catalog, KindleMessage, i);
        if (strcmp(message->msgid, msgid) == 0)
            return message->msgstr;
    }

    return msgid;
}

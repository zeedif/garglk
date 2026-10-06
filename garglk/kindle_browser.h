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

#ifndef KINDLE_BROWSER_H
#define KINDLE_BROWSER_H

#include "kindle_ui.h"

typedef enum
{
    KINDLE_BROWSE_GAME,
    KINDLE_BROWSE_RESTORE,
    KINDLE_BROWSE_SAVE,
} KindleBrowseMode;

/* Full screen file browser: one paged list with folders first, the path next
 * to an up button, and the selection framed. Returns TRUE and the chosen path
 * in buffer when the user accepts. extraLabel and extra add a button to the
 * left of the footer; the folder is listed again after extra returns. */
gboolean kindleBrowse(KindleBrowseMode mode, const char * directory, const char * suggestedName,
                      const char * extraLabel, KindleCallback extra, gpointer extraData,
                      char * buffer, int bufferSize);

#endif /* KINDLE_BROWSER_H */

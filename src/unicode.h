/*
 *  unicode.h
 *  XNJB
 *
 *  Created by Richard Low on 17/09/2004.
 *
 */

#include "glibdefs.h"

#ifndef UNICODEHEADER_INCLUDED
#define UNICODEHEADER_INCLUDED 1

gchar *g_utf16_to_utf8 (const gunichar2 *str, glong len, glong *items_read, glong *items_written);
gunichar2 *g_utf8_to_utf16 (const gchar *str, glong len, glong *items_read, glong *items_written);

#endif
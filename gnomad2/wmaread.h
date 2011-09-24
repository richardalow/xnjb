/* wmaread.h
   WMA interface, headers
   Copyright (C) 2004 Linus Walleij

This file is part of the GNOMAD package.

GNOMAD is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

You should have received a copy of the GNU General Public License
along with GNOMAD; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA. 

*/

#include "glibdefs.h"

#ifndef WMAHEADER_INCLUDED
#define WMAHEADER_INCLUDED 1

typedef struct 
{
  gchar *artist;
  gchar *title;
  gchar *album;
  guint year;
  gchar *genre;
  guint length;
  guint size;
  gchar *codec;
  guint trackno;
  gboolean protect;
  gchar *filename;
  gchar *path;
} metadata_t;

void get_tag_for_wmafile(metadata_t *meta);

#endif

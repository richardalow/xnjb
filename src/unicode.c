/*
 *  unicode.c
 *  XNJB
 *
 *  Created by Richard Low on 17/09/2004.
 */

/* this has unicode routines taken from glib-2.4.6, mostly
 * from gutf8.c
 *
 * Copyright (C) 1999 Tom Tromey
 * Copyright (C) 2000 Red Hat, Inc.
 */

#include "unicode.h"
#include <stdlib.h>
#include <stdio.h>

int g_unichar_to_utf8 (gunichar c, gchar *outbuf);
gunichar g_utf8_get_char (const gchar *p);
static inline gunichar g_utf8_get_char_extended (const  gchar *p, gssize max_len);

// from glib-2.4.6

#define SURROGATE_VALUE(h,l) (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)
#define UTF8_LENGTH(Char)              \
((Char) < 0x80 ? 1 :                 \
 ((Char) < 0x800 ? 2 :               \
	((Char) < 0x10000 ? 3 :            \
	 ((Char) < 0x200000 ? 4 :          \
		((Char) < 0x4000000 ? 5 : 6)))))

#define UTF8_COMPUTE(Char, Mask, Len)					      \
	if (Char < 128)							      \
	{									      \
		Len = 1;											\
		Mask = 0x7f;							      \
	}									      \
	else if ((Char & 0xe0) == 0xc0)					      \
	{									      \
		Len = 2;								      \
		Mask = 0x1f;							      \
	}									      \
	else if ((Char & 0xf0) == 0xe0)					      \
	{									      \
		Len = 3;								      \
			Mask = 0x0f;							      \
	}									      \
	else if ((Char & 0xf8) == 0xf0)					      \
	{									      \
		Len = 4;								      \
			Mask = 0x07;							      \
	}									      \
	else if ((Char & 0xfc) == 0xf8)					      \
	{									      \
		Len = 5;								      \
			Mask = 0x03;							      \
	}									      \
	else if ((Char & 0xfe) == 0xfc)					      \
	{									      \
		Len = 6;								      \
			Mask = 0x01;							      \
	}									      \
	else									      \
	Len = -1;

#define UTF8_GET(Result, Chars, Count, Mask, Len)			      \
(Result) = (Chars)[0] & (Mask);					      \
for ((Count) = 1; (Count) < (Len); ++(Count))				      \
{									      \
	if (((Chars)[(Count)] & 0xc0) != 0x80)				      \
	{								      \
	  (Result) = -1;						      \
			break;							      \
	}								      \
	(Result) <<= 6;							      \
		(Result) |= ((Chars)[(Count)] & 0x3f);				      \
}

static const gchar utf8_skip_data[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

const gchar * const g_utf8_skip = utf8_skip_data;

#define g_utf8_next_char(p) (char *)((p) + g_utf8_skip[*(guchar *)(p)])

/**
* g_utf16_to_utf8:
 * @str: a UTF-16 encoded string
 * @len: the maximum length of @str to use. If @len < 0, then
 *       the string is terminated with a 0 character.
 * @items_read: location to store number of words read, or %NULL.
 *              If %NULL, then %G_CONVERT_ERROR_PARTIAL_INPUT will be
 *              returned in case @str contains a trailing partial
 *              character. If an error occurs then the index of the
 *              invalid input is stored here.
 * @items_written: location to store number of bytes written, or %NULL.
 *                 The value stored here does not include the trailing
 *                 0 byte.
 *
 * Convert a string from UTF-16 to UTF-8. The result will be
 * terminated with a 0 byte.
 * 
 * Return value: a pointer to a newly allocated UTF-8 string.
 *               This value must be freed with g_free(). If an
 *               error occurs, %NULL will be returned and
 *               @error set.
 **/
gchar *g_utf16_to_utf8 (const gunichar2 *str, glong len, glong *items_read, glong *items_written)
{
  /* This function and g_utf16_to_ucs4 are almost exactly identical - The lines that differ
	* are marked.
	*/
  const gunichar2 *in;
  gchar *out;
  gchar *result = NULL;
  gint n_bytes;
  gunichar high_surrogate;
	
	if (str == NULL)
		return NULL;
	
  n_bytes = 0;
  in = str;
  high_surrogate = 0;
  while ((len < 0 || in - str < len) && *in)
	{
		gunichar2 c = *in;
		gunichar wc;
		
		if (c >= 0xdc00 && c < 0xe000) /* low surrogate */
		{
			if (high_surrogate)
	    {
	      wc = SURROGATE_VALUE (high_surrogate, c);
	      high_surrogate = 0;
	    }
			else
	    {
	      //g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
				//						 _("Invalid sequence in conversion input"));
				printf("Invalid sequence in conversion input");
	      goto err_out;
	    }
		}
		else
		{
			if (high_surrogate)
	    {
	      //g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
				//						 _("Invalid sequence in conversion input"));
				printf("Invalid sequence in conversion input");
	      goto err_out;
	    }
			
			if (c >= 0xd800 && c < 0xdc00) /* high surrogate */
	    {
	      high_surrogate = c;
	      goto next1;
	    }
			else
				wc = c;
		}
		
		/********** DIFFERENT for UTF8/UCS4 **********/
		n_bytes += UTF8_LENGTH (wc);
		
next1:
      in++;
	}
	
  if (high_surrogate && !items_read)
	{
		//g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
		//						 _("Partial character sequence at end of input"));
		printf("Partial character sequence at end of input");
		goto err_out;
	}
  
  /* At this point, everything is valid, and we just need to convert
		*/
  /********** DIFFERENT for UTF8/UCS4 **********/
  result = malloc (n_bytes + 1);
  
  high_surrogate = 0;
  out = result;
  in = str;
  while (out < result + n_bytes)
	{
		gunichar2 c = *in;
		gunichar wc;
		
		if (c >= 0xdc00 && c < 0xe000) /* low surrogate */
		{
			wc = SURROGATE_VALUE (high_surrogate, c);
			high_surrogate = 0;
		}
		else if (c >= 0xd800 && c < 0xdc00) /* high surrogate */
		{
			high_surrogate = c;
			goto next2;
		}
		else
			wc = c;
		
		/********** DIFFERENT for UTF8/UCS4 **********/
		out += g_unichar_to_utf8 (wc, out);
		
next2:
      in++;
	}
  
  /********** DIFFERENT for UTF8/UCS4 **********/
  *out = '\0';
	
  if (items_written)
    /********** DIFFERENT for UTF8/UCS4 **********/
    *items_written = out - result;
	
err_out:
		if (items_read)
			*items_read = in - str;
	
  return result;
}

/**
* g_utf8_to_utf16:
 * @str: a UTF-8 encoded string
 * @len: the maximum length of @str to use. If @len < 0, then
 *       the string is nul-terminated.
 * @items_read: location to store number of bytes read, or %NULL.
 *              If %NULL, then %G_CONVERT_ERROR_PARTIAL_INPUT will be
 *              returned in case @str contains a trailing partial
 *              character. If an error occurs then the index of the
 *              invalid input is stored here.
 * @items_written: location to store number of words written, or %NULL.
 *                 The value stored here does not include the trailing
 *                 0 word.
 *
 * Convert a string from UTF-8 to UTF-16. A 0 word will be
 * added to the result after the converted text.
 * 
 * Return value: a pointer to a newly allocated UTF-16 string.
 *               This value must be freed with g_free(). If an
 *               error occurs, %NULL will be returned and
 *               @error set.
 **/
gunichar2 *g_utf8_to_utf16 (const gchar *str, glong len, glong *items_read, glong *items_written)
{
  gunichar2 *result = NULL;
  gint n16;
  const gchar *in;
  gint i;
	
	if (str == NULL)
		return NULL;
	
  in = str;
  n16 = 0;
  while ((len < 0 || str + len - in > 0) && *in)
	{
		gunichar wc = g_utf8_get_char_extended (in, str + len - in);
		if (wc & 0x80000000)
		{
			if (wc == (gunichar)-2)
	    {
	      if (items_read)
					break;
	      else
				//	g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
				//							 _("Partial character sequence at end of input"));
					printf("Partial character sequence at end of input");
	    }
			else
				//g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
				//						 _("Partial character sequence at end of input"));
				printf("Partial character sequence at end of input");
			goto err_out;
		}
		
		if (wc < 0xd800)
			n16 += 1;
		else if (wc < 0xe000)
		{
			//g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
			//						 _("Invalid sequence in conversion input"));
			printf("Invalid sequence in conversion input");
			goto err_out;
		}
		else if (wc < 0x10000)
			n16 += 1;
		else if (wc < 0x110000)
			n16 += 2;
		else
		{
			//g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
			//						 _("Character out of range for UTF-16"));
			printf("Character out of range for UTF-16");
			goto err_out;
		}
		
		in = g_utf8_next_char (in);
	}
	
  result = malloc(sizeof(gunichar2) * (n16 + 1));
  
  in = str;
  for (i = 0; i < n16;)
	{
		gunichar wc = g_utf8_get_char (in);
		
		if (wc < 0x10000)
		{
			result[i++] = wc;
		}
		else
		{
			result[i++] = (wc - 0x10000) / 0x400 + 0xd800;
			result[i++] = (wc - 0x10000) % 0x400 + 0xdc00;
		}
		
		in = g_utf8_next_char (in);
	}
	
  result[i] = 0;
	
  if (items_written)
    *items_written = n16;
	
err_out:
		if (items_read)
			*items_read = in - str;
  
  return result;
}

/**
* g_unichar_to_utf8:
 * @c: a ISO10646 character code
 * @outbuf: output buffer, must have at least 6 bytes of space.
 *       If %NULL, the length will be computed and returned
 *       and nothing will be written to @outbuf.
 * 
 * Converts a single character to UTF-8.
 * 
 * Return value: number of bytes written
 **/
int g_unichar_to_utf8 (gunichar c, gchar *outbuf)
{
  guint len = 0;    
  int first;
  int i;
	
  if (c < 0x80)
	{
		first = 0;
		len = 1;
	}
  else if (c < 0x800)
	{
		first = 0xc0;
		len = 2;
	}
  else if (c < 0x10000)
	{
		first = 0xe0;
		len = 3;
	}
	else if (c < 0x200000)
	{
		first = 0xf0;
		len = 4;
	}
  else if (c < 0x4000000)
	{
		first = 0xf8;
		len = 5;
	}
  else
	{
		first = 0xfc;
		len = 6;
	}
	
  if (outbuf)
	{
		for (i = len - 1; i > 0; --i)
		{
			outbuf[i] = (c & 0x3f) | 0x80;
			c >>= 6;
		}
		outbuf[0] = c | first;
	}
	
  return len;
}

/**
* g_utf8_get_char:
 * @p: a pointer to Unicode character encoded as UTF-8
 * 
 * Converts a sequence of bytes encoded as UTF-8 to a Unicode character.
 * If @p does not point to a valid UTF-8 encoded character, results are
 * undefined. If you are not sure that the bytes are complete
 * valid Unicode characters, you should use g_utf8_get_char_validated()
 * instead.
 * 
 * Return value: the resulting character
 **/
gunichar g_utf8_get_char (const gchar *p)
{
  int i, mask = 0, len;
  gunichar result;
  unsigned char c = (unsigned char) *p;
	
  UTF8_COMPUTE (c, mask, len);
  if (len == -1)
    return (gunichar)-1;
  UTF8_GET (result, p, i, mask, len);
	
  return result;
}

/* Like g_utf8_get_char, but take a maximum length
* and return (gunichar)-2 on incomplete trailing character
*/
static inline gunichar g_utf8_get_char_extended (const  gchar *p, gssize max_len)  
{
  guint i, len;
  gunichar wc = (guchar) *p;
	
  if (wc < 0x80)
	{
		return wc;
	}
  else if (wc < 0xc0)
	{
		return (gunichar)-1;
	}
  else if (wc < 0xe0)
	{
		len = 2;
		wc &= 0x1f;
	}
  else if (wc < 0xf0)
	{
		len = 3;
		wc &= 0x0f;
	}
  else if (wc < 0xf8)
	{
		len = 4;
		wc &= 0x07;
	}
  else if (wc < 0xfc)
	{
		len = 5;
		wc &= 0x03;
	}
  else if (wc < 0xfe)
	{
		len = 6;
		wc &= 0x01;
	}
  else
	{
		return (gunichar)-1;
	}
  
  if (max_len >= 0 && len > max_len)
	{
		for (i = 1; i < max_len; i++)
		{
			if ((((guchar *)p)[i] & 0xc0) != 0x80)
				return (gunichar)-1;
		}
		return (gunichar)-2;
	}
	
  for (i = 1; i < len; ++i)
	{
		gunichar ch = ((guchar *)p)[i];
		
		if ((ch & 0xc0) != 0x80)
		{
			if (ch)
				return (gunichar)-1;
			else
				return (gunichar)-2;
		}
		
		wc <<= 6;
		wc |= (ch & 0x3f);
	}
	
  if (UTF8_LENGTH(wc) != len)
    return (gunichar)-1;
  
  return wc;
}
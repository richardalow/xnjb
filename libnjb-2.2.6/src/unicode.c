/**
 * \file unicode.c
 *
 * This file contains general Unicode string manipulation functions.
 * It mainly consist of functions for converting between UCS-2 (used on
 * the devices), UTF-8 (used by several applications) and 
 * ISO 8859-1 / Codepage 1252 (fallback).
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "libnjb.h"
#include "protocol.h"
#include "protocol3.h"
#include "unicode.h"
#include "njb_error.h"
#include "usb_io.h"
#include "ioutil.h"
#include "defs.h"
#include "base.h"

extern int __sub_depth;
int njb_unicode_flag = NJB_UC_8859;
#define MAX_STRING_LENGTH 512

/**
 * This flag determines whether to use ISO 8859-1 / codepage 1252 
 * (default) or unicode UTF-8 for ALL strings sent into and out of
 * libnjb, for ALL sessions and devices.
 *
 * @param flag 0 for ISO 8859-1 / codepage 1252 or 1 for Unicode
 *             UTF-8.
 */
void njb_set_unicode (int flag)
{
	njb_unicode_flag = flag;
}

/** 
 * Gets the length (in characters, not bytes) of a unicode 
 * UCS-2 string, eg a string which physically is 0x00 0x41 0x00 0x00
 * will return a value of 1.
 *
 * @param unicstr a UCS-2 Unicode string
 * @return the length of the string, in number of characters. If you 
 *         want to know the length in bytes, multiply this by two and
 *         add two (for zero terminator).
 */
int ucs2strlen(const unsigned char *unicstr){

	__dsub= "ucs2strlen";

	int length=0;
	int i;

	__enter;

	/* Unicode strings are terminated with 2 * 0x00 */
	for(i=0; (unicstr[i] | unicstr[i+1])!='\0'; i+=2) {
		length++;
	}

	__leave;
	return length;
}

/**
 * This routine returns the length in bytes that this
 * UCS-2 string would occupy if encoded as UTF-8
 *
 * @param unicstr the Unicode UCS-2 string to analyze
 * @return the number of bytes this string would occupy
 *         in UTF-8
 */
static int ucs2utf8len(const unsigned char *unicstr){
  int length=0;
  int i;
  for(i=0; (unicstr[i] | unicstr[i+1]) != '\0'; i+=2) {
    if (unicstr[i] == 0x00 && unicstr[i+1] < 0x80)
      length++;
    else if (unicstr[i] < 0x08)
      length+=2;
    else
      length+=3;
  }
  return length;
}

/** 
 * Create a new, allocated UCS-2 string that is a copy
 * of the parameter
 *
 * @param unicstr the UCS-2 string to copy
 * @return a newly allocated copy of the string
 */
static unsigned char *ucs2strdup(const unsigned char *unicstr) {
  int length = ucs2strlen(unicstr);
  unsigned char *data;
  
  data = (unsigned char *) malloc(length*2+2);
  if ( data == NULL ) {
    return NULL;
  }
  memcpy(data, unicstr, length*2+2);
  return data;
}

/**
 * This function converts an ordinary ISO 8859-1 string
 * to a unicode UTF-8 string
 *
 * @param str the ISO 8859-1 string to convert
 * @return a newly allocated UTF-8 encoded string with 
 *         the same content. Should be freed after use.
 */
char *strtoutf8(const unsigned char *str) {  
  unsigned char buffer[MAX_STRING_LENGTH];
  int l = 0;
  int i;

  memset(buffer,0,MAX_STRING_LENGTH);
  
  for (i = 0; i < strlen((char *) str); i++) {
    if (str[i]<0x80) {
      buffer[l] = str[i];
      l++;
    } else {
      buffer[l] = 0xC0 | (str[i]>>6 & 0x03);
      buffer[l+1] = 0x80 | (str[i] & 0x3F);
      l+=2;
    }
    buffer[l] = 0x00;
  }
  /* The duplicate the string and return it */
  return strdup((char *) buffer);
}

/**
 * This function approximates an ISO 8859-1 string from
 * a UTF-8 string, leaving out untranslatable characters
 *
 * @param str the UTF-8 string to use as indata
 * @return a newly allocated ISO 8859-1 string which is
 *         as close a possible to the UTF-8 string.
 */
char *utf8tostr(const unsigned char *str) {
  unsigned char buffer[MAX_STRING_LENGTH];
  unsigned char *ucs2string;
  int i = 0;
  int l = 0;

  memset(buffer,0,MAX_STRING_LENGTH);

  ucs2string = strtoucs2(str);

  for(i=0; (ucs2string[i] | ucs2string[i+1])!='\0'; i+=2) {
    if (ucs2string[i] == '\0') {
      buffer[l] = ucs2string[i+1];
      l++;
    }
  }
  buffer[l] = '\0';

  free(ucs2string);

  /* If there was nothing in this string, return NULL */
  if (l>0 || i == 0)
    return strdup((char *) buffer);
  else
    return NULL;
}

/**
 * Converts a Unicode UCS-2 2-byte string to a common 
 * ISO 8859-1 string quick and dirty (japanese unicodes etc, 
 * that use all 16 bits will fail miserably)
 *
 * @param unicstr the UCS-2 unicode string to convert
 * @return a newly allocated ISO 8859-1 string that tries
 *         to resemble the UCS-2 string
 */
char *ucs2tostr(const unsigned char *unicstr){

	__dsub= "ucs2tostr";

	char *data = NULL;
	int i = 0;
	int l = 0;

	__enter;


	/* Real unicode support in UTF8 */
	if (njb_unicode_flag == NJB_UC_UTF8) {
	  int length8 = ucs2utf8len(unicstr);
	  data= (char *) malloc(length8+1);
	  if ( data == NULL ) {
	    __leave;
	    return NULL;
	  }
	  for(l=0;(unicstr[l] | unicstr[l+1])!='\0'; l+=2) {
	    if (unicstr[l] == 0x00 && unicstr[l+1] < 0x80) {
	      data[i]=unicstr[l+1];
	      i++;
	    } else if (unicstr[l] < 0x08) {
	      data[i] = 0xc0 | (unicstr[l]<<2 & 0x1C) | (unicstr[l+1]>>6  & 0x03);
	      data[i+1] = 0x80 | (unicstr[l+1] & 0x3F);
	      i+=2;
	    } else {
	      data[i] = 0xe0 | (unicstr[l]>>4 & 0x0F);
	      data[i+1] = 0x80 | (unicstr[l]<<2 & 0x3C) | (unicstr[l+1]>>6 & 0x03);
	      data[i+2] = 0x80 | (unicstr[l+1] & 0x3F);
	      i+=3;
	    }
	  }
	  /* Terminate string */
	  data[i]=0x00;	  
	} else {
	  /* If we're running in ISO 8859-1 mode, approximate
	   * and concatenate, loosing any chars above 0xff */
	  int length=ucs2strlen(unicstr);

	  data = (char *) malloc(length+1);
	  if ( data == NULL ) {
	    __leave;
	    return NULL;
	  }

	  l = 0;
	  for(i=0;l<length*2;){
	    if (unicstr[l] == 0x00) {
	      data[i]=unicstr[l+1];
	      i++;
	    }
	    l+=2;
	  }
	  /* Terminate string */
	  data[i]=0x00;
	}


	__leave;
	return data;
}

/**
 * Convert a simple ISO 8859-1 or a Unicode
 * UTF8 string (depending on library Unicode flag) to a 
 * unicode UCS-2 string.
 *
 * @param str the ISO 8859-1 or UTF-8 string to conver
 * @return a pointer to a newly allocated UCS-2 string
 */
unsigned char *strtoucs2(const unsigned char *str) {

	__dsub= "strtoucs2";

	unsigned char *data = NULL;
	int i=0;
	int l=0;

	__enter;

	/* Real unicode support in UTF8 */
	if (njb_unicode_flag == NJB_UC_UTF8) {
	  unsigned char buffer[MAX_STRING_LENGTH*2];

	  int length=0;
	  int i;

	  for(i=0; str[i] != '\0';) {
	    if (str[i] < 0x80) {
	      buffer[length] = 0x00;
	      buffer[length+1] = str[i];
	      length += 2;
	      i++;
	    } else {
	      unsigned char numbytes = 0;
	      unsigned char lenbyte = 0;
	      
	      /* Read the number of encoded bytes */
	      lenbyte = str[i];
	      while (lenbyte & 0x80) {
		numbytes++;
		lenbyte = lenbyte<<1;
	      }
	      /* UCS-2 can handle no more than 3 UTF-8 encoded bytes */
	      if (numbytes <= 3) {
		if (numbytes == 2 && str[i+1] >= 0x80) {
		  /* This character can always be handled correctly */
		  buffer[length] = (str[i]>>2 & 0x07);
		  buffer[length+1] = (str[i]<<6 & 0xC0) | (str[i+1] & 0x3F);
		  i += 2;
		  length += 2;
		} else if (numbytes == 3 && str[i+1] >= 0x80 && str[i+2] >= 0x80) {
		  buffer[length] = (str[i]<<4 & 0xF0) | (str[i+1]>>2 & 0x0F);
		  buffer[length+1]= (str[i+1]<<6 & 0xC0) | (str[i+2] & 0x3F);
		  i += 3;
		  length += 2;
		} else {
		  /* Abnormal string character, just skip */
		  i += numbytes;
		}
	      } else {
		/* Just skip that character */
		i += numbytes;
	      }
	    }
	  }
	  /* Copy the buffer contents */
	  buffer[length] = 0x00;
	  buffer[length+1] = 0x00;
	  data = ucs2strdup(buffer);
	  if (data == NULL) {
	    __leave;
	    return NULL;
	  }
	} else {
	  /* If we're running in ISO 8859-1 mode, approximate
	   * and concatenate, loosing any chars above 0xff */
	  data = (unsigned char *) malloc(2 * strlen((char *) str) + 2);
	  if ( data == NULL ) {
	    __leave;
	    return NULL;
	  }
	
	  for(i = 0; i <= strlen((char *) str); i++){
	    data[l] = 0x00;
	    data[l+1] = str[i];
	    l += 2;
	  }
	}

	__leave;
	return data;
}

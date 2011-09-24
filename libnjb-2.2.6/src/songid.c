/**
 * \file songid.c
 *
 * This file contains the functions that are used for manipulating
 * all song/track metadata, such as artist, title etc.
 */

#include <stdlib.h>
#include <string.h>
#include "libnjb.h"
#include "base.h"
#include "njb_error.h"
#include "defs.h"
#include "protocol3.h"
#include "byteorder.h"
#include "unicode.h"
#include "songid.h"

/*
 * These unicode tags appeared in a users NJB1 jukebox,
 * we still don't know what program that writes them, and
 * we do not support writing them, but if they're present,
 * they are recognized and used.
 */
#define UNICODE_ON_NJB1 /**< We want to use Unicode also on NJB1 */
#define FR_UNI_TITLE	"UNI_TITLE" /**< Unicode Title metadata for NJB1 */
#define FR_UNI_ALBUM	"UNI_ALBUM" /**< Unicode Album metadata for NJB1 */
#define FR_UNI_GENRE	"UNI_GENRE" /**< Unicode Genre metadata for NJB1 */
#define FR_UNI_ARTIST	"UNI_ARTIST" /**< Unicode Artist metadata for NJB1 */
/* Haven't seen this one being used, but add it anyway. */
#define FR_UNI_FNAME	"UNI_FNAME" /**< Unicode Filename metadata for NJB1 (not used) */

extern int __sub_depth;
extern int njb_unicode_flag; /**< A flag for if unicode is used or not (global) */

/**
 * This creates a new song ID holder structure. (A songid in turn
 * contains several frames represening different metadata.)
 *
 * @return a new song ID structure
 */
njb_songid_t *NJB_Songid_New(void)
{
  __dsub= "NJB_Songid_New";
  njb_songid_t *song;
  
  song= (njb_songid_t *) malloc (sizeof(njb_songid_t));
  if ( song == NULL ) {
    return NULL;
  }
  
  memset(song, 0, sizeof(njb_songid_t));
  song->next = NULL;
  
  return song;
}

/**
 * This takes a raw chunk of memory representing a song ID as 
 * used by the NJB1 and convert it into a song ID structure as 
 * used by libnjb.
 *
 * @param data the raw bytes from the NJB1
 * @param nbytes the size of the NJB1 byte array
 * @return a newly allocated song ID that shall be freed by
 *         the caller after use
 */
njb_songid_t *songid_unpack (void *data, size_t nbytes)
{
  __dsub= "songid_unpack";
  unsigned char *dp= (unsigned char *) data;
  size_t index;
  njb_songid_t *song;
  u_int16_t i, nframes;
  /* The unicode tags shall take precedence */
  int had_uni_title = 0;
  int had_uni_album = 0;
  int had_uni_genre = 0;
  int had_uni_artist = 0;
  int had_uni_fname = 0;
  
  song = NJB_Songid_New();
  if ( song == NULL ) return NULL;
  
  nframes = njb1_bytes_to_16bit(&dp[0]);
  
  index = 2;
  
  /* Loop through the frames of a single song ID */
  for (i = 0; i < nframes; i++) {
    /* Each frame has a label, a value and a type */
    u_int16_t lsize, vsize, type;
    char *label = NULL;
    char *value = NULL;
    njb_songid_frame_t *frame = NULL;
    
    type = njb1_bytes_to_16bit(&dp[index]);
    lsize = njb1_bytes_to_16bit(&dp[index+2]);
    vsize = njb1_bytes_to_16bit(&dp[index+4]);
    index += 8;
    label = (char *) &dp[index];
    value = (char *) &dp[index+lsize];
    
    /* Handle Unicode conversion on ASCII fields, or default
     * to ISO 8859-1 */
    if (type == ID_DATA_ASCII) {
      if (njb_unicode_flag == NJB_UC_UTF8) {
	if ( (!strcmp(label, FR_TITLE) && had_uni_title) ||
	     (!strcmp(label, FR_ALBUM) && had_uni_album) ||
	     (!strcmp(label, FR_GENRE) && had_uni_genre) ||
	     (!strcmp(label, FR_ARTIST) && had_uni_artist) ||
	     (!strcmp(label, FR_FNAME) && had_uni_fname) ) {
	  frame = NULL;
	} else {
	  char *utf8str = NULL;
	  
	  utf8str = strtoutf8((unsigned char *) value);
	  if (utf8str == NULL) {
	    NJB_Songid_Destroy(song);
	    return NULL;
	  }
	  frame = NJB_Songid_Frame_New_String(label, utf8str);
	  free(utf8str);
	}
      } else {
	/* No Unicode conversion, default by treating as ISO 8859-1 */
	/* Autoconvert erroneous Year tag */
	if (!strcmp(label, FR_YEAR)) {
	  u_int16_t tmpyear = strtoul(value, NULL, 10);
	  frame = NJB_Songid_Frame_New_Year(tmpyear);
	} else if (!strcmp(label, FR_TRACK)) {
	  u_int16_t tmptrack = strtoul(value, NULL, 10);
	  frame = NJB_Songid_Frame_New_Tracknum(tmptrack);
	}
	frame = NJB_Songid_Frame_New_String(label, value);
      }
      /*
       * The Unicode labels appeared in later software, and shall take
       * precedence if present.
       */
    } else if (type == ID_DATA_UNI) {
      unsigned char *clone; /* Needed because of NJB1 byteorder */
      char *utf8str = NULL;
      u_int16_t i;
      
      /* Switch byteorder on the string, we use bigendian internally */
      clone = malloc(vsize);
      for (i = 0; i < vsize; i+=2) {
	clone[i] = value[i+1];
	clone[i+1] = value[i];
      }
      utf8str = ucs2tostr(clone);
      free(clone);
      /* printf("Found unicode frame: %s, content %s\n", label, utf8str); */
      /*
       * After converting the Unicode frame into an UTF8 ASCII 
       * frame, we use the standard labels to simplify the API.
       */
      if (!strcmp(label, FR_UNI_TITLE)) {
	frame = NJB_Songid_Frame_New_Title(utf8str);
	had_uni_title = 1;
      } else if (!strcmp(label, FR_UNI_ALBUM)) {
	frame = NJB_Songid_Frame_New_Album(utf8str);
	had_uni_album = 1;
      } else if (!strcmp(label, FR_UNI_GENRE)) {
	frame = NJB_Songid_Frame_New_Genre(utf8str);
	had_uni_genre = 1;
      } else if (!strcmp(label, FR_UNI_ARTIST)) {
	frame = NJB_Songid_Frame_New_Artist(utf8str);
	had_uni_artist = 1;
      } else if (!strcmp(label, FR_UNI_FNAME)) {
	frame = NJB_Songid_Frame_New_Filename(utf8str);
	had_uni_fname = 1;
      }
      free(utf8str);
    } else { /* This means it is a numeric value */
      /* Depending on value size we construct different frames */
      if (vsize == 2) {
	/* These are unlikely. NJB1 don't have them much... */
	u_int16_t dummy = njb1_bytes_to_16bit((unsigned char *) value);
	printf("LIBNJB confusion: a NJB1 device listed a 16 bit integer for field: %s\n", label);
	frame = NJB_Songid_Frame_New_Uint16(label, dummy);
      } else if (vsize == 4) {
	if (!strcmp(label, FR_YEAR) || !strcmp(label, FR_LENGTH) || !strcmp(label, FR_TRACK)) {
	  u_int16_t dummy = njb1_bytes_to_32bit((unsigned char *) value);
	  /* 
	   * Length, track no and year:
	   * We don't process these as 32 bit attributes, 
	   * it is not needed, and it is not supported.
	   */
	  frame = NJB_Songid_Frame_New_Uint16(label, dummy);
	} else {
	  u_int32_t dummy = njb1_bytes_to_32bit((unsigned char *) value);
	  /* Size and track ID (e.g.) is really 32-bit */
	  frame = NJB_Songid_Frame_New_Uint32(label, dummy); 
	}
      } else {
	/* We don't know what this is... */
	printf("LIBNJB panic: unknown data format (%d bytes) when unpacking frame %s!\n", 
	       vsize, label);
      }
    }
    /*
     * The frame will only be NULL if there was a Unicode frame
     * taking precedence over the ISO 8859-1 frame.
     */
    if (frame != NULL) {
      NJB_Songid_Addframe(song, frame);
    }
    
    index += (lsize + vsize);
    
    if ( index > nbytes ) {
      NJB_Songid_Destroy(song);
      return NULL;
    }
  }
  
  return song;
}

/**
 * This packs (serialize, marshal) a libnjb song ID into the raw 
 * byte structure used by the NJB1.
 *
 * @param song the libnjb song ID to pack
 * @param tagsize a pointer to an integer that will hold the size of 
 *                the resulting NJB1 songid structure
 * @return a newly allocated byte array holding the NJB1 representation
 *         of the song ID, the memory should be freed by the caller after
 *         use
 */
unsigned char *songid_pack (njb_songid_t *song, u_int32_t *tagsize)
{
  __dsub= "songid_pack";
  njb_songid_frame_t *frame;
  unsigned char tagbuffer[1024];
  unsigned char *data;
  u_int16_t nframes = 0;
  size_t index;
  
  *tagsize = 0;
  
  if ( !song->nframes ) {
    /* The song has to have frames */
    return NULL;
  }
  
  index = 2;
  
  NJB_Songid_Reset_Getframe(song);
  while ((frame = NJB_Songid_Getframe(song))) {
    char *label = strdup(frame->label);
    u_int16_t labelsz = strlen(label) + 1;
    
    /* ASCII frames such as filename, artist, album... */
    if (frame->type ==  NJB_TYPE_STRING) {
      char *frame_ascii_content = NULL;
      u_int16_t frame_ascii_len = 0;

#ifdef UNICODE_ON_NJB1
      /* Convert ASCII fields to ISO 8859-1 as is used on NJB 1 */
      if(njb_unicode_flag == NJB_UC_UTF8) {
	/* First add UNI_ strings if unicode is used */
	char *frame_unicode_label = NULL;
	
	if (!strcmp(frame->label, FR_TITLE)) {
	  frame_unicode_label = strdup(FR_UNI_TITLE);
	} else if (!strcmp(frame->label, FR_ALBUM)) {
	  frame_unicode_label = strdup(FR_UNI_ALBUM);
	} else if (!strcmp(frame->label, FR_GENRE)) {
	  frame_unicode_label = strdup(FR_UNI_GENRE);
	} else if (!strcmp(frame->label, FR_ARTIST)) {
	  frame_unicode_label = strdup(FR_UNI_ARTIST);
	}
	
	/* If it was something unicodifiable, add it */
	if (frame_unicode_label != NULL) {
	  /* Add type and size for this frame */
	  u_int16_t frame_unicode_label_len = strlen(frame_unicode_label)+1;
	  unsigned char* frame_unicode_content = strtoucs2((unsigned char *) frame->data.strval);
	  u_int16_t frame_unicode_content_len = 2*ucs2strlen(frame_unicode_content) + 2;
	  u_int16_t i;
	  
	  /* Switch around the byteorder for NJB1 */
	  for (i = 0; i < frame_unicode_content_len; i+= 2) {
	    unsigned char tmp;
	    
	    tmp = frame_unicode_content[i+1];
	    frame_unicode_content[i+1] = frame_unicode_content[i];
	    frame_unicode_content[i] = tmp;
	  }
	  from_16bit_to_njb1_bytes(ID_DATA_UNI, &tagbuffer[index]);
	  from_16bit_to_njb1_bytes(frame_unicode_label_len, &tagbuffer[index+2]);
	  from_16bit_to_njb1_bytes(frame_unicode_content_len, &tagbuffer[index+4]);
	  from_16bit_to_njb1_bytes(0x0000U, &tagbuffer[index+6]);
	  index += 8;
	  /* Copy the label */
	  memcpy(&tagbuffer[index], frame_unicode_label, frame_unicode_label_len);
	  index += frame_unicode_label_len;
	  /* Copy the content */
	  memcpy(&tagbuffer[index], frame_unicode_content, frame_unicode_content_len);
	  index += frame_unicode_content_len;
	  /* Free dummy variables */
	  free(frame_unicode_label);
	  free(frame_unicode_content);
	  nframes ++;
	}
	/* Then also add the ASCII version */
	frame_ascii_content = utf8tostr((unsigned char *) frame->data.strval);
      } else {
	frame_ascii_content = strdup(frame->data.strval);
      }
#else
      if(njb_unicode_flag == NJB_UC_UTF8) {
	frame_ascii_content = utf8tostr(frame->data.strval);
      } else {
	frame_ascii_content = strdup(frame->data.strval);
      }
#endif
      if (frame_ascii_content == NULL) {
	return NULL;
      }
      
      /* Add type and size for this frame */
      from_16bit_to_njb1_bytes(ID_DATA_ASCII, &tagbuffer[index]);
      from_16bit_to_njb1_bytes(labelsz, &tagbuffer[index+2]);
      frame_ascii_len = strlen(frame_ascii_content)+1;
      from_16bit_to_njb1_bytes(frame_ascii_len, &tagbuffer[index+4]);
      from_16bit_to_njb1_bytes(0x0000U, &tagbuffer[index+6]);
      index += 8;
      memcpy(&tagbuffer[index], label, labelsz);
      index += labelsz;
      memcpy(&tagbuffer[index], frame_ascii_content, frame_ascii_len);
      free(frame_ascii_content);
      index += frame_ascii_len;
      nframes ++;
      
      /* Numerical tags such as year or length */
    } else if (frame->type == NJB_TYPE_UINT16) {
      /* Add type and size for this frame */
      // NJB1 does not have 16 bit integers!
      from_16bit_to_njb1_bytes(ID_DATA_BIN, &tagbuffer[index]);
      from_16bit_to_njb1_bytes(labelsz, &tagbuffer[index+2]);
      from_16bit_to_njb1_bytes(4, &tagbuffer[index+4]); // So always use 4 here
      from_16bit_to_njb1_bytes(0x0000U, &tagbuffer[index+6]);
      index += 8;
      memcpy(&tagbuffer[index], label, labelsz);
      index += labelsz;
      from_32bit_to_njb1_bytes(frame->data.u_int16_val, &tagbuffer[index]); // was 16
      index += 4; // was 2
      nframes ++;
    } else if (frame->type == NJB_TYPE_UINT32) {
      /* Add type and size for this frame */
      from_16bit_to_njb1_bytes(ID_DATA_BIN, &tagbuffer[index]);
      from_16bit_to_njb1_bytes(labelsz, &tagbuffer[index+2]);
      from_16bit_to_njb1_bytes(4, &tagbuffer[index+4]);
      from_16bit_to_njb1_bytes(0x0000U, &tagbuffer[index+6]);
      index += 8;
      memcpy(&tagbuffer[index], label, labelsz);
      index += labelsz;
      from_32bit_to_njb1_bytes(frame->data.u_int32_val, &tagbuffer[index]);
      index += 4;
      nframes ++;
    } else {
      printf("LIBNJB panic: unknown frametype of \"%s\" when packing frames!\n",
	     label);
      /* Note: nframes not increased */
    }
    /* Release temporary label copy */
    free(label);
  }
  from_16bit_to_njb1_bytes(nframes, &tagbuffer[0]);
  *tagsize = index;
  if (*tagsize == 0) {
    return NULL;
  }
  /* Duplicate the buffer and return */
  data = (unsigned char *) malloc (*tagsize);
  if ( data == NULL ) {
    return NULL;
  }
  memcpy(data, tagbuffer, *tagsize);
  
  return data;
}

/**
 * Helper function for taking a song ID frame and converting it to
 * an unsigned 32 bit integer, regardless of the numeric format
 * (32 bit or 16 bit) of the source frame.
 *
 * @param frame the libnjb song ID frame to convert to a 32 bit
 *        integer
 * @return the 32 bit integer contained in this frame
 */
static u_int32_t valtoint32(njb_songid_frame_t *frame)
{
  u_int32_t retval = 0;

  if (frame->type == NJB_TYPE_UINT16) {
    u_int16_t dummy = frame->data.u_int16_val;
    retval = (u_int32_t) dummy;
  } else if (frame->type == NJB_TYPE_UINT32) {
    u_int32_t dummy = frame->data.u_int32_val;
    retval = dummy;
  } else {
    printf("LIBNJB panic: could not convert weird numeric format to 32 bit unsigned integer!\n");
  }
  return retval;
}

/**
 * Helper function that adds a UCS2 unicode string to a memory position
 *
 * @param data a pointer to the raw memory byte array that shall hold the UCS2 string
 * @param datap a index pointer into the same array that points to the current
 *              location (where the string shall be copied) and which will be
 *              modified to point to the next free byte after this function has
 *              been called
 * @param tagtype an identifier for the type of tag, prepended to the string as
 *                16 bits
 * @param unistr the string to add
 */
static void add_bin_unistr(unsigned char *data, u_int32_t *datap, u_int16_t tagtype, unsigned char *unistr)
{
  u_int32_t binlen;

  binlen = ucs2strlen(unistr) * 2 + 2;
  from_16bit_to_njb3_bytes(binlen+2, &data[*datap]);
  *datap += 2;
  from_16bit_to_njb3_bytes(tagtype, &data[*datap]);
  *datap += 2;
  memcpy(data+(*datap), unistr, binlen);
  *datap += binlen;
}

/**
 * This packs (serialize, marshal) a libnjb song ID into the raw 
 * byte structure used by the series 3 devices.
 *
 * @param song the libnjb song ID to pack
 * @param tagsize a pointer to an integer that will hold the size of 
 *                the resulting series 3 songid structure
 * @return a newly allocated byte array holding the series 3 representation
 *         of the song ID, the memory should be freed by the caller after
 *         use
 */
unsigned char *songid_pack3 (njb_songid_t *song, u_int32_t *tagsize)
{
  __dsub= "songid_pack3";
  njb_songid_frame_t *frame;
  unsigned char *data = NULL;
  u_int32_t datap = 0;
  u_int32_t track_size = 0x00000000U;
  u_int16_t track_year = 0x0000U;
  u_int16_t track_number = 0x0000U;
  u_int16_t track_length = 0x0000U;
  u_int16_t track_protected = 0x0000U;
  u_int16_t track_codec = NJB3_CODEC_MP3_ID;
  unsigned char *track_title = NULL;
  unsigned char *track_album = NULL;
  unsigned char *track_artist = NULL;
  unsigned char *track_genre = NULL;
  unsigned char *track_fname = NULL;
  unsigned char *track_folder = NULL;
  u_int8_t had_size = 0;
  u_int8_t had_year = 0;
  u_int8_t had_trackno = 0;
  u_int8_t had_length = 0;
  u_int8_t had_codec = 0;
  
  *tagsize = 0;
  
  if ( !song->nframes ) {
    return NULL;
  }
  
  NJB_Songid_Reset_Getframe(song);
  
  while ( (frame = NJB_Songid_Getframe(song)) ) {
    if (!strcmp(frame->label, FR_SIZE)) {
      /* 8 bytes, 2 length 2 tagid 4 bytes size */
      track_size = valtoint32(frame);
      *tagsize += 8;
      had_size = 1;
    }
    else if (!strcmp(frame->label, FR_YEAR)) {
      /* 6 bytes, 2 length 2 tagid 2bytes year */
      track_year = frame->data.u_int16_val;
      *tagsize += 6;
      had_year = 1;
    }
    else if (!strcmp(frame->label, FR_TRACK)) {
      /* 6 bytes, 2 length 2 tagid 2bytes trackno */
      track_number = frame->data.u_int16_val;
      *tagsize += 6;
      had_trackno = 1;
    }
    else if (!strcmp(frame->label, FR_LENGTH)) {
      /* 6 bytes, 2 length 2 tagid 2bytes length */
      track_length = frame->data.u_int16_val;
      *tagsize += 6;
      had_length = 1;
    }
    /* For strings, stringlengh * 2 + 4 bytes */
    else if (!strcmp(frame->label, FR_TITLE)) {
      track_title = strtoucs2((unsigned char *) frame->data.strval);
      *tagsize += (6 + 2*ucs2strlen(track_title));
    }
    else if (!strcmp(frame->label, FR_ALBUM)) {
      track_album = strtoucs2((unsigned char *) frame->data.strval);
      *tagsize += (6 + 2*ucs2strlen(track_album));
    }
    else if (!strcmp(frame->label, FR_ARTIST)) {
      track_artist = strtoucs2((unsigned char *) frame->data.strval);
      *tagsize += (6 + 2*ucs2strlen(track_artist));
    }
    else if (!strcmp(frame->label, FR_GENRE)) {
      track_genre = strtoucs2((unsigned char *) frame->data.strval);
      *tagsize += (6 + 2*ucs2strlen(track_genre));
    }
    else if (!strcmp(frame->label, FR_FNAME)) {
      track_fname = strtoucs2((unsigned char *) frame->data.strval);
      *tagsize += (6 + 2*ucs2strlen(track_fname));
    }
    else if (!strcmp(frame->label, FR_FOLDER)) {
      track_folder = strtoucs2((unsigned char *) frame->data.strval);
      *tagsize += (6 + 2*ucs2strlen(track_folder));
    }
    else if (!strcmp(frame->label, FR_CODEC)) {
      /* 6 bytes, 2 length 2 tagid 2bytes year */
      if (!strcmp(frame->data.strval, NJB_CODEC_MP3)) {
	/* Use old codec name NJB3_CODEC_MP3_ID_OLD on old firmware? */
	track_codec = NJB3_CODEC_MP3_ID;
	*tagsize += 6;
	had_codec = 1;
      } else if (!strcmp(frame->data.strval, NJB_CODEC_WAV)) {
	track_codec = NJB3_CODEC_WAV_ID;
	*tagsize += 6;
	had_codec = 1;
      } else if (!strcmp(frame->data.strval, NJB_CODEC_WMA)) {
	track_codec = NJB3_CODEC_WMA_ID;
	*tagsize += 6;
	had_codec = 1;
      } else if (!strcmp(frame->data.strval, NJB_CODEC_AA)) {
  track_codec = NJB3_CODEC_AA_ID;
  *tagsize += 6;
  had_codec = 1;
      } else {
	printf("LIBNJB panic: unknown codec type!\n");
      }
    }
    else if (!strcmp(frame->label, FR_PROTECTED)) {
      /* It's enough for this to exist in order to protect the track */
      track_protected = 0x0001U;
      *tagsize += 6;
    }
  }
  
  data = (unsigned char *) malloc (*tagsize);
  if ( data == NULL ) {
    return NULL;
  }
  memset(data, 0, *tagsize);
  
  /* Then add the album strings */
  if (track_title != NULL)
    add_bin_unistr(data, &datap, NJB3_TITLE_FRAME_ID, track_title);
  if (track_album != NULL)
    add_bin_unistr(data, &datap, NJB3_ALBUM_FRAME_ID, track_album);
  if (track_artist != NULL)
    add_bin_unistr(data, &datap, NJB3_ARTIST_FRAME_ID, track_artist);
  if (track_genre != NULL)
    add_bin_unistr(data, &datap, NJB3_GENRE_FRAME_ID, track_genre);
  if (had_size != 0) {
    from_16bit_to_njb3_bytes(6, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(NJB3_FILESIZE_FRAME_ID, &data[datap]);
    datap += 2;
    from_32bit_to_njb3_bytes(track_size, &data[datap]);
    datap += 4;
  }
  /* Default to unlocked files if == 0 */
  if (track_protected != 0) {
    from_16bit_to_njb3_bytes(4, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(NJB3_FILESIZE_FRAME_ID, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(0x0001U, &data[datap]);
    datap += 2;
  }
  if (had_codec != 0) {
    from_16bit_to_njb3_bytes(4, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(NJB3_CODEC_FRAME_ID, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(track_codec, &data[datap]);
    datap += 2;
  }
  if (had_year != 0) {
    from_16bit_to_njb3_bytes(4, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(NJB3_YEAR_FRAME_ID, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(track_year, &data[datap]);
    datap += 2;
  }
  if (had_trackno != 0) {
    from_16bit_to_njb3_bytes(4, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(NJB3_TRACKNO_FRAME_ID, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(track_number, &data[datap]);
    datap += 2;
  }
  if (had_length != 0 && track_length != 0) {
    from_16bit_to_njb3_bytes(4, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(NJB3_LENGTH_FRAME_ID, &data[datap]);
    datap += 2;
    from_16bit_to_njb3_bytes(track_length, &data[datap]);
    datap += 2;
  }
  if (track_fname != NULL)
    add_bin_unistr(data, &datap, NJB3_FNAME_FRAME_ID, track_fname);
  if (track_folder != NULL)
    add_bin_unistr(data, &datap, NJB3_DIR_FRAME_ID, track_folder);
  
  if (track_title != NULL)
    free(track_title);
  if (track_album != NULL)
    free(track_album);
  if (track_artist != NULL)
    free(track_artist);
  if (track_genre != NULL)
    free(track_genre);
  if (track_fname != NULL)
    free(track_fname);
  if (track_folder != NULL)
    free(track_folder);
  return data;
}

/**
 * This adds a song ID frame to a song ID.
 *
 * @param song the song ID to add the frame to
 * @param frame the frame to add to this song ID
 */
void NJB_Songid_Addframe(njb_songid_t *song, njb_songid_frame_t *frame)
{
  if (frame == NULL) {
    return;
  }
  if ( song->nframes ) {
    song->last->next= frame;
    song->last= frame;
  } else {
    song->first= song->last= frame;
    song->cur= song->first;
  }
  frame->next= NULL;
  song->nframes++;
}

/**
 * This destroys an entire song ID structure and free the memory
 * used by it.
 *
 * @param songid the song ID structure to destroy
 */
void NJB_Songid_Destroy(njb_songid_t *songid)
{
  njb_songid_frame_t *frame;
  
  NJB_Songid_Reset_Getframe(songid);
  while ( (frame = NJB_Songid_Getframe(songid)) ) {
    NJB_Songid_Frame_Destroy(frame);
  }
  free(songid);
}

/**
 * This resets the internal pointer in the song ID so that it
 * points to the first frame of the song ID. It should typically
 * be called before subsequent calls to
 * <code>NJB_Songid_Getframe()</code>.
 *
 * Typical usage:
 *
 * <pre>
 * njb_songid_t *song;
 * njb_songid_frame_t *frame;
 * 
 * // Get a song ID into "song"...
 * NJB_Songid_Reset_Getframe(song);
 * while ( (frame = NJB_Songid_Getframe(song)) != NULL ) {
 *    // Do something with all the frames...
 * }
 * </pre>
 *
 * @param song the song ID structure whose internal frame pointer
 *             shall be reset.
 * @see NJB_Songid_Getframe()
 */
void NJB_Songid_Reset_Getframe(njb_songid_t *song)
{
  song->cur = song->first;
}

/**
 * This gets the next song ID frame from a song ID structure.
 *
 * @param song the song ID to get frames from
 * @return a song ID frame, or NULL if the last frame has already
 *         been returned.
 * @see NJB_Songid_Reset_Getframe()
 */
njb_songid_frame_t *NJB_Songid_Getframe(njb_songid_t *song)
{
  njb_songid_frame_t *t;
  
  if ( song->cur == NULL ) return NULL;
  t = song->cur;
  song->cur = song->cur->next;
  
  return t;
}

/**
 * This locates a particular song ID frame inside a song ID, by using the
 * textual label given.
 *
 * @param song the song ID to look in
 * @param label the textual label of the frame to look for
 * @return the song ID frame if found, else NULL
 */
njb_songid_frame_t *NJB_Songid_Findframe(njb_songid_t *song, const char *label)
{
  njb_songid_frame_t *frame;
  
  NJB_Songid_Reset_Getframe(song);
  while ( (frame = NJB_Songid_Getframe(song)) ) {
    if ( !strcmp(frame->label, label) ) return frame;
  }
  
  return NULL;
}

/**
 * This function checks that a tag has all compulsory elements,
 * i.e.: size, codec and track number
 *
 * @param songid the song ID to be checked for sanity
 * @return 0 if the tag is sane, -1 if it is insane
 */
int songid_sanity_check(njb_t *njb, njb_songid_t *songid)
{
  __dsub= "songid_sanity_check";
  njb_songid_frame_t *frame;
  u_int8_t has_title = 0;
  u_int8_t size_sane = 0;
  u_int8_t codec_sane = 0;
  u_int8_t length_sane = 0;
  u_int8_t has_trackno = 0;
  u_int8_t types_match = 1;

  __enter;

  NJB_Songid_Reset_Getframe(songid);
  while ((frame = NJB_Songid_Getframe(songid)) != NULL) {
    if (!strcmp(frame->label, FR_TITLE) && frame->type == NJB_TYPE_STRING) {
      has_title = 1;
    } else if (!strcmp(frame->label, FR_SIZE) && frame->type == NJB_TYPE_UINT32) {
      if ( frame->data.u_int32_val > 0) {
	size_sane = 1;
      }
    } else if (!strcmp(frame->label, FR_CODEC) && frame->type == NJB_TYPE_STRING) {
      if (!strcmp(frame->data.strval,NJB_CODEC_MP3) ||
	  !strcmp(frame->data.strval,NJB_CODEC_WAV) ||
	  !strcmp(frame->data.strval,NJB_CODEC_WMA)) {
	codec_sane = 1;
      }
    } else if (!strcmp(frame->label, FR_TRACK) && frame->type == NJB_TYPE_UINT16) {
      has_trackno = 1;
    } else if (!strcmp(frame->label, FR_LENGTH) && frame->type == NJB_TYPE_UINT16) {
      if (frame->data.u_int16_val > 0) {
	length_sane = 1;
      }
    } else if ( (!strcmp(frame->label, FR_YEAR) && frame->type != NJB_TYPE_UINT16) ||
		(!strcmp(frame->label, FR_PROTECTED) && frame->type != NJB_TYPE_UINT16) ) {
      types_match = 0;
    }
  }
  if (!has_trackno) {
    /* Sanitize tag */
    frame = NJB_Songid_Frame_New_Tracknum(0);
    NJB_Songid_Addframe(songid, frame);
  }

  /* If it is insane, signal back */
  if (!has_title) {
    njb_error_add_string(njb, subroutinename, "Song title missing.");
    __leave;
    return -1;
  } else  if (!size_sane) {
    njb_error_add_string(njb, subroutinename, "File is zero bytes long.");
    __leave;
    return -1;
  } else if (!codec_sane) {
    njb_error_add_string(njb, subroutinename, "Unrecognized codec.");
    __leave;
    return -1;
  } else if (!length_sane) {
    njb_error_add_string(njb, subroutinename, "Song is zero seconds long.");
    __leave;
    return -1;
  } else if (!types_match) {
    njb_error_add_string(njb, subroutinename, "Year or protection frame is not 16-bit.");
    __leave;
    return -1;
  }
  __leave;
  return 0;
}


/*
 * Song ID frame methods 
 */

/**
 * Creates a new string frame.
 *
 * @param label the label of this string frame
 * @param value the string contained in this string frame
 * @return valid string frame on success, NULL on failure
 */
njb_songid_frame_t *NJB_Songid_Frame_New_String(const char *label, const char *value)
{
  __dsub= "Songid_Frame_New_String";
  njb_songid_frame_t *frame;

  __enter;
  if (label == NULL || value == NULL) {
    return NULL;
  }
  frame = (njb_songid_frame_t *) malloc(sizeof(njb_songid_frame_t));
  if (frame == NULL) {
    __leave;
    return NULL;
  }
  frame->label = malloc(strlen(label)+1);
  frame->type = NJB_TYPE_STRING;
  frame->data.strval = malloc(strlen(value)+1);
  if (frame->label == NULL || frame->data.strval == NULL) {
    __leave;
    return NULL;
  }
  strcpy(frame->label, label);
  strcpy(frame->data.strval, value);
  __leave;
  return frame;
}

/**
 * This is a wrapper function to fix a common mistake made when creating codec
 * frames: lowercase codec names and other strange mistakes.
 */
njb_songid_frame_t *NJB_Songid_Frame_New_Codec(const char *value)
{
  __dsub= "Songid_Frame_New_Codec";
  njb_songid_frame_t *frame;
  int bad_codec = 0;

  __enter;
  if (!strcmp(value,NJB_CODEC_MP3) ||
      !strcmp(value,NJB_CODEC_WAV) ||
      !strcmp(value,NJB_CODEC_WMA)) {
    frame = NJB_Songid_Frame_New_String(FR_CODEC, value);
  } else if (!strcmp(value,"mp3")) {
    frame = NJB_Songid_Frame_New_String(FR_CODEC, NJB_CODEC_MP3);
    bad_codec = 1;
  } else if (!strcmp(value,"wav")) {
    frame = NJB_Songid_Frame_New_String(FR_CODEC, NJB_CODEC_WAV);
    bad_codec = 1;
  } else if (!strcmp(value,"wma") || !strcmp(value,"asf") || !strcmp(value, "ASF") ) {
    frame = NJB_Songid_Frame_New_String(FR_CODEC, NJB_CODEC_WMA);
    bad_codec = 1;
  } else {
    printf("LIBNJB panic: really bad codec string: \"%s\"\n", value);
    frame = NULL;
  }
  
  if (bad_codec != 0) {
    printf("LIBNJB warning: bad codec string: \"%s\"\n", value);
    printf("LIBNJB warning: the error has been corrected but please bug report the program.\n");
  }
  
  __leave;
  return frame;
}

/**
 * Creates a new unsigned 16-bit integer frame.
 *
 * @param label the label of this unsigned 16-bit integer frame
 * @param value the unsigned 16-bit integer contained in this frame
 * @return valid unsigned 16-bit integer frame on success, NULL on failure
 */
njb_songid_frame_t *NJB_Songid_Frame_New_Uint16(const char *label, u_int16_t value)
{
  __dsub= "Songid_Frame_New_Uint16";
  njb_songid_frame_t *frame;

  __enter;
  if (label == NULL) {
    return NULL;
  }
  frame = (njb_songid_frame_t *) malloc(sizeof(njb_songid_frame_t));
  if (frame == NULL) {
    __leave;
    return NULL;
  }
  frame->label = malloc(strlen(label)+1);
  if (frame->label == NULL) {
    __leave;
    return NULL;
  }
  strcpy(frame->label, label);
  frame->type = NJB_TYPE_UINT16;
  frame->data.u_int16_val = value;
  __leave;
  return frame;
}

/**
 * Creates a new unsigned 32-bit integer frame.
 *
 * @param label the label of this unsigned 32-bit integer frame
 * @param value the unsigned 32-bit integer contained in this frame
 * @return valid unsigned 32-bit integer frame on success, NULL on failure
 */
njb_songid_frame_t *NJB_Songid_Frame_New_Uint32(const char *label, u_int32_t value)
{
  __dsub= "Songid_Frame_New_Uint32";
  njb_songid_frame_t *frame;

  __enter;
  if (label == NULL) {
    return NULL;
  }
  frame = (njb_songid_frame_t *) malloc(sizeof(njb_songid_frame_t));
  if (frame == NULL) {
    __leave;
    return NULL;
  }
  frame->label = malloc(strlen(label)+1);
  if (frame->label == NULL) {
    __leave;
    return NULL;
  }
  strcpy(frame->label, label);
  frame->type = NJB_TYPE_UINT32;
  frame->data.u_int32_val = value;
  __leave;
  return frame;
}

/**
 * This destroys a song ID frame and free any memory used by it.
 *
 * @param frame the song ID frame to destroy
 */
void NJB_Songid_Frame_Destroy (njb_songid_frame_t *frame)
{
  if ( frame->label != NULL ) free(frame->label);
  if ( frame->type == NJB_TYPE_STRING && frame->data.strval != NULL) {
    free ( frame->data.strval );
  }
  free(frame);
}

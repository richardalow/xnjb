/**
 * \file procedure.c
 *
 * This file contains most generic functions and switches
 * between different protocols depending on the device type
 * that has been connected.
 */

/* MSVC does not use autoconf */
#ifndef _MSC_VER
#include "config.h"
#endif

#include <sys/stat.h> /* stat() */
#include <ctype.h> /* isspace() */
#include <fcntl.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h> /* basename() */
#endif
#include <string.h> /* Various memory and string functions */
#include "libnjb.h"
#include "procedure.h"
#include "protocol.h"
#include "protocol3.h"
#include "base.h"
#include "njb_error.h"
#include "defs.h"
#include "ioutil.h"
#include "unicode.h"
#include "byteorder.h"
#include "eax.h"
#include "songid.h"
#include "datafile.h"
#include "njbtime.h"

static int _lib_ctr_update (njb_t *njb);
int _file_size (njb_t *njb, const char *path, u_int64_t *size);
int _file_time (njb_t *njb, const char *path, time_t *ts);
int NJB_Handshake (njb_t *njb);

extern int njb_unicode_flag;
extern int __sub_depth;

/* Function that compensate for missing libgen.h on Windows */
#ifndef HAVE_LIBGEN_H
static char *basename(char *in) {
  char *p;
  if (in == NULL)
    return NULL;
  p = in + strlen(in) - 1;
  while (*p != '\\' && *p != '/' && *p != ':')
    { p--; }
  return ++p;
}
#endif

static int _lib_ctr_update (njb_t *njb)
{
  __dsub= "_lib_ctr_update";
  njblibctr_t lcount;
  njb_state_t *state = (njb_state_t *) njb->protocol_state;
  
  __enter;
  
  if ( state->session_updated ) return 0;
  
  if ( njb_get_library_counter(njb, &lcount) == -1 ) {
    __leave;
    return -1;
  }
  
  if ( memcmp(&state->sdmiid, lcount.id, 16) ) {
    NJB_ERROR(njb,EO_BADNJBID);
    __leave;
    return -1;
  }
  
  lcount.count++;
  
  if ( njb_set_library_counter(njb, lcount.count) == -1 ) {
    __leave;
    return -1;
  }
  
  if ( njb_verify_last_command(njb) == -1 ) {
    __leave;
    return -1;
  }
  
  state->session_updated = 1;
  state->libcount ++;
  __leave;
  return 0;
}

/**
 * This scans the USB buses for available devices, i.e. jukeboxes.
 *
 * @param njbs a pointer to an array of <code>njb_t</code>
 *             objects that can be used for storing up to
 *             <code>limit</code> objects.
 * @param limit the maximum number of <code>njb_t</code>
 *             device objects to retrieve (currently unused).
 * @param n a pointer to a variable that will hold the number
 *          of objects actually retrieved.
 * @return 0 on success, -1 on failure.
 */
int NJB_Discover (njb_t *njbs, int limit, int *n)
{
  __dsub= "NJB_Discover";
  int ret;
  
  __enter;
  
  ret = njb_discover(njbs, limit, n);
  
  __leave;
  return ret;
}

/**
 * This opens a device for use. This routine will initialize
 * the USB endpoints and interfaces, and allocate an error stack
 * so that error reporting routines can be used after this
 * call.
 *
 * @param njb a pointer to the NJB object to open
 * @return 0 on success, -1 on failure
 */
int NJB_Open (njb_t *njb)
{
  __dsub= "NJB_Open";
  int ret = 0;
  
  __enter;
  
  if ( (ret = njb_open(njb)) == -1 ) {
    /*
     * We do not free the error stack on failed open,
     * this will leak memory since we do not call NJB_Close()
     * when this routine fails with -1 as return code, but not 
     * reporting our errors is worse. During normal operation, 
     * the error stack will be freed in NJB_Close().
     */
  } else {
    /*
     * This was invented for the Zen but is now activated
     * for NJB3. It is apparently not applicable for USB 2.0
     * devices (Zen USB 2.0, NJB 2, Zen NX).
     * Also see NJB_Close() (similar code). LW.
     */
    
    if (njb->device_type == NJB_DEVICE_NJB1) {
      if ( njb_init_state(njb) == -1) {
	__leave;
	return -1;
      }
    }
    
    if (PDE_PROTOCOL_DEVICE(njb)) {
      if ( njb3_init_state(njb) == -1) {
	__leave;
	return -1;
      }
    }
    
    if (njb->device_type == NJB_DEVICE_NJBZEN
	|| njb->device_type == NJB_DEVICE_NJB3) {
      njb3_capture(njb);
    }
    
    ret = NJB_Handshake(njb);
    
  }
  __leave;
  return ret;
}

/**
 * This closes a previously opened and used NJB object.
 *
 * @param njb a pointer to the <code>njb_t</code> object to close.
 */
void NJB_Close (njb_t *njb)
{
  __dsub= "NJB_Close";
  
  __enter;
  
  /* Release 3-series devices */
  if (PDE_PROTOCOL_DEVICE(njb)) {
    /*
     * This special was written by Dwight Engen due to problems with
     * the ZEN refusing to reconnect after running sample programs.
     * the same thing used for NJB3. USB 2.0 devices should not use this.
     * Also see NJB_Open() (similar code).
     */
    if (njb->device_type == NJB_DEVICE_NJBZEN
	|| njb->device_type == NJB_DEVICE_NJB3
	) {
      njb3_ping(njb, 1);
    }
    njb3_release(njb);
    
    /* Free dangling pointers in njb_t struct */
    njb3_destroy_state(njb);
  }
  
  njb_close(njb);
  
  /* Destroy error stack */
  destroy_errorstack(njb);
  
  __leave;
}

/**
 * This captures a device for usage after opening.
 *
 * @param njb a pointer to the <code>njb_t</code> object to capture
 * @return 0 on success, -1 on failure
 */
int NJB_Capture (njb_t *njb)
{
  __dsub= "NJB_Capture";
  __enter;
  
  
  /* This is simply ignored on NJB3, where it isn't applicable.
   * The ZEN is captured at NJB_Open and released at NJB_Close.
   */
  if (njb->device_type == NJB_DEVICE_NJB1) {
    njblibctr_t lcount;
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    njb_error_clear(njb);
    
    if ( njb_capture(njb, NJB_CAPTURE) == -1 ) {
      __leave;
      return -1;
    }
    
    if ( njb_get_library_counter(njb, &lcount) == -1 ) {
      __leave;
      return -1;
    }
    
    if ( state->libcount != lcount.count ) {
      njb_capture(njb, NJB_RELEASE);
      NJB_ERROR(njb, EO_BADCOUNT);
      __leave;
      return -1;
    }
  }
  
  __leave;
  return 0;
}

/**
 * This releases a captured NJB object after use.
 *
 * @param njb a pointer to the <code>njb_t</code> object to release
 * @return 0 on success, -1 on failure
 */
int NJB_Release (njb_t *njb)
{
  __dsub= "NJB_Release";
  int ret;
  
  __enter;
  
  njb_error_clear(njb);
  
  /* This is simply ignored on NJB3, where it isn't applicable */
  if (njb->device_type == NJB_DEVICE_NJB1) {
    ret= njb_capture(njb, NJB_RELEASE);
  } else {
    ret= 0;
  }
  
  __leave;
  return ret;
}

/**
 * This function "handshakes" a previously opened and captured
 * device represented by a <code>njb_t</code> object. This basically
 * means some basic "how do you do" calls are made, and the 
 * <code>njb_id_t</code> property of the <code>njb_t</code> object
 * gets filled in properly.
 *
 * @param njb a pointer to the <code>njb_t</code> object to perform
 *            the handshake on
 * @return 0 on success, -1 on failure
 */
int NJB_Handshake (njb_t *njb)
{
  __dsub= "NJB_Handshake";
  
  __enter;
  
  njb_error_clear(njb);
  
  if ( njb->device_type == NJB_DEVICE_NJB1 ) {
    if ( njb_ping(njb) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    
    if ( njb3_ping(njb, 0) == -1 ) {
      __leave;
      return -1;
    }
    /* Retrieve the list of supported codecs, just for fun. */
    if ( njb3_get_codecs(njb) == -1 ) {
      __leave;
      return -1;
    }
    /*
     * Retrieve keys - this is not used for anything yet,
     * part of the "AR00" key should be used in the storage
     * of an MP3 file. 
     */
    if ( njb3_read_keys(njb) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  /*
   * For the NJB1, retrieve the library counter.
   */
  if (njb->device_type == NJB_DEVICE_NJB1) {
    njblibctr_t lcount, lcount_new, lcount_old;
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    if ( njb_get_library_counter(njb, &lcount) == -1 ) {
      __leave;
      return -1;
    }
    
    /* Check that it's the same ID for the device and the library counter */
    if ( memcmp(&state->sdmiid, lcount.id, 16) ) {
      NJB_ERROR(njb, EO_BADNJBID);
      __leave;
      return -1;
    }
    
    lcount_new = lcount;
    lcount_old = lcount;
    
    /*
     * The following tries to increase the library counter by
     * one, checks that this succeeds and then resets the counter
     * to the old value again.
     */
    
    lcount_new.count++;
    
    if ( njb_set_library_counter(njb, lcount_new.count) == -1 ) {
      __leave;
      return -1;
    }
    
    if ( njb_verify_last_command(njb) == -1 ) {
      __leave;
      return -1;
    }
    
    if ( njb_get_library_counter(njb, &lcount) == -1 ) {
      __leave;
      return -1;
    }
    
    if ( memcmp(&state->sdmiid, lcount.id, 16) ) {
      NJB_ERROR(njb, EO_BADNJBID);
      __leave;
      return -1;
    }
    
    if ( lcount.count != lcount_new.count ) {
      NJB_ERROR(njb, EO_BADCOUNT);
      __leave;
      return -1;
    }
    
    if ( njb_set_library_counter(njb, lcount_old.count) == -1 ) {
      __leave;
      return -1;
    }
    
    if ( njb_verify_last_command(njb) == -1 ) {
      __leave;
      return -1;
    }
    
    state->libcount = lcount_old.count;
  }
  
  __leave;
  return 0;
}

/**
 * This configures libnjb to retrieve extended tags from the device.
 * For the NJB1 this is the default behaviour anyway so it need not
 * be set, but for the series 3 devices, retrieving the extended tag
 * information is a costly operation that will slow down the initial
 * track scanning by orders of magnitude and irritate the user. Make
 * sure end-users can configure whether they want to use this or not.
 *
 * The extended tags will include things like the filename and folder
 * that the file used on the host before it was transfered to the
 * device.
 * 
 * @param njb a pointer to the <code>njb_t</code> object to set this mode for
 * @param extended use 0 for non-exetended tags (default), 1 for 
 *                 extended tags
 */
void NJB_Get_Extended_Tags (njb_t *njb, int extended)
{
  __dsub= "NJB_Get_Extended_Tags";
  __enter;

  njb_error_clear(njb);

  if (PDE_PROTOCOL_DEVICE(njb)) {
    njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
    state->get_extended_tag_info = extended;
  }

  __leave;
}

/**
 * This resets the track tag (song ID) retrieveal function. The track
 * tags can then be retrieved one by one using the <code>NJB_Get_Track_Tag()</code>
 * function.
 *
 * Typical usage:
 *
 * <pre>
 * njb_t *njb;
 * njb_songid_t *song;
 * 
 * NJB_Songid_Reset_Get_Track_Tag(njb);
 * while ( (song = NJB_Get_Track_Tag(njb)) != NULL ) {
 *    // Do something with all the songs...
 * }
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to reset
 *            the track retrieveal pointer for
 * @see NJB_Get_Track_Tag()
 */
void NJB_Reset_Get_Track_Tag (njb_t *njb)
{
  __dsub= "NJB_Reset_Get_Track_Tag";
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    state->reset_get_track_tag = 1;
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if (njb3_reset_get_track_tag(njb) == -1) {
      /* FIXME: do something with this error some day. */
    }
  }
  
  __leave;
}

/**
 * This gets a track tag (song ID) from the device. The device should
 * first be rewound using the <code>NJB_Reset_Get_Track_Tag()</code>
 * function.
 *
 * Notice that there is no function for getting the tag for a 
 * <i>single</i> track, there is just this function that dumps out
 * the entire database in one big go. The recommended approach is
 * to keep an internal track cache of all tracks and use this for
 * getting metadata for single tracks. There is as far as we know
 * no function in the Creative firmwares for getting the metadata
 * of a single track, so access through this function is the only
 * option.
 *
 * @param njb a pointer to the <code>njb_t</code> object to reset
 *            the track retrieveal pointer for
 * @return a track tag (song ID) or NULL if the last track tag
 *         has already been retrieved
 * @see NJB_Reset_Get_Track_Tag()
 */
njb_songid_t *NJB_Get_Track_Tag (njb_t *njb)
{
  __dsub= "NJB_Get_Track_Tag";
  int status;
  njb_songid_t *ret = NULL;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    njbttaghdr_t tagh;
    njb_state_t *state = (njb_state_t *) njb->protocol_state;

    if ( state->reset_get_track_tag ) {
      status = njb_get_first_track_tag_header(njb, &tagh);
      state->reset_get_track_tag = 0;
    } else {
      status = njb_get_next_track_tag_header(njb, &tagh);
    }
    ret = ( status < 0 ) ? NULL : njb_get_track_tag(njb, &tagh);
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_get_next_track_tag(njb);
  }
  
  __leave;
  return ret;
}

/**
 * This resets the playlist retrieveal function. The playlists
 * can then be retrieved one by one using the 
 * <code>NJB_Get_Playlist()</code> function.
 *
 * Typical usage:
 *
 * <pre>
 * njb_t *njb;
 * njb_playlist_t *pl;
 * 
 * NJB_Reset_Get_Playlist(njb);
 * while ( (pl = NJB_Get_Playlist(njb)) != NULL ) {
 *    // Do something with all the playlists...
 *    NJB_Playlist_Destroy(pl);
 * }
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to reset
 *            the playlist retrieveal pointer for
 * @see NJB_Get_Playlist()
 */
void NJB_Reset_Get_Playlist (njb_t *njb)
{
  __dsub= "NJB_Reset_Get_Playlist";
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    state->reset_get_playlist= 1;
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if (njb3_reset_get_playlist_tag(njb) == -1) {
      /* FIXME: do something about this error eventually */
    }
  }
  
  __leave;
}

/**
 * This gets a playlist from the device. The device should
 * first be rewound using the <code>NJB_Reset_Get_Playlist()</code>
 * function. The playlists are newly allocated and should be 
 * destroyed with <code>NJB_Playlist_Destroy()</code> after use.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get
 *            playlists from
 * @return a playlist or NULL if the last playlist has already
 *         been returned
 * @see NJB_Reset_Get_Playlist()
 * @see NJB_Playlist_Destroy()
 */
njb_playlist_t *NJB_Get_Playlist (njb_t *njb)
{
  __dsub= "NJB_Get_Playlist";
  njbplhdr_t plh;
  int retry= 3;
  njb_playlist_t *ret = NULL;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    if ( state->reset_get_playlist ) {
      /* Get the first header */
      while (retry) {
	int retval = njb_get_first_playlist_header(njb, &plh);
	/* On error or last playlist */
	if ((retval == -1) || (retval == -3)) {
	  __leave;
	  return NULL;
	}
	else if (retval == -2) {
	  retry --;
	}
	else break;
      }
      state->reset_get_playlist = 0;
    } else {
      /* Then get them all */
      while (retry) {
	int retval = njb_get_next_playlist_header(njb, &plh);
	/* If error or last playlist */
	if ((retval == -1) || (retval == -3)) {
	  __leave;
	  return NULL;
	}
	else if (retval == -2) {
	  retry --;
	}
	else break;
      }
    }
    ret = njb_get_playlist(njb, &plh);
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_get_next_playlist_tag(njb);
  }
  
  __leave;
  return ret;
}

/**
 * This retrieves the amount of currently used disk space in bytes.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get usage for
 * @param btotal a pointer to a 64 bit integer that shall hold the number
 *               of total bytes on the device after the call
 * @param bfree a pointer to a 64 bit integer that shall hold the number
 *               of free bytes on the device after the call
 * @return 0 on success, -1 on failure
 */
int NJB_Get_Disk_Usage (njb_t *njb, u_int64_t *btotal, u_int64_t *bfree)
{
  __dsub= "NJB_Get_Disk_Usage";
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    int retry = 3;
    while (retry) {
      int retval = njb_get_disk_usage(njb, btotal, bfree);
      if (retval == -1) {
	__leave;
	return -1;
      } else if (retval == -2) {
	retry --;
      } else break;
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if ( njb3_get_disk_usage(njb, btotal, bfree) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  __leave;
  return 0;
}

/**
 * This retrieves the owner string for the device, a string representing
 * the owners name.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get the owner
 *            string for
 * @return a valid owner string or NULL on failure. The string is newly
 *         allocated on the heap and should be freed by the caller after use.
 */
char *NJB_Get_Owner_String (njb_t *njb)
{
  __dsub= "NJB_Get_Owner_String";
  owner_string name;
  char *op = NULL;
  
  njb_error_clear(njb);
  
  __enter;
  
  if ( njb->device_type == NJB_DEVICE_NJB1 ) {
    if ( njb_get_owner_string(njb, name) == -1 ) {
      __leave;
      return NULL;
    }
    if (njb_unicode_flag == NJB_UC_UTF8) {
      op = strtoutf8((unsigned char *) name);
    } else {
      op = strdup((char *) name);
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if ( njb3_get_owner_string(njb, (char *)name) == -1 ) {
      __leave;
      return NULL;
    }
    op = strdup((char *) name);
  }
  
  if ( op == NULL ) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return NULL;
  }
  
  __leave;
  return op;
}

/**
 * This sets the owner string for the device, a string representing
 * the owner.
 *
 * @param njb a pointer to the <code>njb_t</code> object to set 
 *            the owner string for
 * @param name the new owner string
 * @return 0 on success, -1 on failure
 */
int NJB_Set_Owner_String (njb_t *njb, const char *name)
{
  __dsub= "NJB_Set_Owner_String";
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    owner_string oname;
    
    memset(oname, 0, OWNER_STRING_LENGTH);
    if (njb_unicode_flag == NJB_UC_UTF8) {
      char *tmp;
      
      tmp = utf8tostr((unsigned char *) name);
      strncpy((char *) oname, tmp, OWNER_STRING_LENGTH);
      free(tmp);
    } else {
      strncpy((char *) oname, name, OWNER_STRING_LENGTH);
    }
    
    if ( njb_set_owner_string(njb, oname) == -1 ) {
      __leave;
      return -1;
    }
    
    if ( njb_verify_last_command(njb) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if ( njb3_set_owner_string(njb, name) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  __leave;
  return 0;
}

/**
 * This resets the datafile metadata retrieveal function. 
 * The datafile tags can then be retrieved one by one using the 
 * <code>NJB_Get_Datafile_Tag()</code> function.
 *
 * Typical usage:
 *
 * <pre>
 * njb_t *njb;
 * njb_datafile_t *df;
 * 
 * NJB_Reset_Get_Datafile_Tag(njb);
 * while ( (df = NJB_Get_Datafile_Tag(njb)) != NULL ) {
 *    // Do something with all the datafiles...
 *    NJB_Datafile_Destroy(df);
 * }
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to reset
 *            the datafile retrieveal pointer for
 * @see NJB_Get_Datafile_Tag()
 */
void NJB_Reset_Get_Datafile_Tag (njb_t *njb)
{
  __dsub= "NJB_Get_Datafile_Tag";
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1 ) {
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    state->reset_get_datafile_tag = 1;
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if (njb3_reset_get_datafile_tag(njb) == -1) {
      /* FIXME: handle this error eventually */
    }
  }
  
  __leave;
}

/**
 * This gets a datafile tag from the device. The device should
 * first be rewound using the <code>NJB_Reset_Get_Datafile_Tag()</code>
 * function. The tag is newly allocated and should be destroyed with
 * <code>NJB_Datafile_Destroy()</code> after use.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get
 *            datafiles from
 * @return a datafile tag or NULL if the last datafile tag has 
 *         already been returned
 * @see NJB_Reset_Get_Datafile_Tag()
 */
njb_datafile_t *NJB_Get_Datafile_Tag (njb_t *njb)
{
  __dsub= "NJB_Get_Datafile_Tag";
  njbdfhdr_t dfh;
  int status;
  njb_datafile_t *ret = NULL;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1 ) {
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    if ( state->reset_get_datafile_tag ) {
      status = njb_get_first_datafile_header(njb, &dfh);
      state->reset_get_datafile_tag = 0;
    } else {
      status = njb_get_next_datafile_header(njb, &dfh);
    }
    
    ret= ( status < 0 ) ? NULL : njb_get_datafile_tag(njb, &dfh);
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_get_next_datafile_tag(njb);
  }
  
  __leave;
  return ret;
}

/**
 * This retrieves ("uploads") a track from the device to the host
 * computer.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get
 *            the track from
 * @param fileid the unique trackid (also known as file ID, they
 *               are the same things) as reported by the device
 *               from e.g. <code>NJB_Get_Track_Tag()</code>.
 * @param size the size of the track in bytes. You know this size
 *             from previous calls to <code>NJB_Get_Track_Tag()</code>
 *             and it is needed among other things for displaying a
 *             progress bar and for determining that all bytes have been
 *             correctly retrieved.
 * @param path the path where the resulting file should be written.
 *             If this parameter is null, the blocks are delivered
 *             via the callback's buf parameter.
 * @param callback a function that will be called repeatedly to report
 *             progress during transfer, used for e.g. displaying
 *             progress bars
 * @param data a voluntary parameter that can associate some 
 *             user-supplied data with each callback call. It is OK
 *             to set this to NULL of course.
 * @return 0 on success, -1 on failure
 * @see NJB_Get_Track_fd()
 */
 int NJB_Get_Track (njb_t *njb, u_int32_t fileid, u_int32_t size,
		    const char *path, NJB_Xfer_Callback *callback, void *data)
 {
   
   __dsub= "NJB_Get_Track";
   int fd = -1;
   int ret;
   
   __enter;
   
   /* big ini chunk */
   
#ifdef __WIN32__
   if ( path && (fd = open(path, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 0664)) == -1 ) {
#else										
   if ( path && (fd = open(path, O_CREAT|O_TRUNC|O_WRONLY, 0664)) == -1 ) {
#endif										
     njb_error_add(njb, "open", -1);
     NJB_ERROR(njb, EO_TMPFILE);
     ret = -1;
     goto clean_up_and_return;
   }

   ret = NJB_Get_Track_fd(njb, fileid, size, fd, callback, data);

   if ( path != NULL ) {
     close(fd);
     fd = -1;
   }

clean_up_and_return:

   if ( fd != -1 ) {
     close(fd);
   }
   
   /* On error, remove the partial file. */
   if ( ret == -1 ) {
     unlink(path);
   }
   
   __leave;
   return ret;
}


/**
 * This retrieves ("uploads") a track from the device to the host
 * computer by way of a file descriptor, which is good for e.g.
 * streaming stuff. The daring type can start playing back audio
 * from the file descriptor before it is finished. This is also
 * good for fetching to temporary files, which are often only
 * given as file descriptors.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get
 *            the track from
 * @param fileid the unique trackid (also known as file ID, they
 *               are the same things) as reported by the device
 *               from e.g. <code>NJB_Get_Track_Tag()</code>.
 * @param size the size of the track in bytes. You know this size
 *             from previous calls to <code>NJB_Get_Track_Tag()</code>
 *             and it is needed among other things for displaying a
 *             progress bar and for determining that all bytes have been
 *             correctly retrieved.
 * @param fd the file descriptor that shall be fed with the track
 *           contents. The file descriptor must be writable. On
 *           win32 make sure it is a binary descriptor and not textual.
 * @param callback a function that will be called repeatedly to report
 *             progress during transfer, used for e.g. displaying
 *             progress bars
 * @param data a voluntary parameter that can associate some 
 *             user-supplied data with each callback call. It is OK
 *             to set this to NULL of course.
 * @return 0 on success, -1 on failure
 * @see NJB_Get_Track()
 */
int NJB_Get_Track_fd (njb_t *njb, u_int32_t fileid, u_int32_t size,
		      int fd, NJB_Xfer_Callback *callback, void *data)
{
  __dsub= "NJB_Get_Track_fd";

  unsigned char *block = NULL;
  u_int32_t bsize, offset;
  u_int32_t remain = size;
  int abortxfer = 0;
  int ret;

  __enter;

  njb_error_clear(njb);

  if (njb->device_type == NJB_DEVICE_NJB1) {

    if ( njb_request_file(njb, fileid) == -1 ) {
      ret = -1;
      goto clean_up_and_return;
    }
    
    if ( njb_verify_last_command(njb) == -1 ) {
      NJB_ERROR(njb, EO_XFERDENIED);
      ret = -1;
      goto clean_up_and_return;
    }
  }

  /* File transfer routine for the NJB1 */
  if (njb->device_type == NJB_DEVICE_NJB1) {
    u_int32_t bread = 0;
    int wasshort = 0;
    
    block = (unsigned char *) malloc(NJB_XFER_BLOCK_SIZE + NJB_XFER_BLOCK_HEADER_SIZE);
    if ( block == NULL ) {
      NJB_ERROR(njb, EO_NOMEM);
      ret = -1;
      goto clean_up_and_return;
    }
    memset(block, 0, NJB_XFER_BLOCK_SIZE + NJB_XFER_BLOCK_HEADER_SIZE);
    offset = 0;
    while (remain != 0 && !abortxfer) {
      /* Request as much as possible unless the last chunk is reached */
      bsize = (remain > NJB_XFER_BLOCK_SIZE) ? NJB_XFER_BLOCK_SIZE : remain;
      
      bread = 0;
      
      bread = njb_receive_file_block(njb, offset, bsize, &block[0]);
      if (bread < bsize + NJB_XFER_BLOCK_HEADER_SIZE) {
	wasshort = 1;
      } else {
	wasshort = 0;
      }
      
      /* Handle errors */
      if (bread == -1) {
	ret = -1;
	goto clean_up_and_return;
      }
      
      /* Good debug messages */
      /*
	fprintf(stdout, "File get: recieved %04x bytes", bread);
	if (wasshort)
	fprintf(stdout, " - short read");
	fprintf(stdout,"\n");
      */
      
      /* Write receieved bytes to file */
      if (fd > -1 && bread > NJB_XFER_BLOCK_HEADER_SIZE) {
	if ( (write(fd, &block[NJB_XFER_BLOCK_HEADER_SIZE], bread-NJB_XFER_BLOCK_HEADER_SIZE)) == -1 ) {
	  njb_error_add(njb, "write", -1);
	  NJB_ERROR(njb, EO_WRFILE);
	  ret = -1;
	  goto clean_up_and_return;
	}
      }
      
      remain -= (bread-NJB_XFER_BLOCK_HEADER_SIZE);
      offset += (bread-NJB_XFER_BLOCK_HEADER_SIZE);
      
      if ( callback != NULL ) {
	const char *last_block= (const char *) &block[NJB_XFER_BLOCK_HEADER_SIZE];
	if ( callback(offset, size, last_block, bread-NJB_XFER_BLOCK_HEADER_SIZE, data) == -1 ) {
	  abortxfer = 1;
	}
      }
    }
    
    /* This is probably not the right way to abort a file transfer */
    
    if ( abortxfer ) {
      njb_transfer_complete(njb);
      NJB_ERROR(njb, EO_ABORTED);
      ret = -1;
      goto clean_up_and_return;
    } 
    
    if ( njb_transfer_complete(njb) == -1 ) {
      NJB_ERROR(njb, EO_XFERERROR);
      ret = -1;
      goto clean_up_and_return;
    }
  }

  /* Protocol 3 series file transfer routine */
  if (PDE_PROTOCOL_DEVICE(njb)) {
    int confirm_size;
    njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
    
    /* FIXME: add this and test. */
    /* njb3_ctrl_playing(njb, NJB3_STOP_PLAY); */
    
    block = (unsigned char *) malloc(NJB3_CHUNK_SIZE);
    if (block == NULL) {
      NJB_ERROR(njb, EO_NOMEM);
      ret = -1;
      goto clean_up_and_return;
    }
    
    /* Start at beginning of file */
    offset = 0;
    remain = size;
    
    while (remain != 0 && abortxfer == 0) {
      u_int32_t bwritten;
      int chunk_size;
      int chunk_remain;
      int bread;
      
      /*
       * Request chunks of size NJB3_CHUNK_SIZE at a time, 
       * indexed at an offset into the file. 
       */
      chunk_size = njb3_request_file_chunk(njb, fileid, offset);
      /*
       * This addresses a particular problem exposed by the recordings
       * made on the NJB3: files are reported as being smaller than
       * they actually are. It is a firmware bug, but let's just work
       * around it. (Fixed by Richard Low on 2005-09-22.)
       */
      if (chunk_size > remain) {
	printf("LIBNJB panic: chunk_size > remain, going to get whole chunk and see what happens\n");
	if (chunk_size == NJB3_CHUNK_SIZE) {
	  size += (NJB3_CHUNK_SIZE) + 1 - remain;
	  remain = NJB3_CHUNK_SIZE + 1;
	} else {
	  size += (chunk_size - remain);
	  remain = chunk_size;
	}
      } else if ( chunk_size == -1 ) {
	ret = -1;
	goto clean_up_and_return;
      }
      
      chunk_remain = chunk_size;
      
      /* Do NOT break on abort here - all blocks must be sent before 
       * cancelling the transfer! */
      while (chunk_remain != 0) {
	u_int32_t blocksize;

	/*
	 * This speed-up hack works on most devices but is
	 * of course user-configurable.
	 */
	if (state->turbo_mode == NJB_TURBO_OFF) {
	  blocksize = NJB3_DEFAULT_GET_FILE_BLOCK_SIZE;
	} else {
	  /*
	   * This speed-up hack comes courtesy of Richard Low.
	   * files of size 8192 or 8191 modulo 0x4000 are known to hang when 
	   * downloading if request is 0x4000 bytes, so we avoid this 
	   * scenario
	   */
	  if (chunk_remain <= 0x2000U) {
	    blocksize = 0x2000U;
	  } else {
	    blocksize = 0x4000U;
	  }
	}

	/*
	 * Try to get maximum amount, if we run into a short 
	 * read, that's OK 
	 */
	/* printf("Requesting chunk size %08X, remaining %08X...\n", NJB3_SUBCHUNK_SIZE, chunk_remain); */
	bread = njb3_get_file_block(njb, block, blocksize);
	/* Negative value signals error */
	if ( bread == -1 ) {
	  ret = -1;
	  goto clean_up_and_return;
	}
	
	/*
	 * Sometimes the chunk will be one
	 * byte larger than the reported chunk size.
	 * The byte is not part of the actual file.
	 *
	 * Adjust this downwards as of now...
	 */
	if (bread > chunk_remain) {
	  int extraneous = bread - chunk_remain;
	  if (extraneous == 1) {
	    /* printf("LIBNJB: one extraneous byte.\n"); */
	  }
	  if (extraneous > 1) {
	    /* This should, however, not happen. */
	    printf("LIBNJB panic: recieved %d extraneous bytes!\n", extraneous);
	  }
	  bread = chunk_remain;
	}
	
	if (fd > -1) {
	  /* Write the bytes to the destination file */
	  if ( (bwritten = write(fd, block, bread) ) == -1 ) {
	    njb_error_add(njb, "write", -1);
	    NJB_ERROR(njb, EO_WRFILE);
	    ret = -1;
	    goto clean_up_and_return;
	  } else if ( bwritten != bread ) {
	    NJB_ERROR(njb, EO_WRFILE);
	    ret = -1;
	    goto clean_up_and_return;
	  }
	}
	chunk_remain -= bread;
	offset += bread;
	
	if ( callback != NULL) {
	  if ( callback(offset, size, (const char *) block, bread, data) == -1 )
	    abortxfer= 1;
	}
      }
      
      /*
       * We have recieved a whole chunk or are at the 
       * end of the file.
       */
      if (chunk_size > remain) {
	int extraneous = chunk_size - remain;
	if (extraneous == 1) {
	  /* printf("LIBNJB: one extraneous byte in chunk.\n"); */
	}
	if (extraneous > 1) {
	  /* This should, however, not happen. */
	  printf("LIBNJB panic: recieved %d extraneous bytes!\n", extraneous);
	}
	chunk_size = remain;
      }
      remain -= chunk_size;
      if (remain < 0) {
	printf("LIBNJB panic: Remain < 0!\n");
	remain = 0;
      }
    }
    
    /*
     * File confirmation is accomplished by requesting 
     * file offset set at the file size (e.g. one past 
     * the end)
     */
    confirm_size = njb3_request_file_chunk(njb, fileid, size);
    if (confirm_size != 0) {
      ret = -1; /* FIXME: add some error message here */
      goto clean_up_and_return;
    }
  }

  if ( abortxfer ) {
    NJB_ERROR(njb, EO_ABORTED);
    ret = -1;
    goto clean_up_and_return;
  }
  
  ret = 0;
  
clean_up_and_return:
  
  if ( block != NULL ) {
    free(block);
  }
  
  __leave;
  return ret;
}


/**
 * This is a helper function for sending tracks and files.
 * The <code>NJB_Send_Track()</code> and <code>NJB_Send_File()</code> 
 * functions set up the transfer, then call this to transfer the file
 * chunks, then commit the transfer with special commands.
 *
 * @param njb a pointer to the <code>njb_t</code> object to send the
 *            file to
 * @param path the filesystem path on the local host to use as indata
 * @param size the size of the file to be sent, in bytes
 * @param fileid the file ID to send the data to; this must be
 *               established by earlier calls to the device before
 *               the actual transfer can commence.
 * @param callback a function that will be called repeatedly to report
 *             progress during transfer, used for e.g. displaying
 *             progress bars
 * @param data a voluntary parameter that can associate some 
 *             user-supplied data with each callback call. It is OK
 *             to set this to NULL of course.
 * @param operation 0 means this is a file or track send, 1 means that
 *             this operation sends a firmware image.
 * @return 0 on success, -1 on failure
 * @see NJB_Send_Track()
 * @see NJB_Send_File()
 */
static int send_file (njb_t *njb, const char *path, u_int64_t size, 
		      u_int32_t fileid, NJB_Xfer_Callback *callback, 
		      void *data, int operation)
{
  __dsub= "send_file";
  u_int64_t remain, offset;
  u_int32_t bp;
  size_t bread;
  unsigned char *block;
  int fd;
  int abortxfer= 0;
  int retry = 15;
  struct stat sb;
  
  __enter;

#ifdef __WIN32__
  if ( (fd = open(path, O_RDONLY|O_BINARY)) == -1 ) {
#else
  if ( (fd = open(path, O_RDONLY)) == -1 ) {
#endif
    njb_error_add(njb, "open", -1);
    NJB_ERROR(njb, EO_SRCFILE);
    __leave;
    return -1;
  }
  
  if ( fstat(fd, &sb) == -1 ) {
    njb_error_add(njb, "fstat", -1);
    NJB_ERROR(njb, EO_SRCFILE);
    __leave;
    return -1;
  }

  /* This space will be used as a reading ring buffer for the transfers */
  block = (unsigned char *) malloc(NJB_BUFSIZ);
  
  /* Terminate if memory block could not be allocated */
  if ( block == NULL ) {
    close(fd);
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  
  offset = 0;
  remain = size;
  
  /* Set pointer at end of block, so that a new block will be read. */
  bp = NJB_BUFSIZ;

  /* Keep sending while there is stuff to send and noone
   * interrupts us */
  while (remain && !abortxfer) {
    u_int32_t bwritten = 0;
    u_int32_t xfersize;
    u_int32_t readsize;
    u_int32_t maxblock = 0;
    
    if (PDE_PROTOCOL_DEVICE(njb)) {
      if (operation == 0) {
	njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

	/*
	 * This speed-up hack works on most devices but is
	 * of course user-configurable.
	 */
	if (state->turbo_mode == NJB_TURBO_OFF) {
	  maxblock = NJB3_DEFAULT_SEND_FILE_BLOCK_SIZE;
	} else {
	  /*
	   * This hack courtesy of Richard Low.
	   * Increasing the send max block by a factor of seven speeds up
	   * transfers considerably.
	   */
	  maxblock = 0xE000U;
	}
      } else if (operation == 1) {
	maxblock = NJB3_FIRMWARE_CHUNK_SIZE;
      }
    } else if (njb->device_type == NJB_DEVICE_NJB1) {
      maxblock = NJB_XFER_BLOCK_SIZE;
    }
    
    /* Next we will try to send this much */
    xfersize = (remain < maxblock) ? remain : maxblock;
    /*
      printf("Remain %08x bytes, Buffer %04x bytes, next we will send %04x bytes\n", 
      (u_int32_t) remain, (u_int32_t) NJB_BUFSIZ - bp, xfersize);
    */
    
    /* If the remaining bytes in the buffer is lower than
     * the largest transfer unit, read in a new chunk
     */
    if ( (NJB_BUFSIZ - bp) < xfersize ) {
      /* Bytes remaining at the bottom of the ringbuffer */
      u_int32_t bufbottom = NJB_BUFSIZ - bp;
      /* Buffer size to fill */
      u_int32_t fillsize = NJB_BUFSIZ - bufbottom;
      
      /* Copy end of buffer to top if there is something in it */
      if (bufbottom > 0) {
	memcpy(&block[0], &block[bp], bufbottom);
      }
      
      /* Read in as much as fills the buffer */
      readsize = (remain - bufbottom > (size_t) fillsize) ? (size_t) fillsize : (size_t) remain - bufbottom;
      /*
	printf("Remain %08x bytes, filling buffer with %08x bytes\n", (u_int32_t) remain, readsize);
      */
      
      if ( (bread = read(fd, &block[bufbottom], readsize)) < 1 ) {
	NJB_ERROR2(njb, "reached EOF (unexpected)", EO_SRCFILE);
	close(fd);
	free(block);
	__leave;
	return -1;
      }
      
      /* Reset buffer pointer */
      bp = 0;
      
    }
    
    /*
      printf("Remain %08x bytes, buffer has %08x bytes sending bytes...\n", 
      (u_int32_t) remain, NJB_BUFSIZ-bp);
    */
    
    if (PDE_PROTOCOL_DEVICE(njb)) {
      if (operation == 0) {
	bwritten = njb3_send_file_chunk(njb, &block[bp], xfersize, fileid);
      } else if (operation == 1) {
	bwritten = njb3_send_firmware_chunk(njb, xfersize, &block[bp]);
      }
    } else if (njb->device_type == NJB_DEVICE_NJB1) {
      bwritten = njb_send_file_block(njb, &block[bp], xfersize);
    }

    if ( bwritten == -1 ) {
      close(fd);
      free(block);
      __leave;
      return -1;
    }

    /* On the series 3 devices, a send must be fully successful for us to 
     * proceed. We retry until finished! This hopefully makes it possible
     * to manipulate the NJB3_SEND_FILE_BLOCK_SIZE variable and even
     * autotune the max block size for a device.
     */
    if ((PDE_PROTOCOL_DEVICE(njb)) && (bwritten < xfersize)) {
      /* Do nothing, just resend. */
    }
    /* We use bwritten to cope with short transfers on the NJB1
     * and to decrease the counter for all devices. */
    else {
      remain -= (u_int64_t) bwritten;
      offset += (u_int64_t) bwritten;
      bp += bwritten;
    }
    
    if (PDE_PROTOCOL_DEVICE(njb)) {
      /* DO NOTHING */
    } else if (njb->device_type == NJB_DEVICE_NJB1) {
      if ( njb_verify_last_command(njb) == -1 ) {
	close(fd);
	free(block);
	
	__leave;
	return -1;
      }
    }

    if (callback != NULL) {
      if ( callback(offset, size, NULL, 0, data) == -1 )
	abortxfer = 1;
    }
  }
  
  free(block);
  close(fd);
	
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    /* Complete transfer for NJB3 series devices */
    if ( njb3_send_file_complete(njb, fileid) == -1 ) {
      __leave;
      return -1;
    }
    /* On abort, delete the partial file */
    if ( abortxfer ) {
      if ( njb3_delete_item(njb, fileid) == -1) {
	__leave;
	return -1;
      }
      NJB_ERROR(njb, EO_ABORTED);
      __leave;
      return -1;
    }
  } else {
    /* Complete transfer for NJB1  */
    while ( retry ) {
      if ( njb_transfer_complete(njb) == 0 ) {
	if ( abortxfer ) {
	  NJB_ERROR(njb, EO_ABORTED);
	  __leave;
	  return -1;
	}
	if ( _lib_ctr_update(njb) == -1 ) {
	  NJB_ERROR(njb, EO_BADCOUNT);
	  __leave;
	  return -1;
	}
	__leave;
	return 0;
      }
#ifdef __WIN32__
      /* 
       * This is because sleep() is actually deprecated in favor of
       * Sleep() on Windows.
       */
      _sleep(1);
#else
      sleep(1);
#endif
      retry--;
    }
    NJB_ERROR(njb, EO_TIMEOUT);
    __leave;
    return -1;
  }
  __leave;
  return 0;
}

/**
 * This sends ("downloads") a track (playable music file) to the 
 * device.
 *
 * Typical usage:
 *
 * <pre>
 * njb_t *njb;
 * njb_songid_t *songid;
 * njb_songid_frame_t *frame;
 * u_int32_t id;
 *
 * songid = NJB_Songid_New();
 * frame = NJB_Songid_Frame_New_Codec(NJB_CODEC_MP3);
 * NJB_Songid_Addframe(songid, frame);
 * // This one is optional - libnjb will fill it in if not specified
 * frame = NJB_Songid_Frame_New_Filesize(12345678);
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Title("MyTitle");
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Album("MyAlbum");
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Artist("MyArtist");
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Genre("MyGenre");
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Year(2004);
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Tracknum(1);
 * NJB_Songid_Addframe(songid, frame);
 * // The length of the track should typically be given, or such things
 * // as progress indicators will stop working. If you absolutely want to
 * // upload a file of unknown length and break progress indicators, set
 * // length to 1 second.
 * frame = NJB_Songid_Frame_New_Length(123);
 * NJB_Songid_Addframe(songid, frame);
 * // This one is optional - libnjb will fill it in if not specified
 * frame = NJB_Songid_Frame_New_Filename("Foo.mp3");
 * NJB_Songid_Addframe(songid, frame);
 * if (NJB_Send_Track (njb, "foo.mp3", songid, NULL, NULL, &id) == -1) {
 *     NJB_Error_Dump(stderr);
 * }
 * NJB_Songid_Destroy(songid);
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to send the
 *            track file to.
 * @param path the filesystem path on the local host to use as indata
 * @param songid the tag for this track, which has to be built separately
 *               before the transfer is started with a call to this function.
 * @param callback a function that will be called repeatedly to report
 *             progress during transfer, used for e.g. displaying
 *             progress bars. This may be NULL if you don't want any
 *             callbacks.
 * @param data a voluntary parameter that can associate some 
 *             user-supplied data with each callback call. It is OK
 *             to set this to NULL of course.
 * @param trackid a pointer to an integer that will hold the resulting
 *             track ID after this transfer has commenced successfully.
 * @return 0 on success, -1 on failure
 * @see NJB_Send_File()
 */
int NJB_Send_Track (njb_t *njb, const char *path, njb_songid_t *songid,
	NJB_Xfer_Callback *callback, void *data, u_int32_t *trackid)
{
  __dsub= "NJB_Send_Track";
  u_int64_t btotal, bfree, filesize;
  njb_songid_frame_t *frame;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (NJB_Get_Disk_Usage(njb, &btotal, &bfree) == -1) {
    NJB_ERROR(njb, EO_XFERDENIED);
    __leave;
    return -1;
  }
  
  if ( _file_size(njb, path, &filesize) == -1 ) {
    NJB_ERROR(njb, EO_SRCFILE);
    __leave;
    return -1;
  }
  
  if ( filesize > bfree ) {
    NJB_ERROR(njb, EO_TOOBIG);
    __leave;
    return -1;
  }
  
  /* Add file size if missing from songid */
  if ((frame = NJB_Songid_Findframe(songid, FR_SIZE)) == NULL) {
    u_int32_t tmpsize = (u_int32_t) filesize;
    frame = NJB_Songid_Frame_New_Filesize(tmpsize);
    NJB_Songid_Addframe(songid, frame);
  }
  
  /* Add filename if missing from songid */
  if ((frame = NJB_Songid_Findframe(songid, FR_FNAME)) == NULL) {
    /* Make a copy to be sure so as not to vandalize path */
    char *tmppath = strdup(path);
    char *bfname = basename(tmppath);
    frame = NJB_Songid_Frame_New_Filename(bfname);
    NJB_Songid_Addframe(songid, frame);
    free(tmppath);
  }
  
  /* Make sure the metadata is usable */
  if (songid_sanity_check(njb, songid) == -1) {
    NJB_ERROR(njb, EO_INVALID);
    __leave;
    return -1;
  }
  
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    /* Pack the tag and send it */
    unsigned char *ptag;
    njbttaghdr_t tagh;
    
    if ( (ptag = songid_pack(songid, &tagh.size)) == NULL ) return -1;
    
    if ( njb_send_track_tag(njb, &tagh, ptag) == -1 ) {
      NJB_ERROR(njb, EO_XFERDENIED);
      free(ptag);
      __leave;
      return -1;
    }
    
    free(ptag);
    
    *trackid = tagh.trackid;
    
    /* The trackid referenced is not actually used with the NJB1 */
    if ( send_file(njb, path, (u_int32_t) filesize, *trackid, callback, data, 0) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    unsigned char *ptag;
    u_int32_t tagsize;
    
    /* Request to stop playing before sending a track */
    njb3_ctrl_playing(njb, NJB3_STOP_PLAY);
    
    if ( (ptag = songid_pack3(songid, &tagsize)) == NULL ) {
      __leave;
      return -1;
    }
    if ( (*trackid = njb3_create_file(njb, ptag, tagsize, NJB3_TRACK_DATABASE)) == 0 ) {
      NJB_ERROR(njb, EO_XFERDENIED);
      free(ptag);
      __leave;
      return -1;
    }
    free(ptag);
    
    if ( send_file(njb, path, (u_int32_t) filesize, *trackid, callback, data, 0) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  __leave;
  return 0;
}

/**
 * This function sends a datafile to the device (downloads), optionally using a
 * folder name.
 *
 * @param njb a pointer to the <code>njb_t</code> object to send the
 *            file to
 * @param path a path to the file that shall be downloaded.
 * @param name a filename to use for this file on the device. Should only be the
 *             basename, excluding any folder name(s).
 * @param folder a folder name to use for this file on the device. Not all devices
 *        support folders, so this might be ignored. A folder name must begin and 
 *        end with backslash (\) and have levels of hierarchy separated by 
 *        backslashes too, like this: "\foo\bar\fnord\".
 * @param callback a function that will be called repeatedly to report
 *             progress during transfer, used for e.g. displaying
 *             progress bars. This may be NULL if you don't like callbacks.
 * @param data a voluntary parameter that can associate some 
 *             user-supplied data with each callback call. It is OK
 *             to set this to NULL of course.
 * @param fileid a pointer to a variable that will hold the new file ID when the
 *        operation is finished.
 * @return 0 on success, -1 on failure.
 */
 int NJB_Send_File (njb_t *njb, const char *path, const char *name, 
		    const char *folder, NJB_Xfer_Callback *callback, void *data, 
		    u_int32_t *fileid)
{
  __dsub= "NJB_Send_File";
  u_int64_t btotal, bfree, size;
  time_t ts;
  njb_datafile_t df;
  unsigned char *phdr;
  njbdfhdr_t fh;
  u_int32_t fhsize;
  int status;
  
  __enter;
  
  njb_error_clear(njb);
  
  memset(&df, 0, sizeof(df));
  
  if ( path == NULL ) {
    NJB_ERROR(njb, EO_INVALID);
    __leave;
    return -1;
  }
  
  if ( name == NULL ) {
    /* Make a copy to be sure so as not to vandalize path */
    char *tmppath = strdup(path);
    char *bfname = basename(tmppath);
    status = datafile_set_name(&df, bfname);
    free(tmppath);
  } else {
    status = datafile_set_name(&df, name);
  }
  
  /* Deafault to root folder if not available */
  if ( folder == NULL ) {
    status = datafile_set_folder(&df, "\\");
  } else {
    status = datafile_set_folder(&df, folder);	  
  }
  
  if ( status == -1 ) {
    NJB_Datafile_Destroy(&df);
    __leave;
    return -1;
  }
  
  if ( _file_size(njb, path, &size) == -1 ) {
    NJB_ERROR(njb, EO_SRCFILE);
    __leave;
    return -1;
  }
  if ( _file_time(njb, path, &ts) == -1 ) {
    NJB_ERROR(njb, EO_SRCFILE);
    __leave;
    return -1;
  }
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    if ( njb_get_disk_usage(njb, &btotal, &bfree) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if ( njb3_get_disk_usage(njb, &btotal, &bfree) == -1 ) {
      NJB_ERROR(njb, EO_XFERDENIED);
      __leave;
      return -1;
    }
  }
  
  if ( size > bfree ) {
    NJB_ERROR(njb, EO_TOOBIG);
    __leave;
    return -1;
  }
  
  datafile_set_size(&df, size);
  datafile_set_time(&df, ts);
  
  /* Pack the file tag and send it */
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    
    if ( (phdr = datafile_pack(&df, &fhsize)) == NULL ) return -1;
    fh.size = fhsize;
    
    if ( njb_send_datafile_tag(njb, &fh, phdr) == -1 ) {
      NJB_ERROR(njb, EO_XFERDENIED);
      free(phdr);
      __leave;
      return -1;
    }
    
    free(phdr);
    
    *fileid = fh.dfid;
    
    /* The fileid referenced is not actually used with the NJB1 */
    if ( send_file(njb, path, size, *fileid, callback, data, 0) == -1 ) {
      __leave;
      return -1;
    }
    
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    /* Pack the header */
    phdr = datafile_pack3(njb, &df, &fhsize);
    if ( phdr == NULL ) {
      __leave;
      return -1;
    }
    /* Create the file */
    *fileid = njb3_create_file(njb, phdr, fhsize, NJB3_FILE_DATABASE);
    if ( *fileid == 0 ) {
      NJB_ERROR(njb, EO_XFERDENIED);
      free(phdr);
      __leave;
      return -1;
    }
    /* FIXME: This may cause problems!! Experienced crashes here!! */
    /* Free the memory used by the packed header */
    free(phdr);
    /* Send the actual file */
    if ( send_file(njb, path, size, *fileid, callback, data, 0) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  __leave;
  return 0;
}

/**
 * This function creates a new folder on the device, if the device
 * supports folder creation.
 * @param njb a pointer to the <code>njb_t</code> jukebox object to use
 * @param name the name of the new folder to create
 * @param folderid a pointer to a variable that will hold the new
 *        item ID for the folder if it is successfully created.
 * @return 0 on success, -1 on failure. Notice that a case of failure
 *           is when an NJB1 is used, so this should normally result
 *           in "not implemented" error.
 */
int NJB_Create_Folder (njb_t *njb, const char *name, u_int32_t *folderid)
{
  __dsub = "NJB_Create_Folder";
  __enter;

  njb_error_clear(njb);

  if (njb->device_type == NJB_DEVICE_NJB1) {
    __leave;
    *folderid = 0;
    return -1;
  }
  else if (PDE_PROTOCOL_DEVICE(njb)) {
    int retval = njb3_create_folder(njb, name, folderid);
    __leave;
    return retval;
  }
  __leave;
  return -1;
}

/**
 * This function resets the retrieveal of EAX types.
 * It should typically be called before any subsequent
 * calls to <code>NJB_Get_EAX_Type()</code>.
 *
 * Typical usage:
 *
 * <pre>
 * njb_t *njb;
 * njb_eax_t *eax;
 * 
 * NJB_Reset_Get_EAX_Type(njb);
 * while ( (eax = NJB_Get_EAX_Type(njb)) != NULL ) {
 *    // Do something with all the EAX types...
 *    NJB_Destroy_EAX_Type(eax);
 * }
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to reset
 *            the EAX retrieveal pointer for
 * @see NJB_Get_EAX_Type()
 */
void NJB_Reset_Get_EAX_Type (njb_t *njb)
{
  __dsub= "NJB_Reset_Get_EAX_Type";
  __enter;

  njb_error_clear(njb);

  if (njb->device_type == NJB_DEVICE_NJB1) {
    u_int32_t eaxsz;
    if ( njb_get_eax_size(njb, &eaxsz) == -1 ) {
      __leave;
      return;
    }
    njb_read_eaxtypes(njb, eaxsz);
  }
  else if (PDE_PROTOCOL_DEVICE(njb)) {
    njb3_read_eaxtypes(njb);
  }
  __leave;
  return;
}

/**
 * This retrieves an EAX type from the device. EAX types include
 * volume controls, so this is essential for most player gadgets. 
 * Adjustment of the EAX controls according to the values found in
 * <code>njb_eax_t</code> objects is done using the 
 * <code>NJB_Ajust_EAX()</code> function.
 *
 * @param njb a pointer to the <code>njb_t</code> object to reset
 *            the EAX retrieveal pointer for
 * @return a pointer to a newly allocaed EAX type. This should
 *         be freed with <code>NJB_Destroy_EAX_Type()</code> after use.
 * @see NJB_Reset_Get_EAX_Type()
 * @see NJB_Adjust_EAX()
 * @see njb_eax_t
 */
njb_eax_t *NJB_Get_EAX_Type (njb_t *njb)
{
  __dsub= "NJB_Get_EAX_Type";
  __enter;

  njb_error_clear(njb);

  if (njb->device_type == NJB_DEVICE_NJB1) {
    njb_eax_t *eax = njb_get_nexteax(njb);
    __leave;
    return eax;
  }
  else if (PDE_PROTOCOL_DEVICE(njb)) {
    njb_eax_t *eax = njb3_get_nexteax(njb);
    __leave;
    return eax;
  }
  __leave;
  return NULL;
}

/**
 * This destroys an EAX Type and frees all memory used by it.
 *
 * @param eax the EAX Type to destroy
 */
void NJB_Destroy_EAX_Type (njb_eax_t *eax)

{
  __dsub= "NJB_Destroy_EAX_Type";
  __enter;
  destroy_eax_type(eax);
  __leave;
  return;
}

/**
 * This adjusts an EAX control. The magic numbers needed are to be
 * found in <code>njb_eax_t</code> structs retrived with the
 * <code>NJB_Get_EAX_Type()</code> function. In theory, EAX
 * controls could be both selectable (different <code>patchindex</code>
 * can be selected) and scaleable (different <code>scalevalue</code>
 * can be set) but in practice all controls are either selectable
 * or scaleable.
 *
 * @param njb a pointer to the <code>njb_t</code> object to manipulate
 *            the EAX settings on
 * @param eaxid a unique ID for the EAX effect to manipulate
 * @param patchindex a patch index to set for the EAX effect. If the
 *                   effect is not selectable, you can set this to 0.
 * @param scalevalue a scale value to set for the EAX effect. If the
 *                   effect is not scaleable, you can set this to 0.
 *                   Note that this is a signed integer, and may very 
 *                   well be supplied with negative numbers!
 * @see NJB_Get_EAX_Type()
 */
void NJB_Adjust_EAX (njb_t *njb, 
		     u_int16_t eaxid, 
		     u_int16_t patchindex,
		     int16_t scalevalue)
{
  __dsub= "NJB_Adjust_EAX";
  __enter;

  njb_error_clear(njb);

  if (njb->device_type == NJB_DEVICE_NJB1) {
    int16_t sendvalue;
    
    // We assume that a scalevalue != 0 means that this is
    // a scale to be modified.
    if (scalevalue != 0x0000) {
      sendvalue = scalevalue;
    } else {
      sendvalue = (int16_t) patchindex;
    }
    // Ignore return value
    njb_adjust_sound(njb, (u_int8_t) eaxid, sendvalue);
  }
  else if (PDE_PROTOCOL_DEVICE(njb)) {
    u_int16_t sendindex;
    u_int16_t active;
    njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

    /*
     * Volume is always active. 
     * The rest need to add additional control of the EAX processor 
     * so that it is activated/deactivated as needed.
     */
    if (eaxid == NJB3_VOLUME_FRAME_ID) {
      active = 0x0001;
    } else {
      if (patchindex == 0x0000 && 
	  scalevalue == 0x0000) {
	if (state->eax_processor_active) {
	  njb3_control_eax_processor(njb, 0x0000);
	  state->eax_processor_active = 0x00;
	}
	active = 0x0000;
      } else {
	if (!state->eax_processor_active) {
	  njb3_control_eax_processor(njb, 0x0001);
	  state->eax_processor_active = 0x01;
	}
	active = 0x0001;
      }
    }

    /* Patch 1 is actually patch 0, number 0 is a dummy for the "off" mode */
    if (patchindex > 0x0000) {
      sendindex = patchindex - 1;
    } else {
      sendindex = 0x0000;
    }
    njb3_adjust_eax(njb, eaxid, sendindex, active, scalevalue);
  }
  __leave;
  return;
}

/**
 * This returns the current time stamp for the device.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get
 *            the time from.
 * @return a valid time stamp or NULL on failure.
 */
njb_time_t *NJB_Get_Time(njb_t *njb)
{
  __dsub= "NJB_Get_Time";
  njb_time_t *time = NULL;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1)
    time = njb_get_time(njb);
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    time = njb3_get_time(njb);
  }
  
  if ( time == NULL ) {
    __leave;
    return NULL;
  }
  
  __leave;
  return time;
}

/**
 * This sets the current time stamp for the device. (Sets the
 * on-board clock.)
 *
 * @param njb a pointer to the <code>njb_t</code> object to set
 *            the timestamp on.
 * @param time the new timestamp to use (time to set).
 * @return 0 on success, -1 on failure.
 */
int NJB_Set_Time(njb_t *njb, njb_time_t *time)
{
  __dsub= "NJB_Set_Time";
  int ret;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    if (njb_set_time(njb, time) == -1) {
      __leave;
      return -1;
    }
    
    ret = njb_verify_last_command(njb);
    
    __leave;
    return ret;
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if (njb3_set_time(njb, time) == -1) {
      __leave;
      return -1;
    }
  }
  
  
  __leave;
  return 0;
}

/**
 * This destroys a time stamp and frees up any
 * memory used by it.
 *
 * @param time the time stamp to destroy.
 */
void NJB_Destroy_Time(njb_time_t *time)
{
  /* This is currently a very simple struct... */
  free(time);
}

/**
 * This deletes a playlist from the device.
 *
 * @param njb a pointer to the <code>njb_t</code> object
 *            to delete the playlist from
 * @param plid the playlist ID as reported from
 *            <code>NJB_Get_Playlist()</code>.
 * @return 0 on success, -1 on failure.
 */
int NJB_Delete_Playlist (njb_t *njb, u_int32_t plid)
{
  __dsub= "NJB_Delete_Playlist";
  int ret = 0;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    ret= njb_delete_playlist(njb, plid);
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret= njb3_delete_item(njb, plid);
  }
  
  __leave;
  return ret;
}

/**
 * This writes back an updated (modified) or new playlist
 * to the device.
 *
 * This function <i>may</i> modify the playlist ID, i.e.
 * the <code>plid</code> member of the 
 * <code>njb_playlist_t</code> struct, which means that if
 * your program has cached this number anywhere, you need
 * to update it using the value from <code>pl->plid</code> 
 * afterwards. This stems from the fact that playlists are
 * sometimes updated by deleting the old playlist and creating 
 * a new one.
 *
 * @param njb a pointer to the <code>njb_t</code> object
 *            to update the playlist on
 * @param pl the playlist to update.
 * @return 0 on success, -1 on failure.
 */
int NJB_Update_Playlist (njb_t *njb, njb_playlist_t *pl)
{
  __dsub= "NJB_Update_Playlist";
  u_int32_t *trids, *tptr;
  u_int32_t oplid = 0;
  njb_playlist_track_t *track;
  int state = pl->_state;
  int ret = 0;
  
  __enter;
  
  njb_error_clear(njb);
  
  /*
   * First the NJB1 specific playlist update code
   */
  if (njb->device_type == NJB_DEVICE_NJB1) {
    char *plname;
    
    switch (state) {
    case NJB_PL_NEW:
      /* Do nothing and fall through */
      break;
    case NJB_PL_UNCHANGED:
      /* Just return */
      __leave;
      return 0;
    case NJB_PL_CHNAME:
      /* Convert from UTF-8 if needed, then rename */
      if (njb_unicode_flag == NJB_UC_UTF8) {
	plname = utf8tostr((unsigned char *) pl->name);
      } else {
	plname = strdup(pl->name);
      }
      if (plname == NULL) {
	NJB_ERROR(njb, EO_NOMEM);
	__leave;
	return -1;
      }
      if ( njb_rename_playlist(njb, pl->plid, plname) == -1 ) {
	free(plname);
	__leave;
	return -1;
      }
      free(plname);
      __leave;
      return njb_verify_last_command(njb);
    case NJB_PL_CHTRACKS:
      oplid= pl->plid;
      if (oplid != 0) {
	/* Temporarily rename the playlist so we do not loose it if an error occurs */
	if ( njb_rename_playlist(njb, pl->plid, "dead.playlist") == -1 
	     || njb_verify_last_command(njb) == -1) { 
	  __leave;
	  return -1;
        }
      }
      break;
    }

    /* The following creates a new playlist on the NJB1 device */
    trids = (u_int32_t *) malloc(sizeof(u_int32_t)*pl->ntracks);
    if ( trids == NULL ) {
      NJB_ERROR(njb, EO_NOMEM);
      __leave;
      return -1;
    }
    
    NJB_Playlist_Reset_Gettrack(pl);
    tptr = trids;
    while ( (track = NJB_Playlist_Gettrack(pl)) != 0 ) {
      *tptr = track->trackid;
      tptr++;
    }
    
    /* Convert name from UTF-8 if needed and create it */
    if (njb_unicode_flag == NJB_UC_UTF8) {
      plname = utf8tostr((unsigned char *) pl->name);
    } else {
      plname = strdup(pl->name);
    }
    if (plname == NULL) {
      NJB_ERROR(njb, EO_NOMEM);
      __leave;
      return -1;
    }
    if ( njb_create_playlist(njb, plname, &pl->plid) == -1 ) {
      free(trids);
      __leave;
      return -1;
    }
    free(plname);
    
    if ( njb_add_multiple_tracks_to_playlist(njb, pl->plid, trids,
					     pl->ntracks) == -1 ) {
      
      free(trids);
      __leave;
      return -1;
    }
    
    free(trids);
    
    if ( state == NJB_PL_CHTRACKS && oplid != 0) {
      if ( njb_verify_last_command(njb) == -1 ) {
	__leave;
	return -1;
      }
      if ( njb_delete_playlist(njb, oplid) == -1 ) {
	__leave;
	return -1;
      }
    }
    ret = njb_verify_last_command(njb);
    /* The new playlist ID is now in pl->plid */
  }

  
  /*
   * Then the series 3 specific code
   */
  if (PDE_PROTOCOL_DEVICE(njb)) {
    unsigned char *tmpname;
    
    /* the name which need to be deallocated, that causes all the fuzz */
    tmpname = strtoucs2((unsigned char *) pl->name);
    
    if (tmpname == NULL) {
      NJB_ERROR(njb, EO_NOMEM);
      __leave;
      return -1;
    }
    
    switch (state) {
    case NJB_PL_NEW:
      /* Do nothing before creation */
      break;
    case NJB_PL_UNCHANGED:
      /* Nothing changed. Just return. */
      ret = 0;
      goto exit_njb3_playlist_update;
    case NJB_PL_CHNAME:
      /* Changed name, nothing else. */
      if ( njb3_update_string_frame(njb, pl->plid, NJB3_PLNAME_FRAME_ID, tmpname) == -1) {
	ret = -1;
	goto exit_njb3_playlist_update;
      }
      ret = 0;
      goto exit_njb3_playlist_update;
    case NJB_PL_CHTRACKS:
      /* If track contents are changed, then this deletes the old playlist. */
      oplid = pl->plid;
      if (oplid != 0) {
	if ( njb3_delete_item(njb, oplid) == -1 ) {
	  ret = -1;
	  goto exit_njb3_playlist_update;
	}
      }
    }
    
    /* The following code creates a new playlist on a series 3 device */
    trids = (u_int32_t *) malloc(sizeof(u_int32_t) * pl->ntracks);
    if ( trids == NULL ) {
      NJB_ERROR(njb, EO_NOMEM);
      ret = -1;
      goto exit_njb3_playlist_update;
    }

    /* Pick out the tracks from the playlist, one by one */
    NJB_Playlist_Reset_Gettrack(pl);
    tptr = trids;
    while ( (track = NJB_Playlist_Gettrack(pl)) != 0 ) {
      *tptr = track->trackid;
      tptr++;
    }
    
    if ( njb3_create_playlist(njb, (char *) tmpname, &pl->plid) == -1 ) {
      free(trids);
      ret = -1;
      goto exit_njb3_playlist_update;
    }
    
    if ( njb3_add_multiple_tracks_to_playlist(njb, &pl->plid, trids,
					      pl->ntracks) == -1 ) {
      
      free(trids);
      ret = -1;
      goto exit_njb3_playlist_update;
    }
    
    free(trids);
    
    ret = 0;
  exit_njb3_playlist_update:
    free(tmpname);
    
  }
  
  __leave;
  return ret;
}

/**
 * This deletes a track from the device.
 *
 * @param njb a pointer to the <code>njb_t</code> jukebox object to use
 * @param trackid the track ID for the track to delete
 * @return 0 on success, -1 on failure
 */
int NJB_Delete_Track (njb_t *njb, u_int32_t trackid)
{
  __dsub= "NJB_Delete_Track";
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    if ( njb_delete_track(njb, trackid) == -1 ) {
      __leave;
      return -1;
    }
    if ( _lib_ctr_update(njb) == -1 ) {
      NJB_ERROR(njb, EO_BADCOUNT);
      __leave;
      return -1;
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if ( njb3_delete_item(njb, trackid) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  __leave;
  return 0;
}

/**
 * This deletes a datafile from the device.
 *
 * @param njb a pointer to the <code>njb_t</code> jukebox object to use
 * @param fileid the file ID for the file to delete
 * @return 0 on success, -1 on failure
 */
int NJB_Delete_Datafile (njb_t *njb, u_int32_t fileid)
{
  __dsub= "NJB_Delete_Datafile";
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    if ( njb_delete_datafile(njb, fileid) == -1 ) {
      __leave;
      return -1;
    }
    
    if ( _lib_ctr_update(njb) == -1 ) {
      NJB_ERROR(njb, EO_BADCOUNT);
      __leave;
      return -1;
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if (njb3_delete_item (njb, fileid) == -1) {
      __leave;
      return -1;
    }
  }
  
  
  __leave;
  return 0;
}

/**
 * This starts playing a certain track on the device. This command
 * will implicitly clear any pending or playing playback queue.
 * You may queue up other songs using <code>NJB_Queue_Track()</code>
 * while the first track is playing. The track on which you called
 * <code>NJB_Play_Track()</code> will implicitly be placed first in
 * the queue.
 *
 * @param njb a pointer to the jukebox object to play the track on.
 * @param trackid the track to play.
 * @return 0 on success, -1 on failure.
 * @see NJB_Queue_Track()
 * @see NJB_Stop_Play()
 * @see NJB_Pause_Play()
 * @see NJB_Seek_Track()
 * @see NJB_Elapsed_Time()
 */
int NJB_Play_Track (njb_t *njb, u_int32_t trackid)
{
  __dsub= "NJB_Play_Track";
  int ret;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    if ( njb_play_track(njb, trackid) == -1 ) {
      __leave;
      return -1;
    }
    
    ret = njb_verify_last_command(njb);
    
    __leave;
    return ret;
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_clear_play_queue(njb);
    if (ret != 0) {
      __leave;
      return -1;
    }
    ret = njb3_turnoff_flashing(njb);
    if (ret != 0) {
      __leave;
      return -1;
    }
    ret = njb3_play_track (njb, trackid);
    __leave;
    return ret;
  }
  
  __leave;
  return 0;
}

/**
 * This adds a track to the play queue of a device.
 * 
 * @param njb a pointer to the jukebox object to play the track on.
 * @param trackid the track to add to the queue.
 * @return 0 on success, -1 on failure.
 */
int NJB_Queue_Track (njb_t *njb, u_int32_t trackid)
{
  __dsub= "NJB_Queue_Track";
  int ret;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    if ( njb_queue_track(njb, trackid) == -1 ) {
      __leave;
      return -1;
    }
    
    ret= njb_verify_last_command(njb);
    
    __leave;
    return ret;
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_queue_track (njb, trackid);
    __leave;
    return ret;
  }
  
  __leave;
  return 0;
}

/**
 * This pauses the current playback of a track on the device.
 *
 * @param njb a pointer to the jukebox object to play the track on.
 * @return 0 on success, -1 on failure.
 * @see NJB_Resume_Play()
 * @see NJB_Stop_Play()
 */
int NJB_Pause_Play (njb_t *njb)
{
  __dsub= "NJB_Pause_Play";
  int ret;

  __enter;

  njb_error_clear(njb);

  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_pause_play (njb);
    __leave;
    return ret;
  }

  __leave;
  return 0;
}

/**
 * This resumes play of a track on the device after pause.
 *
 * @param njb a pointer to the jukebox object to resume the track on.
 * @return 0 on success, -1 on failure.
 * @see NJB_Pause_Play()
 */
int NJB_Resume_Play (njb_t *njb)
{
  __dsub= "NJB_Pause_Play";
  int ret;

  __enter;

  njb_error_clear(njb);

  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_resume_play (njb);
    __leave;
    return ret;
  }

  __leave;
  return 0;
}

/**
 * This stops playback of a track on the device.
 *
 * @param njb a pointer to the jukebox object to stop 
 *            the current playing track on.
 * @return 0 on success, -1 on failure.
 * @see NJB_Pause_Play()
 */
int NJB_Stop_Play (njb_t *njb)
{
  __dsub= "NJB_Stop_Play";
  int ret;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_stop_play (njb);
    __leave;
    return ret;
  }
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    ret = njb_stop_play(njb);
    
    __leave;
    return ret;
  }
  
  __leave;
  return 0;
}

/**
 * This seeks into an offset of the currenly playing track
 * on the device. You can skip to an offset forward/backward 
 * in the currently playing file, given as milliseconds.
 *
 * @param njb a pointer to the jukebox object to stop 
 *            the current playing track on.
 * @param position the position in the track file given
 *            as milliseconds. Offsets larger than the file length
 *            should not be used.
 * @return 0 on success, -1 on failure.
 */
int NJB_Seek_Track (njb_t *njb, u_int32_t position)
{
  __dsub= "NJB_Seek_Track";
  int ret;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_seek_track (njb, position);
    __leave;
    return ret;
  }
  
  __leave;
  return 0;
}

/**
 * This function returns the elapsed time for the currently playing
 * track on the device. Additionally, it signals if the track
 * has changed, e.g. if the device has skipped to the next track
 * in the queue.
 *
 * Currently, it may only be determined if the currently playing track
 * in the Nomad's play queue has changed (or ended) from a previous call
 * to NJB_Elapsed_Time, because the indicator is that the elapsed time
 * goes from a greater number to a lower number. Typically, 
 * when the first (zeroth) track in the play queue ends, the elapsed time
 * is set to zero, and the change bit is set.
 * However, if the zeroth track is the only track on the play queue,
 * it must be more than 1 second long and NJB_Elapsed_Time must have been
 * called at least once after 1 second actually has elapsed to register
 * the change (song end) on a subsequent call to NJB_Elapsed_Time.
 *
 * NOTE, the elapsed time will NOT always be zero when
 * the change bit is set, as a call to NJB_Elapsed_Time may arrive late
 * after a song has started.
 *
 * NOTE, after NJB_Play_Track is called an immediate
 * call NJB_Elapsed_Time will not indicate change, even if there was
 * a track playing previously.
 *
 * Obviously, this notification of change scheme is a flawed one,
 * and some Nomad play queue management functions, such as finding
 * out the actual trackid playing on the Nomad would be favored over
 * this approach. One should not really rely too heavily on this 
 * approach.
 * 
 * @param njb a pointer to the jukebox object that the track 
 *            is playing on.
 * @param elapsed a pointer to a variable that will hold the number
 *            of elapsed seconds after the call to this function.
 * @param change a pointer to a variable that will hold 0 if the
 *            track has not changed, and something different from
 *            0 if the track has changed.
 * @return 0 on success, -1 on failure.
 */
int NJB_Elapsed_Time (njb_t *njb, u_int16_t *elapsed, int *change)
{
  __dsub= "NJB_Elapsed_Time";
  int ret;
  
  __enter;
  
  njb_error_clear(njb);
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    ret = njb3_elapsed_time (njb, elapsed, change);
    __leave;
    return ret;
  }
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    ret = njb_elapsed_time(njb, elapsed, change);
    
    __leave;
    return ret;
  }
  
  __leave;
  return 0;
}

/**
 * This is a helper function to retrieve the time stamp for
 * a file on the host system.
 */
int _file_time (njb_t *njb, const char *path, time_t *ts)
{
  __dsub = "_file_time";
  struct stat sb;
  
  __enter;
  
  if ( stat(path, &sb) == -1 ) {
    NJB_ERROR3(njb, "stat", path, -1);
    __leave;
    return -1;
  }
  
  *ts = sb.st_mtime;
  
  __leave;
  return 0;
}

/**
 * This is a helper function for retrieveing the file size for
 * a file on the host system.
 */
int _file_size (njb_t *njb, const char *path, u_int64_t *size)
{
  __dsub= "_file_size";
  struct stat sb;
  
  __enter;
  
  if ( stat(path, &sb) == -1 ) {
    NJB_ERROR3(njb, "stat", path, -1);
    __leave;
    return -1;
  }
  
  *size = sb.st_size;
  
  __leave;
  return 0;
}

/**
 * Set the debug print mode for libnjb. The debug flag is created
 * by OR:ing up the different possible flags.
 *
 * @see debugflags
 * @param debug_flags the debug flags to use
 */
void NJB_Set_Debug (int debug_flags)
{
	njb_set_debug(debug_flags);
}

/**
 * Defines the encoding used by libnjb. By default, ISO 8859-1
 * (or rather Windows Codepage 1252) will be used. However, all
 * modern applications should make a call to this function and
 * set the encoding to <code>NJB_UC_UTF8</code>.
 *
 * @see unicodeflags
 * @param unicode_flag the encoding to use
 */
void NJB_Set_Unicode (int unicode_flag)
{
	njb_set_unicode(unicode_flag);
}

/**
 * This routine will replace the track tag or parts of a track tag for a track 
 * that already exist on the device. On the NJB1 the whole metadata set must be
 * specified for this routine to work properly, but on the series 3 devices
 * you can specify incremental updates (only parts of the metadata set). Be sure
 * to either specify the full set all the time, or check if we are handling an
 * NJB1 before submitting an incremental update (see example below).
 * 
 * @param njb a pointer to the jukebox object to use
 * @param trackid the track ID to replace the tag for
 * @param songid the new tag
 *
 * Typical usage:
 *
 * <pre>
 * njb_t *njb;
 * njb_songid_t *songid;
 * njb_songid_frame_t *frame;
 *
 * songid = NJB_Songid_New();
 * // On NJB1 incremental update is not possible, so a full
 * // metadata set must always be specified.
 * if (njb->device_type == NJB_DEVICE_NJB1) {
 *   frame = NJB_Songid_Frame_New_Codec(meta->codec);
 *   NJB_Songid_Addframe(songid, frame);
 *   frame = NJB_Songid_Frame_New_Filesize(meta->size);
 *   NJB_Songid_Addframe(songid, frame);
 * }
 * frame = NJB_Songid_Frame_New_Codec(NJB_CODEC_MP3);
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Title("MyTitle");
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Album("MyAlbum");
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Artist("MyArtist");
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Genre("MyGenre");
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Year(2004);
 * NJB_Songid_Addframe(songid, frame);
 * frame = NJB_Songid_Frame_New_Tracknum(1);
 * NJB_Songid_Addframe(songid, frame);
 * // The length of the track should typically be given, or such things
 * // as progress indicators will stop working. If you absolutely want to
 * // upload a file of unknown length and break progress indicators, set
 * // length to 1 second.
 * frame = NJB_Songid_Frame_New_Length(123);
 * NJB_Songid_Addframe(songid, frame);
 * // This one is optional, the track will survive without it.
 * frame = NJB_Songid_Frame_New_Filename("Foo.mp3");
 * NJB_Songid_Addframe(songid, frame);
 * if (NJB_Replace_Track_Tag(njb, 123456, songid) == -1) {
 *    NJB_Error_Dump(stderr);
 * }
 * NJB_Songid_Destroy(songid);
 * </pre>
 */
int NJB_Replace_Track_Tag (njb_t *njb, u_int32_t trackid, njb_songid_t *songid)
{
  __dsub= "NJB_Replace_Track_Tag";
  
  __enter;
  
  njb_error_clear(njb);
  
  /*
   * This routine toggles all changed string frames to a
   * different name, as changing the name back and forth
   * to the same value seems to cause trouble. (Originally
   * Friso Brugmans invention.)
   */
  if (PDE_PROTOCOL_DEVICE(njb)) {
    unsigned char *ptag = NULL;
    u_int32_t ptagsize = 0;
    njb_songid_t *tmpsong;
    njb_songid_frame_t *frame;
    
    /*
     * Spin through frames, build a new songid_t and send,
     * this must be done on all string typed frames or
     * the device will go bananas 
     */
    tmpsong = NJB_Songid_New();
    NJB_Songid_Reset_Getframe(songid);
    while ( (frame = NJB_Songid_Getframe(songid)) ) {
      njb_songid_frame_t *tmpframe = NULL;
      
      /*
       * FIXME: experiment with only making this second
       * update if and only if the tag contain string
       * frames. Should work just fine.
       */
      if (!strcmp(frame->label, FR_CODEC)) {
	/* 
	 * Be sure to avoid the CODEC here, the pack routine translates
	 * the CODEC frame string into a number and thus it should not be
	 * manipulated this way.
	 */
	tmpframe = NJB_Songid_Frame_New_String(frame->label, frame->data.strval);
      } else if (frame->type == NJB_TYPE_STRING) {
	char *tmpstring = NULL;
	
	tmpstring = malloc(strlen(frame->data.strval)+6);
	strcpy(tmpstring, frame->data.strval);
	strcat(tmpstring, ".temp");
	tmpframe = NJB_Songid_Frame_New_String(frame->label, tmpstring);
	free(tmpstring);
      } else if (frame->type == NJB_TYPE_UINT16) {
	tmpframe = NJB_Songid_Frame_New_Uint16(frame->label, frame->data.u_int16_val);
      } else if (frame->type == NJB_TYPE_UINT32) {
	tmpframe = NJB_Songid_Frame_New_Uint32(frame->label, frame->data.u_int32_val);
      }
      NJB_Songid_Addframe(tmpsong, tmpframe);
    }
    /* Rename temporarily */
    if ((ptag = songid_pack3(tmpsong, &ptagsize)) == NULL ) {
      __leave;
      return -1;
    }
    if (njb3_update_tag(njb, trackid, ptag, ptagsize) == -1) {
      free(ptag);
      __leave;
      return -1;
    }
    free(ptag);
    NJB_Songid_Destroy(tmpsong);
  }
  
  if (njb->device_type == NJB_DEVICE_NJB1) {
    unsigned char *ptag;
    njbttaghdr_t tagh;
    
    /* Make sure the metadata is usable for NJB1 */
    if (songid_sanity_check(njb, songid) == -1) {
      NJB_ERROR(njb, EO_BAD_NJB1_REPLACE);
      __leave;
      return -1;
    }
    
    /* Now pack the tag and send it */
    
    if ( (ptag = songid_pack(songid, &tagh.size)) == NULL ) return -1;
    tagh.trackid = trackid;
    
    if ( njb_replace_track_tag(njb, &tagh, ptag) == -1 ) {
      free(ptag);
      __leave;
      return -1;
    }
    
    free(ptag);
    
    if ( _lib_ctr_update(njb) == -1 ) {
      NJB_ERROR(njb, EO_BADCOUNT);
      __leave;
      return -1;
    }
    
    __leave;
    return 0;
  }
  
  /*
   * Rewrite of the above routine by Friso Brugmans to pack edited frames
   * of a metadata into one single command block.
   */
  if (PDE_PROTOCOL_DEVICE(njb)) {
    unsigned char *ptag = NULL;
    u_int32_t ptagsize = 0;
    
    /* Whole section needs checking for null strings from strtoucs2,
     * and -1 return codes from njb3_update_x_tagframe */
    
    /* Remove filesize from tag */
    
    if ((ptag = songid_pack3(songid, &ptagsize)) == NULL ) {
      __leave;
      return -1;
    }
    if (njb3_update_tag(njb, trackid, ptag, ptagsize) == -1) {
      free(ptag);
      __leave;
      return -1;
    }
    free(ptag);
  }
  __leave;
  return 0;
}

/**
 * <b>EXPERIMENTAL:</b>
 * This function returns the dimensions for the bitmap image
 * on the device, in case the bitmap can be set. Returns -1
 * if the device does not support setting the bitmap. Currently
 * this command only supports sending bitmap to the NJB2, other
 * models are unsupported, so this needs some development.
 *
 * @param njb a pointer to the jukebox object to use
 * @param x Number of pixels on the X axis
 * @param y Number of pixels on the Y axis
 * @param bytes number of bytes for the entire bitmap image
 * @return 0 on success, -1 if setting bitmap is not possible
 * @see NJB_Set_Bitmap()
 */
int NJB_Get_Bitmap_Dimensions(njb_t *njb, int *x, int *y, int *bytes)
{
  __dsub = "NJB_Get_Bitmap_Dimensions";

  __enter;
  if (njb->device_type == NJB_DEVICE_NJB2 ||
      njb->device_type == NJB_DEVICE_NJB3 ||
      njb->device_type == NJB_DEVICE_NJBZEN ||
      njb->device_type == NJB_DEVICE_NJBZEN2 ||
      njb->device_type == NJB_DEVICE_NJBZENNX) {
    *x = 132;
    *y = 64;
    *bytes = 1056;
    __leave;
    return 0;
  } else if (njb->device_type == NJB_DEVICE_NJBZENXTRA ||
	     njb->device_type == NJB_DEVICE_NJBZENTOUCH) {
    njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
    /*
     * This feature is in fact broken in later 
     * NJB Zen Touch firmware since version 1.01.06
     */
    if (njb->device_type == NJB_DEVICE_NJBZENTOUCH &&
	state->fwMajor >= 1 &&
	state->fwMinor >= 1 &&
	state->fwRel >= 6 ) {
      /* Signal broken firmware. */
      return -1;
    }
    /*
     * Else it is supported at this size:
     */
    *x = 160;
    *y = 104;
    *bytes = 16640;
    __leave;
    return 0;
  }
  /* Else we say it is unsupported */
  __leave;
  return -1;
}

/**
 * <b>EXPERIMENTAL:</b>
 * This sets the bitmap (boot-up logo) on the device. 
 * It is currently experimental and not all devices support changing
 * the bitmap.
 *
 * @param njb a pointer to the jukebox object to use
 * @param bitmap A raw bitmap image to send to the device. Note that
 *               this image shall have the dimensions indicated by
 *               a previous NJB_Get_Bitmap_Dimensions() call.
 * @return 0 on success, -1 on failure
 * @see NJB_Get_Bitmap_Dimensions()
 */
int NJB_Set_Bitmap(njb_t *njb, const unsigned char *bitmap) {
  __dsub = "NJB_Set_Bitmap";

  __enter;

  njb_error_clear(njb);

  if (PDE_PROTOCOL_DEVICE(njb)) {
    int x, y, bytes;

    /* We need the dimensions */
    if ( NJB_Get_Bitmap_Dimensions(njb, &x, &y, &bytes) == -1) {
      __leave;
      return -1;
    }
    
    if ( njb3_set_bitmap(njb, x, y, bitmap) == -1 ) {
      __leave;
      return -1;
    }
  }
  
  __leave;
  return 0;
}

/**
 * This sends a ping ("are you there?") command to the
 * device.
 *
 * @param njb a pointer to the jukebox object to ping.
 */
void NJB_Ping(njb_t *njb)
{
  __dsub= "NJB_Ping";

  __enter;

  njb_error_clear(njb);

  if (njb->device_type == NJB_DEVICE_NJB1) {
    if ( njb_ping(njb) == -1 ) {
      __leave;
      return;
    }
  }
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    if ( njb3_ping(njb, 0) == -1 ) {
      __leave;
      return;
    }
  }
  
  __leave;
}

/**
 * On series 3 devices, this command retrieves some
 * key/value pairs that are believed to be used for
 * DRM schemes. We have no idea of how to use these, just
 * that they make possible to do proper signing of 
 * protected WMA files before transfer.
 *
 * @param njb a pointer to the jukebox object to get the keys
 *            from.
 */
njb_keyval_t *NJB_Get_Keys(njb_t *njb)
{
  __dsub= "NJB_Get_Keys";

  __enter;

  njb_error_clear(njb);
  
  if (PDE_PROTOCOL_DEVICE(njb)) {
    __leave;
    return njb3_get_keys(njb);
  }
  __leave;
  return NULL;
}

/**
 * This retrieves the library counter for the Nomad Jukebox
 * 1 (D.A.P., the original). The library counter can be used to
 * validate a track cache for this device, but not for any other
 * devices. If you have a cache with all tracks, datafiles
 * and playlists (in some self-defined format) you can store 
 * it along with this number when your program exits. When you 
 * restart your program you can check if the numbers are still 
 * the same, and if they are, you can keep using your old
 * track/file/playlist cache and need not re-read that information.
 *
 * Note that you have to take action to make sure that you do not
 * accidentally lock out series 3 devices! If this function returns
 * zero, always re-read the entire library.
 *
 * @param njb a pointer to the jukebox object to get the library
 *            counter from.
 * @return a library counter, if this is 0, the tracks, playlists
 *         and datafiles need to be re-read, i.e. the cached 
 *         database is invalid.
 */
u_int64_t NJB_Get_NJB1_Libcounter(njb_t *njb)
{
  __dsub= "NJB_Get_NJB1_Libcounter";

  __enter;

  if (njb->device_type == NJB_DEVICE_NJB1) {
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    /* This is set to the state in NJB_Handshake() */
    __leave;
    return state->libcount;
  }
  __leave;
  return 0;
}

/**
 * <b>EXPERIMENTAL:</b>
 * This function sends a new firmware to the device.
 *
 * DO NOT TRY OR USE THIS FUNCTION UNLESS YOU ARE 100 PERCENT SURE OF
 * WHAT YOU ARE DOING, IT IS VITALLY DANGEROUS TO YOUR DEVICE AND MAY
 * RENDER IT COMPLETELY USELESS IF USED WITH INVALID DATA.
 *
 * @param njb a pointer to the <code>njb_t</code> object to send the
 *            file to
 * @param path a path to the firmware file that shall be downloaded. This
 *             file must be the <i>raw firmware chunks</i> as sent across
 *             the USB bus to the device.
 * @param callback a function that will be called repeatedly to report
 *             progress during transfer, used for e.g. displaying
 *             progress bars. This may be NULL if you don't like callbacks.
 * @param data a voluntary parameter that can associate some 
 *             user-supplied data with each callback call. It is OK
 *             to set this to NULL of course.
 * @return 0 on success, -1 on failure.
 */
int NJB_Send_Firmware (njb_t *njb, const char *path, NJB_Xfer_Callback *callback, void *data)
{
  __dsub= "NJB_Send_File";
  u_int64_t filesize;
  __enter;

  if ( path == NULL ) {
    NJB_ERROR(njb, EO_INVALID);
    __leave;
    return -1;
  }
  
  if ( _file_size(njb, path, &filesize) == -1 ) {
    NJB_ERROR(njb, EO_SRCFILE);
    __leave;
    return -1;
  }

  /*
   * This only works on series 3 devices as of now.
   */
  if (PDE_PROTOCOL_DEVICE(njb)) {
    /* First tell the device how large this firmware is */
    if ( njb3_announce_firmware(njb, (u_int32_t) filesize) == -1) {
      __leave;
      return -1;
    }
    /* The send the whole shebang */
    if ( send_file(njb, path, (u_int32_t) filesize, 0, callback, data, 1) == -1 ) {
      __leave;
      return -1;
    }
    /* Then verify the transfer */
    if (njb3_get_firmware_confirmation(njb) == -1) {
      __leave;
      return -1;
    }
  }
  
  __leave;
  return 0;
}

/**
 * This function returns the current battery level between 0 and 100.
 * The hardware will typically prevent the device from working properly
 * if the battery level is too low, so figures above 50 are the most 
 * common.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get the 
 *        battery level for.
 * @return the battery level between 0 and 100, or -1 if the function
 *         call fails.
 */
int NJB_Get_Battery_Level (njb_t *njb)
{
  int ret = -1;
    
  if (njb->device_type == NJB_DEVICE_NJB1) {
    u_int8_t power_status;
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    /* Re-read the power status */
    NJB_Ping(njb);
    power_status = state->power;
    /* We can only detect full battery and charging on the NJB1 */
    if (power_status == NJB_POWER_AC_CHARGED) {
      ret = 100;
    } 
  } else  if (PDE_PROTOCOL_DEVICE(njb)) {
    int battery_level, charging, ac_power;

    ret = njb3_power_status(njb, &battery_level, &charging, &ac_power);
    if (ret != -1) {
      ret = battery_level;
    }
  }
  return ret;
}

/**
 * This function determines if we are charging the battery or not.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get the
 *        charging status for.
 * @return 1 if we are charging, 0 if we're not charging, and -1 if
 *         the call failed.
 */
int NJB_Get_Battery_Charging (njb_t *njb)
{
  int ret = -1;
    
  if (njb->device_type == NJB_DEVICE_NJB1) {
    u_int8_t power_status;
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    /* Re-read the power status */
    NJB_Ping(njb);
    power_status = state->power;
    if (power_status == NJB_POWER_AC_CHARGING) {
      ret = 1;
    } else {
      ret = 0;
    }
  } else  if (PDE_PROTOCOL_DEVICE(njb)) {
    int battery_level, charging, ac_power;

    ret = njb3_power_status(njb, &battery_level, &charging, &ac_power);
    if (ret != -1) {
      ret = charging;
    }
  }
  return ret;
}

/**
 * This function determines if we are on auxiliary power (charger connected)
 * or not. (It will also signify if the USB power is connected on devices
 * that support it.) One most typical use of this function is to determine the
 * status before any firmware upgrade commence.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get the
 *        auxiliary power status for.
 * @return 1 if we are connected to auxiliary power, 0 if we're not, and -1 if
 *         the call failed.
 */
int NJB_Get_Auxpower (njb_t *njb)
{
  int ret = -1;
    
  if (njb->device_type == NJB_DEVICE_NJB1) {
    u_int8_t power_status;
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    
    /* Re-read the power status */
    NJB_Ping(njb);
    power_status = state->power;
    if (power_status == NJB_POWER_AC_CHARGED ||
	power_status == NJB_POWER_AC_CHARGING) {
      ret = 1;
    } else {
      ret = 0;
    }
  } else  if (PDE_PROTOCOL_DEVICE(njb)) {
    int battery_level, charging, ac_power;

    ret = njb3_power_status(njb, &battery_level, &charging, &ac_power);
    if (ret != -1) {
      ret = ac_power;
    }
  }
  return ret;
}

/**
 * This function returns the unique SDMI ID of 16 bytes
 * for a jukebox device. Example:
 *
 * <pre>
 * u_int8_t sdmiid[16];
 * int result;
 * result = NJB_Get_SDMI_ID(njb, &sdmiid);
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to get the
 *        SDMI ID for.
 * @param sdmiid a pointer to a byte array of 16 bytes (declare this
 *        as <code>u_int8_t foo[16];</code>) that will hold the SDMI
 *        ID if the function completed successfully.
 * @return 0 on success, -1 on failure.
 */
int NJB_Get_SDMI_ID(njb_t *njb, u_int8_t *sdmiid)
{
  /* Zero the return value by default */
  memset(sdmiid, 0, 16);
  if (njb->device_type == NJB_DEVICE_NJB1) {
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    memcpy(sdmiid, &state->sdmiid, 16);
  } else  if (PDE_PROTOCOL_DEVICE(njb)) {
    if (njb3_readid(njb, sdmiid) == -1) {
      return -1;
    }
  } else {
    return -1;
  }
  return 0;
}

/**
 * This function returns the name of the jukebox device as
 * a string.
 *
 * <pre>
 * char *name;
 * name = NJB_Get_Device_Name(njb, 0);
 * if (name != NULL) {
 *   printf("Device name: %s\n", name);
 * }
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to get the
 *        device name for.
 * @param type decides whether libnjb should decide what kind of device
 *        this is, or if the string from the device itself should be
 *        used. 0 = use libnjb detection (based on USB device IDs),
 *        1 = get the string from the device itself. If you set this
 *        to type 1, you must first call <code>NJB_Open()</code> on
 *        the <code>njb_t</code> object pointer, so this is not 
 *        suitable when you want to display a selection list of available
 *        devices prior to opening one of them.
 * @return a pointer to a string that contains the device 
 *        name string after a call to this function. The string 
 *        <b>must not</b> be freed by the caller, because this is a pointer 
 *        into a string holder inside libnjb! On error, NULL will be
 *        returned.
 */
const char *NJB_Get_Device_Name(njb_t *njb, int type)
{
  char *name = NULL;
  
  if ((type != 0) && (type != 1)) {
    return NULL;
  }

  /*
   * Get static information about the device as determined
   * by the USB ID information.
   */
  if (type == 0) {
    name = njb_get_usb_device_name(njb);
  }

  /* Else actually ask the device what it is */
  if (type == 1) {
    if (njb->device_type == NJB_DEVICE_NJB1) {
      njb_state_t *state = (njb_state_t *) njb->protocol_state;
      name = state->productName;
    } else  if (PDE_PROTOCOL_DEVICE(njb)) {
      njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
      name = state->product_name;
    }
  }
  return name;
}

/**
 * This function returns the firmware revision for a certain jukebox
 * device. The <code>release</code> parameter will not be valid for
 * the NJB1 since this only has two version numbers. Example usage:
 *
 * <pre>
 * uint8 major;
 * uint8 minor;
 * uint8 release;
 *
 * NJB_Get_Firmware_Revision(njb, &major, &minor, &release);
 * printf("Firmware revision: %d.%d.%d\n", major, minor, release);
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to get the
 *        firmware revision for.
 * @param major will hold the major revision number for the firmware
 *        after the call, if the call was successful.
 * @param minor will hold the minor revision number for the firmware
 *        after the call, if the call was successful.
 * @param release will hold the release number for the firmware
 *        after the call, if the call was successful.
 * @return 0 if the call was successful, -1 on failure.
 */
int NJB_Get_Firmware_Revision(njb_t *njb, u_int8_t *major, u_int8_t *minor, u_int8_t *release)
{
  if (njb->device_type == NJB_DEVICE_NJB1) {
    njb_state_t *state = (njb_state_t *) njb->protocol_state;
    *major = state->fwMajor;
    *minor = state->fwMinor;
    *release = 0;
    return 0;
  } else  if (PDE_PROTOCOL_DEVICE(njb)) {
    njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
    *major = state->fwMajor;
    *minor = state->fwMinor;
    *release = state->fwRel;
    return 0;
  }
  return -1;
}

/**
 * This function returns the hardware revision for a certain jukebox
 * device. This information is hardcoded to "1.0.0" for the NJB1
 * since that device does not support retrieving the hardware revision.
 *
 * @param njb a pointer to the <code>njb_t</code> object to get the
 *        hardware revision for.
 * @param major will hold the major revision number for the hardware
 *        after the call, if the call was successful.
 * @param minor will hold the minor revision number for the hardware
 *        after the call, if the call was successful.
 * @param release will hold the release number for the hardware
 *        after the call, if the call was successful.
 * @return 0 if the call was successful, -1 on failure.
 */
int NJB_Get_Hardware_Revision(njb_t *njb, u_int8_t *major, u_int8_t *minor, u_int8_t *release)
{
  if (njb->device_type == NJB_DEVICE_NJB1) {
    /* The NJB1 does not have this information */
    *major = 1;
    *minor = 0;
    *release = 0;
    return 0;
  } else  if (PDE_PROTOCOL_DEVICE(njb)) {
    njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
    *major = state->hwMajor;
    *minor = state->hwMinor;
    *release = state->hwRel;
    return 0;
  }
  return -1;
}


/**
 * This function sets the turbo mode to on or off. The default
 * value if the function is not called is that turbo is ON.
 * We have found that a few (very few) devices will react badly
 * on turbo mode, resulting in bad transfers. This setting is
 * only applicable on the series 3 devices, it will have no effect
 * on the NJB1. (Command available as of libnjb 2.2.4.)
 *
 * Example usage:
 * <pre>
 * NJB_Set_Turbo_Mode(njb, NJB_TURBO_OFF);
 * </pre>
 *
 * @param njb a pointer to the <code>njb_t</code> object to set
 *            the turbo mode for.
 * @param mode the turbo mode. <code>NJB_TURBO_ON</code> or
 *            <code>NJB_TURBO_OFF</code>.
 * @return 0 if the call was successful, -1 on failure.
 */
int NJB_Set_Turbo_Mode(njb_t *njb, u_int8_t mode)
{
  /* The turbo mode setting is silently ignored for the NJB1 */
  if (PDE_PROTOCOL_DEVICE(njb)) {
    njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

    state->turbo_mode = mode;
  }
  return 0;
}

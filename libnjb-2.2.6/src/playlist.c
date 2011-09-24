/**
 * \file playlist.c
 *
 * This file contains the functions dealing with playlists.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libnjb.h"
#include "njb_error.h"
#include "defs.h"
#include "base.h"
#include "unicode.h"
#include "byteorder.h"
#include "playlist.h"

extern int njb_unicode_flag;
extern int __sub_depth;

/**
 * This function creates a new playlist data structure to
 * hold a name and a number of tracks.
 *
 * @return a new playlist structure
 */
njb_playlist_t *NJB_Playlist_New(void)
{
  __dsub= "NJB_Playlist_New";
  njb_playlist_t *pl;
  
  __enter;
  
  pl = (njb_playlist_t *) malloc(sizeof(njb_playlist_t));
  if ( pl == NULL ) {
    __leave;
    return NULL;
  }
  
  memset(pl, 0, sizeof(njb_playlist_t));
  pl->_state = NJB_PL_NEW;
  
  __leave;
  return pl;
}

/**
 * This function unpacks packed playlist data from the
 * NJB1. Not applicable for other jukeboxes.
 *
 * @param data a chunk of raw data to process
 * @param nbytes the size of the data chunk
 * @return a new playlist structure with correct name
 *         and all tracks added.
 */
njb_playlist_t *playlist_unpack(void *data, size_t nbytes)
{
  __dsub= "playlist_unpack";
  unsigned char *dp= (unsigned char *) data;
  size_t index;
  njb_playlist_t *pl;
  u_int32_t ntracks, i;
  u_int16_t lname;
  
  __enter;
  
  pl = NJB_Playlist_New();
  if ( pl == NULL ) {
    __leave;
    return NULL;
  }
  
  pl->plid = njb1_bytes_to_32bit(&dp[0]);
  lname = njb1_bytes_to_16bit(&dp[4]);
  
  if (njb_unicode_flag == NJB_UC_UTF8) {
    char *utf8str = NULL;
    
    utf8str = strtoutf8(&dp[6]);
    if (utf8str == NULL) {
      NJB_Playlist_Destroy(pl);
      __leave;
      return NULL;
    }
    pl->name = utf8str;
  } else {
    pl->name = strdup((char *) &dp[6]);
    if ( pl->name == NULL ) {
      NJB_Playlist_Destroy(pl);
      __leave;
      return NULL;
    }
  }
  
  index = lname+12;
  ntracks = njb1_bytes_to_32bit(&dp[index]);
  index += 4;
  
  for (i = 0; i<ntracks; i++) {
    u_int32_t trackid;
    njb_playlist_track_t *track;
    
    index += 4;
    trackid = njb1_bytes_to_32bit(&dp[index]);
    index += 4;
    if ( index > nbytes ) {
      NJB_Playlist_Destroy(pl);
      __leave;
      return NULL;
    }
    
    track = NJB_Playlist_Track_New(trackid);
    if ( track == NULL ) {
      NJB_Playlist_Destroy(pl);
      __leave;
      return NULL;
    }
    
    NJB_Playlist_Addtrack(pl, track, NJB_PL_END);
  }

  pl->_state = NJB_PL_UNCHANGED;
  __leave;
  return pl;
}

/**
 * This function adds a playlist track (a data structure representing a
 * track that is part of a playlist) to a playlist data structure.
 *
 * @param pl the playlist that the track shall be added to
 * @param track the track to add
 * @param pos the position in the playlist where this track should be
 *            added
 * @see NJB_Playlist_Deltrack()
 */
void NJB_Playlist_Addtrack(njb_playlist_t *pl, njb_playlist_track_t *track, 
			   unsigned int pos)
{
  __dsub= "NJB_Playlist_Addtrack";
  
  __enter;
  
  if ( pl->_state != NJB_PL_NEW ) pl->_state= NJB_PL_CHTRACKS;
  
  if ( pos > pl->ntracks ) pos = NJB_PL_END;
  
  if ( pos == NJB_PL_END ) {
    if ( pl->first == NULL ) {
      pl->first= pl->cur= pl->last= track;
      track->next= track->prev= NULL;
    } else {
      track->prev= pl->last;
      track->next= NULL;
      pl->last->next= track;
      pl->last= track;
    }
  } else if ( pos == NJB_PL_START ) {
    if ( pl->first == NULL ) {
      pl->first= pl->cur= pl->last= track;
      track->next= track->prev= NULL;
    } else {
      track->prev= NULL;
      track->next= pl->first;
      pl->first->prev= track;
      pl->first= track;
    }
  } else {
    int i = 1;
    njb_playlist_track_t *cur;
    
    NJB_Playlist_Reset_Gettrack(pl);
    while ( (cur= NJB_Playlist_Gettrack(pl)) ) {
      if ( i == pos ) {
	cur->prev->next= track;
	track->prev= cur->prev;
	track->next= cur;
	cur->prev= track;
	
	pl->ntracks++;
	
	__leave;
	return;
      }
      
      i++;
    }
  }
  
  pl->ntracks++;
  __leave;
}

/**
 *  This function deletes a track from a playlist.
 *
 * @param pl the playlist that the track shall be removed from
 * @param pos the position in the playlist to be deleted
 * @see NJB_Playlist_Addtrack()
 * @see NJB_Playlist_Deltrack_TrackID()
 */
void NJB_Playlist_Deltrack(njb_playlist_t *pl, unsigned int pos)
{
  __dsub = "NJB_Playlist_Deltrack";
  njb_playlist_track_t *track;
  
  __enter;
  
  if ( pos > pl->ntracks ) pos = NJB_PL_END;
  
  pl->_state = NJB_PL_CHTRACKS;
  
  if ( pos == NJB_PL_START ) {
    track = pl->first;
    pl->first = pl->first->next;
    if (pl->first != NULL) {
      pl->first->prev = NULL;
    }
  } else if ( pos == NJB_PL_END ) {
    track = pl->last;
    pl->last = pl->last->prev;
    if (pl->last != NULL) {
      pl->last->next = NULL;
    }
  } else {
    int i = 1;
    
    NJB_Playlist_Reset_Gettrack(pl);
    while ( (track = NJB_Playlist_Gettrack(pl)) ) {
      if ( i == pos ) {
	if ( track->prev != NULL ) {
	  track->prev->next = track->next;
	}
	if ( track->next != NULL ) {
	  track->next->prev = track->prev;
	}
	
	NJB_Playlist_Track_Destroy(track);
	pl->ntracks--;
	
	__leave;
	return;
      }
      i++;
    }
  }
  
  NJB_Playlist_Track_Destroy(track);
  pl->ntracks--;
  
  __leave;
}

/**
 * This function removes a track from a playlist by way of the
 * track ID (as opposed to the position in the playlist). This
 * function can be called even on playlists that don't have this
 * track in them - this is useful for e.g. looping through all
 * playlists and removing a certain track before deleting the 
 * track itself from the device.
 *
 * You need to call the <code>NJB_Update_Playlist()</code> 
 * function for each playlist that has been manipulated by this
 * function, to assure that any changes are written back to
 * the playlist on the device.
 *
 * Typical use to remove a track with ID <code>id</code> from
 * all playlists on a device:
 *
 * <pre>
 * njb_playlist_t *playlist;
 *
 * NJB_Reset_Get_Playlist(njb);
 * while (playlist = NJB_Get_Playlist(njb)) {
 *    NJB_Playlist_Deltrack_TrackID(playlist, id);
 *    // If the playlist changed, update it
 *    if (NJB_Update_Playlist(njb, playlist) == -1)
 *        NJB_Error_Dump(njb, stderr);
 *    }
 *    NJB_Playlist_Destroy(playlist);
 * }
 * if (NJB_Error_Pending(njb)) {
 *    NJB_Error_Dump(njb, stderr);
 * }
 * </pre>
 *
 * @param pl the playlist that the track shall be removed from,
 *           if present.
 * @param trackid the track ID to remove from this playlist
 * @see NJB_Playlist_Deltrack()
 * @see NJB_Update_Playlist()
 */
void NJB_Playlist_Deltrack_TrackID(njb_playlist_t *pl, u_int32_t trackid)
{
  njb_playlist_track_t *track;

  NJB_Playlist_Reset_Gettrack(pl);
  while ( (track = NJB_Playlist_Gettrack(pl)) != NULL ) {
    if (trackid == track->trackid) {
      /* When the track is located in a playlist, remove it */
      if (track->prev != NULL) {
	track->prev->next = track->next;
      } else {
	pl->first = track->next;
      }
      if (track->next != NULL) {
	track->next->prev = track->prev;
      }
      NJB_Playlist_Track_Destroy(track);
      pl->ntracks--;
      pl->_state = NJB_PL_CHTRACKS;
    }
  }
}

/**
 * This function destroys a playlist and frees up the memory
 * used by it. All tracks that are part of the playlist will
 * also be destroyed.
 * 
 * @param pl the playlist to destroy
 */
void NJB_Playlist_Destroy(njb_playlist_t *pl)
{
  __dsub = "NJB_Playlist_Destroy";
  njb_playlist_track_t *pltrack;
  
  __enter;
  
  pl->cur = pl->first;
  while ( pl->cur != NULL ) {
    pltrack = pl->cur;
    pl->cur = pl->cur->next;
    NJB_Playlist_Track_Destroy(pltrack);
  }
  
  if ( pl->name != NULL ) {
    free(pl->name);
  }
  
  free(pl);
  
  __leave;
}

/**
 * Resets the internal counter used when retrieveing tracks
 * from a playlist. Should typically be called first, before
 * any subsequent calls to NJB_Playlist_Gettrack().
 *
 * @param pl the playlist to be reset
 * @see NJB_Playlist_Gettrack()
 */
void NJB_Playlist_Reset_Gettrack (njb_playlist_t *pl)
{
  __dsub = "NJB_Playlist_Reset_Gettrack";
  
  __enter;
  
  pl->cur = pl->first;
  
  __leave;
}

/**
 * Returns a track from a playlist. The playlist has an internal
 * structure to keep track of the constituent tracks, so the tracks
 * will be retrieved in order. This function should typically
 * be called repeatedly after an initial 
 * <code>NJB_Playlist_Reset_Gettrack()</code> call.
 *
 * @param pl the playlist to get tracks from.
 * @return a track or NULL of the last track from a playlist has 
 *         already been returned
 * @see NJB_Playlist_Reset_Gettrack()
 */
njb_playlist_track_t *NJB_Playlist_Gettrack(njb_playlist_t *pl)
{
  __dsub = "NJB_Playlist_Gettrack";
  njb_playlist_track_t *track;
  
  __enter;
  
  if ( pl->cur == NULL ) {
    __leave;
    return NULL;
  }
  
  track = pl->cur;
  pl->cur = pl->cur->next;
  
  __leave;
  return track;
}

/**
 * This function sets the name of the playlist. The name will be
 * duplicated and stored internally, so the string is only needed
 * during the function call.
 *
 * @param pl the playlist to set the name for
 * @param name the name to set for the playlist
 */
int NJB_Playlist_Set_Name(njb_playlist_t *pl, const char *name)
{
  __dsub = "NJB_Playlist_Set_Name";
  char *newname = strdup(name);
  
  __enter;
  
  if (newname == NULL ) {
    __leave;
    return -1;
  }
  
  if ( pl->name != NULL ) free(pl->name);
  pl->name = newname;
  
  if ( pl->_state == NJB_PL_UNCHANGED ) pl->_state = NJB_PL_CHNAME;
  
  __leave;
  return 0;
}

/**
 * This creates a new track entry for playlists. The 
 * <code>trackid</code> used should be the same as retrieved 
 * from libnjb track reading functions.
 *
 * @param trackid the ID of the new track
 * @return the new playlist track entry
 */
njb_playlist_track_t *NJB_Playlist_Track_New(u_int32_t trackid)
{
  __dsub = "NJB_Playlist_Track_New";
  njb_playlist_track_t *track;
  
  __enter;
  
  track = (njb_playlist_track_t *) malloc(sizeof(njb_playlist_track_t));
  if ( track == NULL ) {
    __leave;
    return NULL;
  }
  
  memset(track, 0, sizeof(njb_playlist_track_t));
  track->trackid = trackid;
  
  __leave;
  return track;
}

/**
 * This destroys a playlist track entry and frees any memory
 * used by it.
 *
 * @param track the track entry to destroy.
 */
void NJB_Playlist_Track_Destroy(njb_playlist_track_t *track)
{
  __dsub= "NJB_Playlist_Track_Destroy";
  
  __enter;
  free(track);
  __leave;
}


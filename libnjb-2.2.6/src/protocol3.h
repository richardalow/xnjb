/**
 * \file protocol3.h
 * This is the main header file for the "series 3" protocol
 * that is believed to have the internal name "PDE" at Creative,
 * an acronym that probably reads out "Personal Digital 
 * Entertainment".
 *
 * The protocol has three distinct namespaced ("enum series") that
 * are used for communications. These are:
 *
 * 1. Command namespace, all commands that can be sent are 
 *   uniquely enumerated.
 * 2. Device register and frame namespace: all kind of metadata,
 *   database data and device registers (!) are enumerated in the
 *   same namespace, making very different things share the same
 *   number series.
 * 3. Return types (error codes).
 * 4. Codecs.
 * 5. File types (or databases), it's just track, file or playlist
 *   really.
 *
 * Known commands (first 16 bits)
 *
 * 0x0001 Update database item metadata on files, tracks, playlists also
 *        used for adding smartvolume information to tracks
 * 0x0002 Get file/track chunk
 * 0x0003 Send file/track chunk
 * 0x0004 Create database file item (files and tracks)
 * 0x0005 Delete database file item (files and tracks)
 * 0x0006 Read database        tracks, datafiles, playlists, all the
 *                             commands seen for this code has produced
 *                             a dump of the entire database so these are
 *                             all-or-nothing operations.
 * 0x0007 Set device register  - owner string, current track position,
 *                             play status (pause/stop), set time,
 *                             EAX processor mode (on/off)
 * 0x0008 Request device register - codecs, device info, ID, fw version, 
 *                             hw version, disk usage, owner string,
 *                             time, current track position, play status
 *                             (pause/stop), number of tracks, albums or
 *                             playlists, battery status.
 * 0x0009 Send file/track complete
 * 0x000a Create folder or playlist (empty, non-file item entry in database)
 * 0x000b Load device firmware (bitmaps and firmware upgrade)
 * 0x000c Get device keys, returns a number of device-unique keys.
 * 0x0010 Verify file??
 * 0x0100 Playback track (on the device)
 * 0x0101 Get elapsed time for current track, just 4 bytes 0101 0001
 * 0x0103 Clear playback queue
 * 0x0104 Enqueue track for playing
 * 0x0107 Add tracks to playlist
 * 0x0108 Get tracks for playlist
 * 0x0200 Get EAX effect settings
 * 0x0201 Adjust EAX setting
 */

#ifndef __NJB__PROTO3__H
#define __NJB__PROTO3__H

#include "libnjb.h"

/* Buffer for short reads */
#define NJB3_SHORTREAD_BUFSIZE 1024

/* Transfer block size (adding and fetching tracks, files */
#define NJB3_CHUNK_SIZE 0x100000U
#define NJB3_FIRMWARE_CHUNK_SIZE 0x40000U
#define NJB3_DEFAULT_GET_FILE_BLOCK_SIZE    0x2000U
#define NJB3_DEFAULT_SEND_FILE_BLOCK_SIZE   0x2000U

/*
 * These are the frame IDs used by the series 3 devices.
 * The frames are used for all the commands listed above, so
 * the same number series will contain very different things
 * like different kind of device properties and parameters 
 * (e.g. disk utilization) and metadata tag elements (e.g.
 * "artist field"). The only way to see what a frame actually
 * represents is to see how it is used.
 */
#define NJB3_CODECS_FRAME_ID    0x0001U /* 16 bit array R: List of supported audio file types/codecs */
#define NJB3_DISKUTIL_FRAME_ID  0x0002U /* 14 bytes R: disk utilization information */
#define NJB3_PRODID_FRAME_ID    0x0003U /* 3 bytes FW rev 3 bytes HW rev, string product ID */
/* 
 * 0x0004 and 0x0005 return NULL when read as device 
 * parameters, are probably used as some kind of 
 * metadata frames. 
 */
#define NJB3_LOCKED_FRAME_ID    0x0006U /* 16 bit word */
#define NJB3_FNAME_FRAME_ID     0x0007U /* String: Original filename on host */
#define NJB3_UNKNOWN1_FRAME_ID  0x0008U /* 16 bit word, value 0x0004 on NJB Zen USB 2.0, not read by Win SW */
#define NJB3_KEY_FRAME_ID       0x000aU /* 4 bytes string "AR00", "PL00", "SG00", "LG00" known */
#define NJB3_CODEC_FRAME_ID     0x000bU /* 16 bit word */
#define NJB3_POSTID_FRAME_ID    0x000cU /* 16 bit word TrackID on tracks, PlaylistID on playlists */
#define NJB3_DIR_FRAME_ID       0x000dU /* String: Original directory on host */
#define NJB3_FILESIZE_FRAME_ID  0x000eU /* 32 bit word */
/*
 * 0x0010 and 0x0011 return a 16-bit value of 0x0002 
 * respective 0x0200 when read as device parameters.
 * e.g. 0000 0004 0011 0200 0000
 * 0x0011 may be sector size or USB xfer block size...
 */
#define NJB3_FILECOUNT_FRAME_ID 0x0013U /* 32 bit word R: file & directory count */
#define NJB3_VALUE_FRAME_ID     0x0014U /* 8 bytes, 2*32 bit words */
#define NJB3_JUKEBOXID_FRAME_ID 0x0015U /* 16 bytes R: unique device ID */
#define NJB3_FILETIME_FRAME_ID  0x0016U /* 32 bit word - timestamp (UNIX format) */
#define NJB3_UNKNOWN6_FRAME_ID  0x0017U /* 32 bit word - could be FAT32 attributes */
#define NJB3_FILEFLAGS_FRAME_ID 0x0018U /* 32 bit word set on files and folders - NTFS file attributes */
/*
 * 0x0100 is some kind of 6-byte device parameter, value
 * from NJB Zen USB 2.0: 0001 0000 03ff
 */
#define NJB3_ALBUM_FRAME_ID     0x0101U /* String */
#define NJB3_ARTIST_FRAME_ID    0x0102U /* String */
#define NJB3_GENRE_FRAME_ID     0x0103U /* String */
#define NJB3_TITLE_FRAME_ID     0x0104U /* String */
#define NJB3_LENGTH_FRAME_ID    0x0105U /* String */
#define NJB3_TRACKNO_FRAME_ID   0x0106U /* 16 bit word */
#define NJB3_YEAR_FRAME_ID      0x0107U /* 16 bit word */
/*
 * Smartvolume parameters for a track is sent after a track with the command:
 * 0x0001 0x0001 update file with info
 * 0x0000 0x0000 4 bytes trackid
 * 0x01b6 (length)
 * 0x010a (smartvol)
 * 0x01b6 unknown bytes, appears to be an array of 109 32-bit words.
 * 0x0000 terminator
 */
#define NJB3_SMARTPAR_FRAME_ID  0x010aU
/*
 * 16 bit word R/W: Info about current track playing on the device 
 * First byte appear to always be 0x0b, second means the following:
 * 0x00 = play/playing
 * 0x01 = stop/stopped
 * 0x02 = pause/paused
 * 0x03 = resume (write only)
 */
#define NJB3_PLAYINFO_FRAME_ID  0x010bU
#define NJB3_SEEKTRACK_FRAME_ID 0x010cU /* 32 bit word W: seek to position (in ms) in current track */
#define NJB3_EAX_TYPENAME       0x010eU /* String with the name of an EAX type */
#define NJB3_PLNAME_FRAME_ID    0x010fU /* String: playlist name */
/*
 * 8 bytes R/W, BCD-encoded time and date:
 * byte: meaning:
 * 00: day of the week (0 = sunday)
 * 01: day of the month (1-31)
 * 02: month of the year (1-12)
 * 03: year century digit (20 mostly...)
 * 04: year lower part
 * 05: hours
 * 06: minutes
 * 07: seconds
 * e.g.: 0619 1120 0518 4351,
 * means saturday, 2005-11-19, 18:43:51
 */
#define NJB3_TIME_FRAME_ID      0x0110U
#define NJB3_ALBUMCNT_FRAME_ID  0x0111U /* 32 bit word R: number of albums on device */
#define NJB3_TRACKCNT_FRAME_ID  0x0112U /* 32 bit word R: number of tracks on device */
#define NJB3_OWNER_FRAME_ID     0x0113U /* String: owner name */
/*
 * 4 bytes: 0x14, one byte charging status then 16 bits of battery level 
 * The first byte appear to always contain 0x14.
 * The second value is interpreted as follows:
 * 0x00 = power connected and charging
 * 0x01 = power connected, battery fully charged
 * 0x02 = power disconnected
 * Then follows the battery level as a 16bit value.
 */
#define NJB3_BATTERY_FRAME_ID   0x0114U
#define NJB3_PLCNT_FRAME_ID     0x0115U /* 32 bit word: number of playlists on device */
/*
 * 0x0116-0x0118 can be read as device registers.
 * they will return one 32 bit word each, this is
 * what it looks like on NJB Zen USB 2.0 in rest
 * mode:
 *
 * 0x0116: 00000000
 * 0x0117: 00000000
 * 0x0118: ffffffff
 * 
 */
#define NJB3_PLAYTRACK_FRAME_ID 0x0119U /* 32 bit word track ID to play */
/*
 * Seen after clearing the playback queue in this sequence:
 * 0007 0001 0004 011a 1a00 0000 reading it out gives the 
 * same value so probably the writing of it is useless.
 */
#define NJB3_UNKNOWN4_FRAME_ID  0x011aU
#define NJB3_PLTRACKS_FRAME_ID  0x011cU /* Array of 16bit words */
/*
 * 0x011d - 0x0120 return sensible, static but undeciphered values
 * when read out as device registers. Sample values from NJB Zen USB 2.0:
 * 0x011d: 00000407
 * 0x011e: 00000070
 * 0x011f: 1f00 1f01 1f02 1f03
 * 0x0120: 0001 fffe 0011
 */
#define NJB3_MINMAX_ID          0x0201U /* 2x16 bit values, max and min */
#define NJB3_EAX_ACTIVE_ID      0x0202U /* 16 bit word - this EAX type is active/to 
					 * be activated (0x0000 = off, 0x0001 = on) */
#define NJB3_VOLUME_FRAME_ID    0x0203U /* 16 bit word */
#define NJB3_ENV_FRAME_ID       0x0204U /* 16 bit word - environment setting */
#define NJB3_EQ_FRAME_ID        0x0205U /* 16 bit word - equalizer setting */
#define NJB3_SPAT_FRAME_ID      0x0206U /* 16 bit word - spatialization 2 = full */
#define NJB3_TSCALE_FRAME_ID    0x0207U /* 16 bit word - time scaling factor */
#define NJB3_SMARTVOL_FRAME_ID  0x0208U /* 16 bit word - smart volume setting */
#define NJB3_EAXACTIVE_FRAME_ID 0x020aU /* 16 bit word - 0x0000/0x0001 = activate/deactivate EAX processor */
#define NJB3_EAXID_FRAME_ID     0x020bU /* 16 bit word with the numerical ID of a certain EAX type */
#define NJB3_EAX_INDEX_ID       0x020cU /* 16 bit word - currently selected effect in a set of effects */
#define NJB3_KEYVALUE_FRAME_ID  0x1400U /* Array of value-key-pairs, requested in a 
					  subrequest parameter to this request */

/* Database IDs, used for eg create_file */
#define NJB3_FILE_DATABASE       0x0000U
#define NJB3__PLAYLIST_DATABASE  0x0001U
#define NJB3_TRACK_DATABASE      0x0002U

/* Codec IDs */
#define NJB3_CODEC_MP3_ID_OLD        0x0000U /* Used on NJB3/Zen FW? */
#define NJB3_CODEC_WAV_ID            0x0001U
#define NJB3_CODEC_MP3_ID            0x0002U
#define NJB3_CODEC_WMA_ID            0x0003U
/* 
 * 0x0004, 0x0005 and 0x0006 unknown, one of them is 
 * undoubtedly the Real Networks AAC + Helix DRM 
 * decoder. Only very certain firmwares will support
 * these I believe...
 */
#define NJB3_CODEC_AA_ID             0x0007U /* Audible.com codec */
#define NJB3_CODEC_PROTECTED_WMA_ID  0x0203U /* Is it two bytes actually? */

/* Stop, pause and resume are very much alike. */
#define NJB3_START_PLAY     0x00
#define NJB3_STOP_PLAY      0x01
#define NJB3_PAUSE_PLAY     0x02
#define NJB3_RESUME_PLAY    0x03

/* Status codes */
#define NJB3_STATUS_OK                0x0000U
#define NJB3_STATUS_EMPTY             0x0001U /* You tried to retrieve an empty item */
#define NJB3_STATUS_TRANSFER_ERROR    0x0002U /* Error during read or write */
#define NJB3_STATUS_BAD_FILESIZE      0x0003U /* Illegal file size (e.g. negative, too large) */
#define NJB3_STATUS_NOTIMPLEMENTED    0x0004U /* For example if EAX is not supported on a device */
#define NJB3_STATUS_NOTEXIST          0x0005U /* Tried to access nonexistant track */
#define NJB3_STATUS_PROTECTED         0x000cU /* Tried to access protected object */
#define NJB3_STATUS_EMPTY_CHUNK       0x000eU /* Appear when requesting empty metadata lists
					       * or beyond the end of files. */

/*
 * Status codes that must exist, find by trial-and-error:
 * - postid invalid (does not exist)
 * - disk full
 * - file path / track name etc too long 
 * - playing (cannot transfer when playing)
 * - tried to skip to position outside file in playback mode
 * - too many files/tracks etc - limit reached.
 * - filename or track+artist+(all metadata) is the same as
 *   one already present on the player
 * - transfer in progress (a track/file is transferring in
 *   either direction
 */

#define njb3_start_play(njb) njb3_ctrl_playing(njb, NJB3_START_PLAY)
#define njb3_stop_play(njb) njb3_ctrl_playing(njb, NJB3_STOP_PLAY)
#define njb3_pause_play(njb) njb3_ctrl_playing(njb, NJB3_PAUSE_PLAY)
#define njb3_resume_play(njb) njb3_ctrl_playing(njb, NJB3_RESUME_PLAY)

/* Structure to hold protocol3 states */
typedef struct {
  /* Get extended tags */
  int get_extended_tag_info;
  njb_songid_t *first_songid;
  njb_songid_t *next_songid;
  njb_playlist_t *first_plid;
  njb_playlist_t *next_plid;
  njb_datafile_t *first_dfid;
  njb_datafile_t *next_dfid;
  int current_playing_track;
  njb_keyval_t *first_key;
  njb_keyval_t *next_key;
  njb_eax_t *first_eax;
  njb_eax_t *next_eax;
  /** If the EAX processor is active or not */
  u_int8_t eax_processor_active;
  /** A string with the product name */
  char *product_name;
  /** Firmware major revision */
  u_int8_t fwMajor;
  /** Firmware minor revision */
  u_int8_t fwMinor;
  /** Firmware release */
  u_int8_t fwRel;
  /** Hardware major revision */
  u_int8_t hwMajor;
  /** Hardware minor revision */
  u_int8_t hwMinor;
  /** Hardware release */
  u_int8_t hwRel;
  /** The last call to njb3_elapsed_time */
  u_int16_t last_elapsed;
  /** Turbo or no turbo mode */
  u_int8_t turbo_mode;
} njb3_state_t;


/* 
 * NJB3 functions: the commands listed at the top of this
 * file are wrapped in binary form more or less using the defined
 * values in this file to conjure and decipher byte-sequences
 * sent by the devices.
 */
int njb3_init_state (njb_t *njb);
int njb3_set_bitmap(njb_t *njb, u_int16_t x_size, u_int16_t y_size, const unsigned char *bitmap);
int njb3_current_track (njb_t *njb, u_int16_t * track);
int njb3_elapsed_time (njb_t *njb, u_int16_t * elapsed, int * change);
int njb3_play_track (njb_t *njb, u_int32_t trackid);
int njb3_queue_track (njb_t *njb, u_int32_t trackid);
int njb3_clear_play_queue(njb_t *njb);
int njb3_ctrl_playing (njb_t *njb, int cmd);
int njb3_seek_track (njb_t *njb, u_int32_t position);
int njb3_get_codecs(njb_t *njb);
int njb3_ping (njb_t *njb, int type);
int njb3_power_status (njb_t *njb, int *battery_level, int *charging, int *ac_power);
int njb3_readid (njb_t *njb, u_int8_t *sdmiid);
int njb3_capture (njb_t *njb);
int njb3_release (njb_t *njb);
int njb3_get_disk_usage (njb_t *njb, u_int64_t *totalbytes, u_int64_t *freebytes);
int njb3_turnoff_flashing(njb_t *njb);
int njb3_get_owner_string (njb_t *njb, char *name);
int njb3_set_owner_string (njb_t *njb, const char *name);
njb_time_t *njb3_get_time(njb_t *njb);
int njb3_set_time(njb_t *njb, njb_time_t *time);
int njb3_reset_get_track_tag (njb_t *njb);
njb_songid_t *njb3_get_next_track_tag (njb_t *njb);
int njb3_reset_get_playlist_tag (njb_t *njb);
njb_playlist_t *njb3_get_next_playlist_tag (njb_t *njb);
int njb3_reset_get_datafile_tag (njb_t *njb);
njb_datafile_t *njb3_get_next_datafile_tag (njb_t *njb);
int njb3_read_keys(njb_t *njb);
njb_keyval_t *njb3_get_keys(njb_t *njb);
int njb3_request_file_chunk(njb_t *njb, u_int32_t fileid, u_int32_t offset);
int njb3_get_file_block(njb_t *njb, unsigned char *data, u_int32_t maxsize);
u_int32_t njb3_create_file(njb_t *njb, unsigned char *ptag, u_int32_t tagsize, u_int16_t database);
u_int32_t njb3_send_file_chunk(njb_t *njb, unsigned char *chunk, u_int32_t chunksize, u_int32_t fileid);
int njb3_send_file_complete(njb_t *njb, u_int32_t fileid);
int njb3_create_folder(njb_t *njb, const char *name, u_int32_t *folderid);
int njb3_delete_item(njb_t *njb, u_int32_t itemid);
int njb3_update_16bit_frame(njb_t *njb, u_int32_t itemid, u_int16_t frameid, u_int16_t value);
int njb3_update_string_frame(njb_t *njb, u_int32_t itemid, u_int16_t frameid, unsigned char *str);
int njb3_update_tag(njb_t *njb, u_int32_t trackid, unsigned char *ptag, u_int32_t ptagsize);
int njb3_create_playlist(njb_t *njb, char *name, u_int32_t *plid);
int njb3_add_multiple_tracks_to_playlist (njb_t *njb, u_int32_t *plid, u_int32_t *trids, u_int16_t ntracks);
int njb3_adjust_volume(njb_t *njb, u_int16_t value);
int njb3_control_eax_processor (njb_t * njb, u_int16_t state);
int njb3_adjust_eax(njb_t *njb, u_int16_t eaxid, u_int16_t patchindex, u_int16_t active, u_int16_t scalevalue);
void njb3_read_eaxtypes(njb_t *njb);
njb_eax_t *njb3_get_nexteax(njb_t *njb);
int njb3_announce_firmware(njb_t *njb, u_int32_t size);
u_int32_t njb3_send_firmware_chunk(njb_t *njb, u_int32_t chunksize, unsigned char *chunk);
int njb3_get_firmware_confirmation(njb_t *njb);
void njb3_destroy_state(njb_t *njb);

#endif

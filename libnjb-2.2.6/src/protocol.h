#ifndef __NJB__PROTO__H
#define __NJB__PROTO__H

#include "libnjb.h"

typedef struct {
	unsigned char id[16];
	u_int64_t count;
} njblibctr_t;

typedef struct {
	u_int32_t trackid;
	u_int32_t size;
} njbttaghdr_t;

typedef struct {
	u_int32_t plid;
	u_int32_t size;
} njbplhdr_t;

typedef struct {
	u_int32_t dfid;
	u_int32_t size;
} njbdfhdr_t;


/* Structure to hold protocol states */
typedef struct {
  int session_updated;
  u_int64_t libcount;
  njb_eax_t *first_eax;
  njb_eax_t *next_eax;
  int reset_get_track_tag;
  int reset_get_playlist;
  int reset_get_datafile_tag;
  /** If the power cord is plugged in or not */
  u_int8_t power;
  /** The SDMI ID */
  u_int8_t sdmiid[16];
  /** A string with the product name */
  char productName[33];
  u_int8_t fwMajor; /**< Firmware major revision */
  u_int8_t fwMinor; /**< Firmware minor revision */
} njb_state_t;

/*
 * These defines are only applicable to the NJB1.
 */
#define NJB_POWER_BATTERY       0x00 /**< If the device is running on battery */
#define NJB_POWER_AC_CHARGED    0x01 /**< If the device is running on DC 
					  and the battery is fully charged */
#define NJB_POWER_AC_CHARGING   0x03 /**< If the device is running on DC
					  and the battery is being charged */

#define NJB_XFER_BLOCK_SIZE	        0x0000FE00
#define NJB_XFER_BLOCK_HEADER_SIZE      68

#define NJB_RELEASE	0x00
#define NJB_CAPTURE	0x01

#define	NJB_CMD_PING				0x01
#define NJB_CMD_GET_DISK_USAGE			0x04
#define NJB_CMD_GET_FIRST_TRACK_TAG_HEADER	0x06
#define NJB_CMD_GET_NEXT_TRACK_TAG_HEADER	0x07
#define NJB_CMD_GET_TRACK_TAG			0x09
#define NJB_CMD_SEND_TRACK_TAG			0x0a
#define NJB_CMD_DELETE_TRACK			0x0b
#define NJB_CMD_REPLACE_TRACK_TAG		0x0c
#define NJB_CMD_REQUEST_TRACK			0x0d
#define NJB_CMD_TRANSFER_COMPLETE		0x0e
#define NJB_CMD_SEND_FILE_BLOCK			0x0f
#define NJB_CMD_RECEIVE_FILE_BLOCK		0x10
#define NJB_CMD_GET_FIRST_PLAYLIST_HEADER	0x11
#define NJB_CMD_GET_NEXT_PLAYLIST_HEADER	0x12
#define NJB_CMD_GET_PLAYLIST			0x14
#define NJB_CMD_CREATE_PLAYLIST			0x15
#define NJB_CMD_DELETE_PLAYLIST			0x16
#define NJB_CMD_ADD_TRACK_TO_PLAYLIST		0x17
#define NJB_CMD_QUEUE_TRACK			0x1b
#define NJB_CMD_PLAY_TRACK			0x1d
#define NJB_CMD_STOP_PLAY			0x1e
#define NJB_CMD_ELAPSED_TIME			0x22
#define NJB_CMD_ADJUST_SOUND			0x23
#define NJB_CMD_GET_TIME			0x29
#define NJB_CMD_SET_TIME			0x2a
#define NJB_CMD_CAPTURE_NJB			0x2b
#define NJB_CMD_RELEASE_NJB			0x2c
#define NJB_CMD_GET_EAX_SIZE			0x3a
#define NJB_CMD_GET_EAX				0x24
#define NJB_CMD_RENAME_PLAYLIST			0x40
#define NJB_CMD_SET_OWNER_STRING		0x41
#define NJB_CMD_GET_OWNER_STRING		0x42
#define NJB_CMD_GET_LIBRARY_COUNTER		0x43
#define NJB_CMD_SET_LIBRARY_COUNTER		0x44
#define NJB_CMD_ADD_MULTIPLE_TRACKS_TO_PLAYLIST	0x46
#define NJB_CMD_SEND_DATAFILE_TAG		0x48
#define NJB_CMD_DELETE_DATAFILE			0x49
#define NJB_CMD_GET_FIRST_DATAFILE_HEADER	0x4a
#define NJB_CMD_GET_NEXT_DATAFILE_HEADER	0x4b
#define NJB_CMD_GET_DATAFILE_TAG		0x4d
#define NJB_CMD_VERIFY_LAST_CMD			0xf0
#define NJB_CMD_CAPTURE_NJB3			0xfd
#define NJB_CMD_RELEASE_NJB3			0xfe
#define NJB_CMD_UNKNOWN_NJB3			0xff

#define NJB_VAL_GET_EAX				0x00
#define NJB_VAL_GET_EAX_UNKNOWN			0x01

#define NJB_SOUND_SET_VOLUME                    0x01
#define NJB_SOUND_SET_BASS                      0x02
#define NJB_SOUND_SET_TREBLE                    0x03
#define NJB_SOUND_SET_MUTING	                0x04
#define NJB_SOUND_SET_MIDRANGE	                0x05
#define NJB_SOUND_SET_MIDFREQ	                0x06
#define NJB_SOUND_SET_EAX	                0x07
#define NJB_SOUND_SET_EAXAMT	                0x08
#define NJB_SOUND_SET_HEADPHONE	                0x09
#define NJB_SOUND_SET_REAR	                0x0A
#define NJB_SOUND_SET_EQSTATUS	                0x0D

#define NJB_MSG_OKAY				0x00
#define NJB_ERR_FAILED				0x01
#define NJB_ERR_DEVICE_BUSY			0x02
#define NJB_ERR_STORAGE_FULL			0x03
#define NJB_ERR_HD_GENERAL_ERROR		0x04
#define NJB_ERR_SETTIME_REJECTED		0x05

#define NJB_ERR_TRACK_NOT_FOUND			0x10
#define NJB_ERR_TRACK_EXISTS			0x11
#define NJB_ERR_TITLE_MISSING			0x12
#define NJB_ERR_CODEC_MISSING			0x13
#define NJB_ERR_SIZE_MISSING			0x14
#define NJB_ERR_IO_OPERATION_ABORTED		0x15
#define NJB_ERR_READ_WRITE_ERROR		0x16
#define NJB_ERR_NOT_OPENED			0x17
#define NJB_ERR_UPLOAD_DENIED			0x18

#define NJB_ERR_PLAYLIST_NOT_FOUND		0x20
#define NJB_ERR_PLAYLIST_EXISTS			0x21
#define NJB_ERR_PLAYLIST_ITEM_NOT_FOUND		0x22
#define NJB_ERR_PLAYLIST_ITEM_EXISTS		0x23

#define NJB_MSG_QUEUED_AUDIO_STARTED		0x30
#define NJB_MSG_PLAYER_FINISHED			0x31

#define NJB_ERR_QUEUE_ALREADY_EMPTY		0x40
#define NJB_ERR_VOLUME_CONTROL_UNAVAILABLE	0x42

#define NJB_ERR_DATA_NOT_FOUND			0x60
#define NJB_ERR_DATA_NOT_OPENED			0x67

#define NJB_ERR_PLAYER_NOT_CONNECTED		0x80
#define NJB_ERR_CANCELLED			0x81
#define NJB_ERR_PORT_NOT_AVAILABLE		0x82
#define NJB_ERR_OUT_OF_MEMORY			0x83
#define NJB_ERR_FILEOPEN_ERR			0x84
#define NJB_ERR_ITEM_NOT_FOUND			0x85
#define NJB_ERR_LOAD_COMPONENTS_FAILED		0x86
#define NJB_ERR_ID_INVALID			0x87
#define NJB_ERR_FILETYPE_ILLEGAL		0x88
#define NJB_ERR_LOADRES_FAIL			0x89
#define NJB_ERR_FORMAT_NOT_FOUND		0x8a
#define NJB_ERR_FILE_ALREADY_EXISTS		0x8b
#define NJB_ERR_LIB_CORRUPTED			0x8c
#define NJB_ERR_LIB_BUSY			0x8d
#define NJB_ERR_FILE_READ_WRITE_FAILED		0x8e
#define NJB_ERR_INVALID_FILEPATH		0x8f
#define NJB_ERR_DISKFULL_FOR_DOWNLOAD		0x90
#define NJB_ERR_FILE_PLAYONLY			0x91

#define NJB_ERR_UNDEFINED_ERR			0xff
#define NJB_ERR_NOT_IMPLEMENTED			0x100

/* NJB1 functions */

int njb_init_state (njb_t *njb);

int njb_get_library_counter (njb_t *njb, njblibctr_t *njbctr);
int njb_set_library_counter (njb_t *njb, u_int64_t count);
int njb_ping (njb_t *njb);
int njb_verify_last_command (njb_t *njb);
int njb_capture (njb_t *njb, int which);
int njb_reset_get_songlist (njb_t *njb);
int njb_get_disk_usage (njb_t *njb, u_int64_t *total, u_int64_t *free);
int njb_get_owner_string (njb_t *njb, owner_string name);
int njb_set_owner_string (njb_t *njb, owner_string name);
int njb_request_file (njb_t *njb, u_int32_t fileid);
int njb_transfer_complete (njb_t *njb);
int njb_send_track_tag (njb_t *njb, njbttaghdr_t *tagh, void *tag);
int njb_send_datafile_tag (njb_t *njb, njbdfhdr_t *dfh, void *tag);
int njb_replace_track_tag (njb_t *njb, njbttaghdr_t *tagh, void *tag);
int njb_add_track_to_playlist (njb_t *njb, u_int32_t plid, u_int32_t trid);
int njb_add_multiple_tracks_to_playlist (njb_t *njb, u_int32_t plid,
	u_int32_t *trids, u_int16_t ntracks);
int njb_delete_track (njb_t *njb, u_int32_t trackid);
int njb_delete_datafile (njb_t *njb, u_int32_t fileid);

int njb_create_playlist (njb_t *njb, char *name, u_int32_t *plid);
int njb_delete_playlist (njb_t *njb, u_int32_t plid);
int njb_rename_playlist (njb_t *njb, u_int32_t plid, char *name);

int njb_get_eax_size (njb_t *njb, u_int32_t *size);
void njb_read_eaxtypes (njb_t *njb, u_int32_t size);
njb_eax_t *njb_get_nexteax(njb_t *njb);

njb_time_t *njb_get_time(njb_t *njb);
int njb_set_time(njb_t *njb, njb_time_t *time);

u_int32_t njb_send_file_block (njb_t *njb, void *data, u_int32_t blocksize);
u_int32_t njb_receive_file_block (njb_t *njb, u_int32_t offset, u_int32_t blocksize, 
				  void *block);
int njb_get_track_tag_header (njb_t *njb, njbttaghdr_t *tagh, int cmd);
#define njb_get_first_track_tag_header(a,b) njb_get_track_tag_header(a,b,NJB_CMD_GET_FIRST_TRACK_TAG_HEADER)
#define njb_get_next_track_tag_header(a,b) njb_get_track_tag_header(a,b,NJB_CMD_GET_NEXT_TRACK_TAG_HEADER)
njb_songid_t *njb_get_track_tag (njb_t *njb, njbttaghdr_t *tagh);

int njb_get_playlist_header(njb_t *njb, njbplhdr_t *plh, int cmd);
#define njb_get_first_playlist_header(a,b) njb_get_playlist_header(a,b,NJB_CMD_GET_FIRST_PLAYLIST_HEADER)
#define njb_get_next_playlist_header(a,b) njb_get_playlist_header(a,b,NJB_CMD_GET_NEXT_PLAYLIST_HEADER)
njb_playlist_t *njb_get_playlist (njb_t *njb, njbplhdr_t *plh);

int njb_get_datafile_header (njb_t *njb, njbdfhdr_t *dfh, int cmd);
#define njb_get_first_datafile_header(a,b) njb_get_datafile_header(a,b,NJB_CMD_GET_FIRST_DATAFILE_HEADER);
#define njb_get_next_datafile_header(a,b) njb_get_datafile_header(a,b,NJB_CMD_GET_NEXT_DATAFILE_HEADER);
njb_datafile_t *njb_get_datafile_tag (njb_t *njb, njbdfhdr_t *dfh);

#define njb_play_track(a,b) njb_play_or_queue(a,b,NJB_CMD_PLAY_TRACK)
#define njb_queue_track(a,b) njb_play_or_queue(a,b,NJB_CMD_QUEUE_TRACK)
int njb_play_or_queue (njb_t *njb, u_int32_t trackid, int cmd);
int njb_stop_play (njb_t *njb);
int njb_elapsed_time (njb_t *njb, u_int16_t *elapsed, int *change);
int njb_adjust_sound(njb_t *njb, u_int8_t effect, int16_t value);

#endif


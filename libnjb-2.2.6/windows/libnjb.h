/**
 * \file libnjb.h
 *
 * Interface to the Nomad Jukebox library libnjb, handles most models.
 * Also handles the Dell Digital Jukebox.
 * This file should be included by programs that want to use libnjb, e.g.:
 *
 * <code>
 * #include <libnjb.h>
 * </code>
 */

#ifndef __LIBNJB__H
#define __LIBNJB__H

/** The version of this installation of libnjb */
#define LIBNJB_VERSION 2.2.4
/** A legacy definition - nowadays we always compile for libusb */
#define LIBNJB_COMPILED_FOR_LIBUSB 1

/* This handles MSVC pecularities */
#ifdef _MSC_VER
#include <windows.h>
#define __WIN32__
#define snprintf _snprintf
#define ssize_t SSIZE_T
#endif

#include <sys/types.h>
#ifdef __WIN32__
/* Windows specific code, types that do not exist in Windows
 * sys/types.h
 */
typedef char int8_t;
typedef unsigned char u_int8_t;
typedef __int16 int16_t;
typedef unsigned __int16 u_int16_t;
typedef __int32 int32_t;
typedef unsigned __int32 u_int32_t;
typedef unsigned __int64 u_int64_t;
#endif
#ifdef __sun

/*
 * Solaris specific code, types that do not exist in Solaris'
 * sys/types.h. u_intN_t is the ISO C way, whereas Solaris'
 * way is POSIXly correct.
 */
#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define u_int32_t uint32_t
#define u_int64_t uint64_t
#endif


#include <stdio.h>
#include <usb.h>

/** The maximum number of devices that can be found by libnjb */
#define NJB_MAX_DEVICES 0xFF

/** 
 * @defgroup njbboxes Enumerators to identify different jukeboxes
 *
 * These are found in the <code>device_type</code> field of the
 * <code>njb_t</code> struct. It is used a lot inside libnjb for
 * deciding how to handle a certain device.
 *
 * @see njb_t
 * @{
 */
#define NJB_DEVICE_NJB1		0x00 /**< Nomad Jukebox 1 */
#define NJB_DEVICE_NJB2		0x01 /**< Nomad Jukebox 2 */
#define NJB_DEVICE_NJB3		0x02 /**< Nomad Jukebox 3 */
#define NJB_DEVICE_NJBZEN	0x03 /**< Nomad Jukebox Zen (with FireWire) */
#define NJB_DEVICE_NJBZEN2	0x04 /**< Nomad Jukebox Zen USB 2.0 */
#define NJB_DEVICE_NJBZENNX	0x05 /**< Nomad Jukebox Zen NX */
#define NJB_DEVICE_NJBZENXTRA	0x06 /**< Nomad Jukebox Zen Xtra */
#define NJB_DEVICE_DELLDJ	0x07 /**< Dell Digital DJ "Dell DJ" */
#define NJB_DEVICE_NJBZENTOUCH	0x08 /**< Nomad Jukebox Zen Touch */
#define NJB_DEVICE_NJBZENMICRO	0x09 /**< Nomad Jukebox Zen Micro */
#define NJB_DEVICE_DELLDJ2	0x0a /**< Second Generation Dell DJ */
#define NJB_DEVICE_POCKETDJ     0x0b /**< Dell Pocket DJ */
#define NJB_DEVICE_ZENSLEEK	0x0c /**< Zen Sleek */
#define NJB_DEVICE_CREATIVEZEN  0x0d /**< Creative Zen (Micro variant) */
/** @} */

/**
 * @defgroup frametypes Frame types for different song ID frames
 * @{
 */
#define NJB_TYPE_STRING		0x00 /**< A string type song ID frame */
#define NJB_TYPE_UINT16		0x02 /**< An unsigned 16 bit integer type song ID frame */
#define NJB_TYPE_UINT32		0x03 /**< An unsigned 32 bit integer type song ID frame */
/** @} */

/**
 * @defgroup codecs Codec types
 * @{
 */
#define NJB_CODEC_MP3	"MP3" /**< The MPEG audio level 3 codec */
#define NJB_CODEC_WMA	"WMA" /**< The Windows Media Audio codec */
#define NJB_CODEC_WAV	"WAV" /**< The RIFF waveform codec */
#define NJB_CODEC_AA    "AA"  /**< The Audible.com codec */
/** @} */

/**
 * @defgroup frames Song ID frame types
 * @{
 */
#define FR_SIZE		"FILE SIZE" /**< Filesize metadata frame */
#define FR_LENGTH	"LENGTH"    /**< Length metadata frame (in seconds) */
#define FR_CODEC	"CODEC"     /**< Codec metadata frame */
#define FR_TITLE	"TITLE"     /**< Title metadata frame */
#define FR_ALBUM	"ALBUM"     /**< Album metadata frame */
#define FR_GENRE	"GENRE"     /**< Genre metadata frame */
#define FR_ARTIST	"ARTIST"    /**< Artist metadata frame */
#define FR_TRACK	"TRACK NUM" /**< Track number metadata frame */
#define FR_FNAME	"FNAME"     /**< File name metadata frame */
#define FR_YEAR		"YEAR"      /**< Year metadata frame */
#define FR_PROTECTED	"PlayOnly"  /**< Copy protected track metadata frame */
/* These two are used by Notmad on NJB1, not Creative */
#define FR_BITRATE	"BITRATE"   /**< Bitrate metadata frame */
#define FR_COMMENT	"COMMENT"   /**< Comment metadata frame */
/* This one is used on series 3 devices only */
#define FR_FOLDER	"FOLDER"    /**< Folder name metadata frame */
/** @} */


/**
 * @defgroup debugflags Debug flags
 * @{
 */
#define DD_USBCTL	1 /**< Print USB control transfer debug messages */
#define DD_USBBLKLIM	2 /**< Print USB bulk transfer debug messages (limited) */
#define DD_USBBLK	4 /**< Print USB bulk transfer debug messages (complete) */
#define DD_SUBTRACE	8 /**< Print subroutine call tree names debug messages */
/** @} */

/**
 * @defgroup unicodeflags Unicode flags
 * @see NJB_Set_Unicode()
 * @{
 */
#define NJB_UC_8859     0 /**< libnjb to use ISO 8859-1 / codepage 1252 */
#define NJB_UC_UTF8     1 /**< libnjb to use Unicode UTF-8 */
/** @} */

/**
 * @defgroup turboflags Turbo flags
 * @see NJB_Set_Turbo_Mode()
 * @{
 */
#define NJB_TURBO_OFF   0 /**< turbo mode is off for series 3 devices */
#define NJB_TURBO_ON    1 /**< turbo mode is on for series 3 devices */

/** The fixed length of the owner string */
#define OWNER_STRING_LENGTH	128
/** A type defined for owner strings */
typedef unsigned char owner_string[OWNER_STRING_LENGTH + 1];

/** 
 * @defgroup types libnjb global type definitions 
 * @{
 */
typedef struct njb_struct njb_t; /**< @see njb_struct */
typedef struct njb_songid_frame_struct njb_songid_frame_t; /**< See struct definition */
typedef struct njb_songid_struct njb_songid_t; /**< See struct definition */
typedef struct njb_playlist_track_struct njb_playlist_track_t; /**< See struct definition */
typedef struct njb_playlist_struct njb_playlist_t; /**< See struct definition */
typedef struct njb_datafile_struct njb_datafile_t; /**< See struct definition */
typedef struct njb_eax_struct njb_eax_t; /**< See struct definition */
typedef struct njb_time_struct njb_time_t; /**< See struct definition */
typedef struct njb_keyval_struct njb_keyval_t; /**< See struct definition */
/** @} */

/**
 * Main NJB object struct
 */
struct njb_struct {
	struct usb_device *device; /**< The libusb device for this jukebox */
	usb_dev_handle *dev; /**< The libusb device handle for this jukebox */
	u_int8_t usb_config; /**< The libusb config for this jukebox */
	u_int8_t usb_interface; /**< The libusb interface for this jukebox */
	u_int8_t usb_bulk_in_ep; /**< The BULK IN endpoint for this jukebox */
	u_int8_t usb_bulk_out_ep; /**< The BULK OUT endpoint for this jukebox */
	int device_type; /**< what kind of jukebox this is */
	int updated; /**< If the device has been updated with some metadata */
	u_int32_t xfersize; /**< The transfer size for endpoints */
	void *protocol_state; /**< dereferenced and maintained individually by protocol implementations */
	void *error_stack; /**< Error stack, used inside libnjb */
};

/* Song/track tag definitions */

/**
 * The song ID frame struct, one song ID has many
 * such frames, stored as a linked list.
 */
struct njb_songid_frame_struct {
	char *label; /**< terminated string with frame type */
	u_int8_t type; /**< this tells us which subtype it is */
	/**
         * This is a union of possible frame types
         */
	union {
		char *strval; /**< A string value */
		u_int8_t u_int8_val; /**< An 8 bit unsigned integer value */
		u_int16_t u_int16_val; /**< A 16 bit unsigned integer value */
		u_int32_t u_int32_val; /**< A 32 bit unsigned integer value */
		u_int64_t u_int64_val; /**< A 64 bit unsigned integer value */
	} data;
	njb_songid_frame_t *next; /**< A pointer to the next frame following this one in the song ID */
};

/**
 * The song ID struct is used for holding metadata about a particular
 * track.
 */
struct njb_songid_struct {
	u_int32_t trid; /**< The track ID as used on the device */
	u_int16_t nframes; /**< The number of frames in this song ID */
	njb_songid_frame_t *first; /**< A pointer to the first frame */
	njb_songid_frame_t *last; /**< A pointer to the last frame */
	njb_songid_frame_t *cur; /**< A pointer to the current frame */
	njb_songid_t *next; /**< Used internally on series 3 devices for spanning lists of song IDs only */
};

/* Playlist definitions */

/**
 * This struct is used for representing a track that
 * is part of a <code>njb_playlist_t</code> playlist
 */
struct njb_playlist_track_struct {
	u_int32_t trackid; /**< The track ID as used on the device */
	struct njb_playlist_track_struct *prev; /**< The previous track in the playlist */
	struct njb_playlist_track_struct *next; /**< The next track in the playlist */
};

/**
 * This struct holds the actual playlist. A playlist usually
 * contain a number of <code>njb_playlist_track_t</code> tracks.
 */
struct njb_playlist_struct {
	char *name; /**< The name of this playlist */
	int _state; /**< The state of this playlist */
#define NJB_PL_NEW		0 /**< This playlist is new */
#define NJB_PL_UNCHANGED	1 /**< This playlist has not changed */
#define NJB_PL_CHNAME		2 /**< This playlist has changed name */
#define NJB_PL_CHTRACKS		3 /**< This playlist has a new track listing */
	u_int32_t ntracks; /**< The number of tracks in this playlist */
	u_int32_t plid; /**< The playlist ID for this playlist, as used on the device */
	njb_playlist_track_t *first; /**< A pointer to the first track in this playlist */
	njb_playlist_track_t *last; /**< A pointer to the last track in this playlist */
	njb_playlist_track_t *cur; /**< A pointer to the current track in this playlist */
	njb_playlist_t *nextpl; /**< Used internally for spanning lists of 
				     playlists on series 3 devices only */
};

/**
 * This definition corresponds to the standard file permissions
 * set for most files transferred from Windows machines to the
 * device "file system" (file database). Can be used as a template
 * to simplify things.
 */
#define NJB_FILEFLAGS_REGULAR_FILE   0x80000000U

/**
 * This is the struct storing the metadata of a regular file
 * or folder.
 */
struct njb_datafile_struct {
	char *filename;
	/**<
	* The name of this file. The name "." means that this is
	* an empty folder name marker.
	*/
	char *folder;
	/**< 
	* This is the name of the folder the file belongs in.
	* All folder names are given with full hierarchy and leading 
	* and trailing backslash as in: "\foo\bar\fnord\".
	* If filename is "." this is the name of the empty folder.
	*/
	u_int32_t timestamp;
	/**< This is an ordinary UNIX styled timestamp for the file. */
	u_int32_t flags;
	/**< 
	 * These are ordinary windows file flags: 
	 * <pre>
	 * bit (from MSB)   meaning
	 * -----------------------------
	 * 31               Normal file (0x80000000U)
	 * 29               This file should be archived (0x20000000U) 
	 * 28               Directory (0x10000000U)
	 * 26               System file (0x04000000U)
	 * 25               Hidden file (0x02000000U)
	 * 24               Read only file (0x01000000U)
	 * 22               Encrypted file (0x00400000U)
	 * 21               Normal file? (0x00200000U)
	 * 19               Compressed file (0x00080000U)
	 * 17               Sparse file (0x00020000U)
	 * 16               Temporary file (0x00010000U)
	 * </pre>
	 *
	 * SAMBA maps the bits to Unix permissions thus:
	 *
	 * <pre>
	 *  owner        group        world
	 *  r  w  x      r  w  x      r  w  x
	 *  ^  ^  ^            ^            ^
	 *  |  |  |            |            |
	 *  |  |  Archive      System       Hidden
	 *  |  |
	 *  Read only
	 * </pre>
	 *
	 * The meaning of bits 30, 27, 23, 20 and 15-0 is unknown.
	 */
	u_int32_t dfid;
	/**< The 32-bit unsigned file ID. */
	u_int64_t filesize;
	/**<
	 * The file size as a 64-bit unsigned integer. The files on
	 * series 3 devices only have 32-bit signed length (and can only be
	 * 2GB in size) but the NJB1 supports 64-bit length.
	 */
	njb_datafile_t *nextdf;
	/**< This is only to be used internally by libnjb. */
};


/**
 * This is the EAX Control Type
 */
typedef enum {
       NJB_EAX_NO_CONTROL,
       NJB_EAX_SLIDER_CONTROL,
       NJB_EAX_FIXED_OPTION_CONTROL
} njb_eax_control_t;


/**
 * This is the EAX API type.
 */
struct njb_eax_struct {
	u_int16_t number; /**< The number of this effect */
	char *name; /**< The name of this effect */
	u_int8_t exclusive;
	/**< 
	 * 0x00 = not exclusive, 0x01 = exclusive
         * The "exclusive" attribute signifies if this effect can be used 
         * in parallell with other EAX effects,
	 * GUI:s shall make sure all other effects are reset to default
         * values when one effect is chosen.
         */
	u_int8_t group;
	/**<
         * The visual group for this effect - 
	 * effects appearing after each other are grouped if their 
	 * group numbers are identical
	 */
        njb_eax_control_t type;
        /**<
         * NJB_EAX_NO_CONTROL,
         * NJB_EAX_SLIDER_CONTROL,
         * NJB_EAX_FIXED_OPTION_CONTROL
         *
         * this replaces selectable, scalable with a single variable
         */
        int16_t   current_value;
	/**<
	 * The current value of this effect.
	 * Notice that this value may be negative!
	 */
        int16_t   min_value;
	/**<
	 * The minumum value for this effect.
	 * Notice that this value may be negative!
	 */
        int16_t   max_value;
	/**<
	 * The maximum value for this effect.
	 */
	char **option_names;
	/**< 
	 * Array with names for the selections, if this is a fixed
	 * option control. It may not be dereferenced for slider
         * controls.
	 */
	njb_eax_t *next;
	/**< 
	 * Only to be used inside of libnjb
	 */
};

/**
 * Time set/read struct
 */
struct njb_time_struct {
	int16_t year; /**< Year (4 digits) */
	int16_t month; /**< Month (0-11) */
	int16_t day; /**< Day (0-31) */
	int16_t weekday; /**< Day of the week 0=sunday, 6=saturday */
	int16_t hours; /**< Hours (24-hour notation) */
	int16_t minutes; /**< Minutes */
	int16_t seconds; /**< Seconds */
};

/** 
 * Struct to hold key/value pairs which are used
 * for some kind of DRM encryption scheme. Usage unknown.
 */
struct njb_keyval_struct {
	char key[5]; /**< The name of this key (AR00, PL00 etc.) */
	u_int32_t value1; /**< The first 32 bit unsigned integer of this key */
	u_int32_t value2; /**< The second 32 bit unsigned integer of this key */
	unsigned char deviceid[16]; /**< The SDMI compliant device ID associated with this key */
	njb_keyval_t *next; /**< A pointer to the next key/value pair */
};

#ifdef __cplusplus
extern "C" {
#endif

/* NJB commands */

/** The callback type */
typedef int NJB_Xfer_Callback(u_int64_t sent, u_int64_t total,
		const char* buf, unsigned len, void *data);

/**
 * @defgroup internals The libnjb configuration API
 * @{
 */
void NJB_Set_Debug (int debug_flags);
void NJB_Set_Unicode (int unicode_flag);
int NJB_Error_Pending(njb_t *njb);
void NJB_Error_Reset_Geterror(njb_t *njb);
const char *NJB_Error_Geterror(njb_t *njb);
void NJB_Error_Dump(njb_t *njb, FILE *fp);
/**
 * @}
 * @defgroup basic The basic device management and information API
 * @{
 */
int NJB_Discover(njb_t *njbs, int limit, int *n);
int NJB_Open(njb_t *njb);
void NJB_Close(njb_t *njb);
int NJB_Capture (njb_t *njb);
int NJB_Release (njb_t *njb);
void NJB_Ping(njb_t *njb);
int NJB_Get_Disk_Usage (njb_t *njb, u_int64_t *btotal, u_int64_t *bfree);
char *NJB_Get_Owner_String (njb_t *njb);
int NJB_Set_Owner_String (njb_t *njb, const char *name);
int NJB_Get_Bitmap_Dimensions(njb_t *njb, int *x, int *y, int *bytes);
int NJB_Set_Bitmap(njb_t *njb, const unsigned char *bitmap);
njb_keyval_t *NJB_Get_Keys(njb_t *njb);
u_int64_t NJB_Get_NJB1_Libcounter(njb_t *njb);
int NJB_Send_Firmware (njb_t *njb, const char *path, NJB_Xfer_Callback *callback, void *data);
int NJB_Get_Battery_Level (njb_t *njb);
int NJB_Get_Battery_Charging (njb_t *njb);
int NJB_Get_Auxpower (njb_t *njb);
int NJB_Get_SDMI_ID(njb_t *njb, u_int8_t *sdmiid);
const char *NJB_Get_Device_Name(njb_t *njb, int type);
int NJB_Get_Firmware_Revision(njb_t *njb, u_int8_t *major, u_int8_t *minor, u_int8_t *release);
int NJB_Get_Hardware_Revision(njb_t *njb, u_int8_t *major, u_int8_t *minor, u_int8_t *release);
int NJB_Set_Turbo_Mode(njb_t *njb, u_int8_t mode);
/**
 * @}
 * @defgroup tagapi The track and tag (song ID metadata) manipulation API
 * @{
 */
njb_songid_t *NJB_Songid_New(void);
void NJB_Songid_Destroy(njb_songid_t *song);
void NJB_Songid_Addframe(njb_songid_t *song, njb_songid_frame_t *frame);
void NJB_Songid_Reset_Getframe(njb_songid_t *song);
njb_songid_frame_t *NJB_Songid_Getframe(njb_songid_t *song);
njb_songid_frame_t *NJB_Songid_Findframe(njb_songid_t *song, const char *label);
njb_songid_frame_t *NJB_Songid_Frame_New_String(const char *label, const char *value);
njb_songid_frame_t *NJB_Songid_Frame_New_Uint16(const char *label, u_int16_t value);
njb_songid_frame_t *NJB_Songid_Frame_New_Uint32(const char *label, u_int32_t value);
/* Good helper functions for creating frames - USE THESE IF YOU CAN */
njb_songid_frame_t *NJB_Songid_Frame_New_Codec(const char *value);
/* #define NJB_Songid_Frame_New_Codec(a) NJB_Songid_Frame_New_String(FR_CODEC, a) */
#define NJB_Songid_Frame_New_Title(a) NJB_Songid_Frame_New_String(FR_TITLE, a)
#define NJB_Songid_Frame_New_Album(a) NJB_Songid_Frame_New_String(FR_ALBUM, a)
#define NJB_Songid_Frame_New_Genre(a) NJB_Songid_Frame_New_String(FR_GENRE, a)
#define NJB_Songid_Frame_New_Artist(a) NJB_Songid_Frame_New_String(FR_ARTIST, a)
#define NJB_Songid_Frame_New_Length(a) NJB_Songid_Frame_New_Uint16(FR_LENGTH, a)
#define NJB_Songid_Frame_New_Filesize(a) NJB_Songid_Frame_New_Uint32(FR_SIZE, a)
#define NJB_Songid_Frame_New_Tracknum(a) NJB_Songid_Frame_New_Uint16(FR_TRACK, a)
#define NJB_Songid_Frame_New_Year(a) NJB_Songid_Frame_New_Uint16(FR_YEAR, a)
#define NJB_Songid_Frame_New_Filename(a) NJB_Songid_Frame_New_String(FR_FNAME, a)
#define NJB_Songid_Frame_New_Protected(a) NJB_Songid_Frame_New_Uint16(FR_PROTECTED, a)
/* These two only apply to NJB1 */
#define NJB_Songid_Frame_New_Bitrate(a) NJB_Songid_Frame_New_Uint32(FR_BITRATE, a)
#define NJB_Songid_Frame_New_Comment(a) NJB_Songid_Frame_New_String(FR_COMMENT, a)
/* This one only apply to series 3 devices */
#define NJB_Songid_Frame_New_Folder(a) NJB_Songid_Frame_New_String(FR_FOLDER, a)
void NJB_Songid_Frame_Destroy (njb_songid_frame_t *frame);
void NJB_Get_Extended_Tags (njb_t *njb, int extended);
void NJB_Reset_Get_Track_Tag (njb_t *njb);
njb_songid_t *NJB_Get_Track_Tag (njb_t *njb);
int NJB_Replace_Track_Tag(njb_t *njb, u_int32_t trackid, njb_songid_t *songid);
int NJB_Get_Track (njb_t *njb, u_int32_t trackid, u_int32_t size,
	const char *path, NJB_Xfer_Callback *callback, void *data);
int NJB_Get_Track_fd (njb_t *njb, u_int32_t trackid, u_int32_t size,
	int fd, NJB_Xfer_Callback *callback, void *data);
int NJB_Send_Track (njb_t *njb, const char *path, njb_songid_t *songid,
	NJB_Xfer_Callback *callback, void *data, u_int32_t *trackid);
int NJB_Delete_Track (njb_t *njb, u_int32_t trackid);
/**
 * @}
 * @defgroup playlistapi The playlist manipulation API
 * @{
 */
void NJB_Reset_Get_Playlist (njb_t *njb);
njb_playlist_t *NJB_Get_Playlist (njb_t *njb);
int NJB_Delete_Playlist (njb_t *njb, u_int32_t plid);
int NJB_Update_Playlist (njb_t *njb, njb_playlist_t *pl);
njb_playlist_t *NJB_Playlist_New(void);
void NJB_Playlist_Destroy(njb_playlist_t *pl);
void NJB_Playlist_Addtrack(njb_playlist_t *pl, njb_playlist_track_t *track, 
	unsigned int pos);
#define NJB_PL_END	0
#define NJB_PL_START	1
void NJB_Playlist_Reset_Gettrack(njb_playlist_t *pl);
njb_playlist_track_t *NJB_Playlist_Gettrack(njb_playlist_t *pl);
int NJB_Playlist_Set_Name(njb_playlist_t *pl, const char *name);
void NJB_Playlist_Deltrack(njb_playlist_t *pl, unsigned int pos);
void NJB_Playlist_Deltrack_TrackID(njb_playlist_t *pl, u_int32_t trackid);
njb_playlist_track_t *NJB_Playlist_Track_New(u_int32_t trackid);
void NJB_Playlist_Track_Destroy(njb_playlist_track_t *track);
/**
 * @}
 * @defgroup datatagapi The datafile tag (metadata) retrieveal API
 * @{
 */
void NJB_Reset_Get_Datafile_Tag (njb_t *njb);
njb_datafile_t *NJB_Get_Datafile_Tag (njb_t *njb);
void NJB_Datafile_Destroy(njb_datafile_t *df);
#define NJB_Get_File NJB_Get_Track
#define NJB_Get_File_fd NJB_Get_Track_fd
int NJB_Send_File (njb_t *njb, const char *path, const char *name, const char *folder,
	NJB_Xfer_Callback *callback, void *data, u_int32_t *fileid);
int NJB_Delete_Datafile (njb_t *njb, u_int32_t fileid);
int NJB_Create_Folder (njb_t *njb, const char *name, u_int32_t *folderid);
/**
 * @}
 * @defgroup eaxapi The EAX (and volume) manipulation API
 * @{
 */
void NJB_Reset_Get_EAX_Type (njb_t *njb);
njb_eax_t *NJB_Get_EAX_Type (njb_t *njb);
void NJB_Destroy_EAX_Type (njb_eax_t *eax);
void NJB_Adjust_EAX (njb_t *njb,
	u_int16_t eaxid,
	u_int16_t patchindex,
	int16_t scalevalue);
/**
 * @}
 * @defgroup timeapi The time manipulation API
 * @{
 */
njb_time_t *NJB_Get_Time(njb_t *njb);
int NJB_Set_Time(njb_t *njb, njb_time_t *time);
void NJB_Destroy_Time(njb_time_t *time);
/**
 * @}
 * @defgroup playapi The track playback (listening) API
 * @{
 */
int NJB_Play_Track (njb_t *njb, u_int32_t trackid);
int NJB_Queue_Track (njb_t *njb, u_int32_t trackid);
int NJB_Stop_Play (njb_t *njb);
int NJB_Pause_Play (njb_t *njb);
int NJB_Resume_Play (njb_t *njb);
int NJB_Seek_Track (njb_t *njb, u_int32_t position);
int NJB_Elapsed_Time (njb_t *njb, u_int16_t *elapsed, int *change);
/** @} */

#ifdef __cplusplus
}
#endif

#endif

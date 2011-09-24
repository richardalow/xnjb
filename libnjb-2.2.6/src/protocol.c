/**
 * \file protocol.c
 *
 * This file contains the major parts of the NJB1 (OaSIS) protocol
 * for all jukebox functionality.
 */

/* MSVC does not have these */
#ifndef _MSC_VER
#include "config.h"
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "libnjb.h"
#include "protocol.h"
#include "byteorder.h"
#include "njb_error.h"
#include "usb_io.h"
#include "ioutil.h"
#include "defs.h"
#include "base.h"
#include "eax.h"
#include "songid.h"
#include "datafile.h"
#include "njbtime.h"
#include "playlist.h"

extern int __sub_depth;


/**
 * Initializes the basic state of the njb->protocol_state for the
 * NJB1-device.
 *
 * @param njb is a pointer to the jukebox object to use.
 * @return 0 on success, -1 on failure.
 */
int njb_init_state (njb_t *njb) {
  __dsub= "njb_init_state";

  njb_state_t *state;

  __enter;

  state = malloc(sizeof(njb_state_t));
  if (state == NULL) {
    __leave;
    return -1;
  }

  state->session_updated = 0;
  state->libcount = 0;
  state->first_eax = NULL;
  state->next_eax = NULL;
  state->reset_get_track_tag = 0;
  state->reset_get_playlist = 0;
  state->reset_get_datafile_tag = 0;
  state->power = NJB_POWER_BATTERY;
  njb->protocol_state = (unsigned char *) state;
  memset(&state->sdmiid, 0, 16);

  __leave;
  return 0;
}

/*
 * Beginning with firmware 2.93, very strange things started happening.
 * For certian functions, a non-zero status code does not automatically
 * imply an error.  Instead, the NJB seems to send a code with the 
 * four low bits cleared (meaning, a success can be 0x20, 0x40, 0x80,
 * etc.).  I have only seen this on a few functions, and I am not sure
 * what it means, but I have implemented it where I've run into it.
 */


/**
 * This returns the string representing an internal NJB1 error code.
 */
#define NJB_STATUS(a,b) { \
	char *s = njb_status_string(b);\
	njb_error_add_string(a,subroutinename,s);\
	free (s); \
}
static char *njb_status_string (unsigned char code)
{
  char buffer[100];
  const char *status = NULL;
  const char *fmt = "%s";
 
  switch (code)
    {
    case NJB_MSG_OKAY:
      status = "no error";
      break;
    case NJB_ERR_FAILED:
      status = "operation failed";
      break;
    case NJB_ERR_DEVICE_BUSY:
      status = "device busy";
      break;
    case NJB_ERR_STORAGE_FULL:
      status = "storage full";
      break;
    case NJB_ERR_HD_GENERAL_ERROR:
      status = "general hard drive failure";
      break;
    case NJB_ERR_SETTIME_REJECTED:
      status = "set time rejected";
      break;
      
    case NJB_ERR_TRACK_NOT_FOUND:
      status = "track not found";
      break;
    case NJB_ERR_TRACK_EXISTS:
      status = "track exists";
      break;
    case NJB_ERR_TITLE_MISSING:
      status = "title missing";
      break;
    case NJB_ERR_CODEC_MISSING:
      status = "CODEC missing";
      break;
    case NJB_ERR_SIZE_MISSING:
      status = "size missing";
      break;
    case NJB_ERR_IO_OPERATION_ABORTED:
      status = "I/O operation aborted";
      break;
    case NJB_ERR_READ_WRITE_ERROR:
      status = "read/write error";
      break;
    case NJB_ERR_NOT_OPENED:
      status = "not opened";
      break;
    case NJB_ERR_UPLOAD_DENIED:
      status = "upload denied";
      break;
      
    case NJB_ERR_PLAYLIST_NOT_FOUND:
      status = "playlist not found";
      break;
    case NJB_ERR_PLAYLIST_EXISTS:
      status = "playlist exists";
      break;
    case NJB_ERR_PLAYLIST_ITEM_NOT_FOUND:
      status = "playlist item not found";
      break;
    case NJB_ERR_PLAYLIST_ITEM_EXISTS:
      status = "playlist item exists";
      break;
      
    case NJB_MSG_QUEUED_AUDIO_STARTED:
      status = "queued audio started";
      break;
    case NJB_MSG_PLAYER_FINISHED:
      status = "player finished";
      break;

    case NJB_ERR_QUEUE_ALREADY_EMPTY:
      status = "queue already empty";
      break;
    case NJB_ERR_VOLUME_CONTROL_UNAVAILABLE:
      status = "volume control unavailable";
      break;
      
    case NJB_ERR_DATA_NOT_FOUND:
      status = "data not found";
      break;
    case NJB_ERR_DATA_NOT_OPENED:
      status = "data not opened";
      break;
      
    case NJB_ERR_UNDEFINED_ERR:
      status = "undefined error";
      break;
      /*
	This is currently not used and does not have a meaningful value
	case NJB_ERR_NOT_IMPLEMENTED:
	status = "function not implemented";
	break;
      */
    default:
      status = "unknown error";
      fmt = "%s %02x";
      break;
    }
  
  sprintf (buffer, fmt, status, code);
  
  return strdup (buffer);
}

/**
 * This function sets the library counter on the device.
 * The library counter is sent back and forth to the device to indicate if
 * something in the library has changed.
 *
 * @param njb is a pointer to the jukebox object to use.
 * @param count is the current library counter to set on the device.
 * @return 0 on success, -1 on failure.
 */
int njb_set_library_counter (njb_t *njb, u_int64_t count)
{
	__dsub= "njb_set_library_counter";
	unsigned char data[8];

	__enter;

	memset(data, 0, 8);
	from_64bit_to_njb1_bytes(count, &data[0]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_SET_LIBRARY_COUNTER, 0, 0, 8, data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

#if (DEBUG>=2)
	fprintf(stderr, "Count: 0x%08qx\n", count);
#endif
	__leave;
	return 0;
}

/**
 * This function gets the library counter from the device.
 * The library counter is sent back and forth to the device to indicate if
 * something in the library as changed.
 *
 * @param njb is a pointer to the jukebox object to use.
 * @lcount is a pointer to the current library counter to get from the device.
 * @return 0 on success, -1 on failure.
 */
int njb_get_library_counter (njb_t *njb, njblibctr_t *lcount)
{
	__dsub= "njb_get_library_counter";
	unsigned char data[25];

	__enter;

	memset(lcount, 0, sizeof(njblibctr_t));
	memset(data, 0, 25);
	
	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_GET_LIBRARY_COUNTER, 0, 0, 25, data) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data[0] & 0xf ) {
		NJB_STATUS(njb,data[0]);
		__leave;
		return -1;
	} else if ( data[0] ) {
		njb_get_library_counter(njb, lcount);
	} else {
		memcpy(&lcount->id, &data[1], 16);
		lcount->count = njb1_bytes_to_64bit(&data[17]);
#if (DEBUG>=2)
		fprintf(stderr, "NJB ID: ");
		data_dump(stderr, lcount->id, 16);
		fprintf(stderr, "Count: 0x%08qx\n", lcount->count);
#endif
	}

	__leave;
	return 0;
}

/**
 * This function pings the NJB1 device to see if it is up and running,
 * and ready for action. This is done every now and then.
 *
 * @param njb is a pointer to the jukebox object to use.
 * @return 0 on success, -1 on failure.
 */
int njb_ping (njb_t *njb)
{
  __dsub= "njb_ping";
  ssize_t bread;
  unsigned char data[58];
  njb_state_t *state = (njb_state_t *) njb->protocol_state;
  
  __enter;
  
  memset(data, 0, 58);
  
  if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		 NJB_CMD_PING, 0, 0, 0, NULL) == -1 ) {
    NJB_ERROR(njb, EO_USBCTL);
    __leave;
    return -1;
  }
  
  if ( (bread= usb_pipe_read(njb, data, 58)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 58 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }

  if ( data[0] ) {
    NJB_STATUS(njb,data[0]);
    __leave;
    return -1;
  }
  memcpy(&state->sdmiid, &data[1], 16);
  state->fwMinor = (u_int8_t) data[19];
  state->fwMajor = (u_int8_t) data[20];
  memcpy(&state->productName, &data[25], 32);
  state->power= (u_int8_t) data[57];
  
#if (DEBUG>=2)
  printf("NJB ID   : ");
  data_dump(stdout, &state->sdmiid, 16);
  printf("Firmware : %d.%d\n", state->fwMajor, state->fwMinor);
  printf("Product  : %s\n", state->productName);
  printf("Power    : %d\n", state->power);
#endif
  __leave;
  return 0;
}

/**
 * This function verifies if the last command was successful or not.
 *
 * @param njb is a pointer to the jukebox object to use.
 * @return 0 on success, -1 on failure.
 */
int njb_verify_last_command (njb_t *njb)
{
	__dsub= "njb_verify_last_command";
	unsigned char data = '\0';

	__enter;

	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_VERIFY_LAST_CMD, 0, 0, 1, &data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data ) {
		NJB_STATUS(njb,data);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

/**
 * This function captures the NJB1 device.
 *
 * @param njb is a pointer to the jukebox object to use.
 * @return 0 on success, -1 on failure.
 */
int njb_capture (njb_t *njb, int which)
{
	__dsub= "njb_capture";

       	unsigned char data = '\0';
       	int mode= (which == NJB_CAPTURE) ? NJB_CMD_CAPTURE_NJB :
       		NJB_CMD_RELEASE_NJB;

	__enter;

       	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
       		mode, 0, 0, 1, &data) == -1 ) {

       		NJB_ERROR(njb, EO_USBCTL);
       		__leave;
       		return -1;
       	}

       	if ( data && (data & 0xf) ) {
       		NJB_ERROR(njb, EO_BADSTATUS);
       		__leave;
       		return -1;
       	}

	__leave;
	return 0;
}

/**
 * Get a track header from the NJB1.
 *
 * @return 0 on OK, -1 on error, -2 if the last header has been
 *         retrieved.
 */
int njb_get_track_tag_header (njb_t *njb, njbttaghdr_t *tagh, int cmd)
{
	__dsub= "njb_get_track_tag_header";
	unsigned char data[9];

	__enter;

	memset(data, 0, 9);
	
	if ( usb_setup(njb, UT_READ_VENDOR_OTHER, cmd, 0, 0, 9, data)
		 == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data[0] == NJB_ERR_TRACK_NOT_FOUND ) {
	        /*
	         * Ignore this "error" as of now, it probably
		 * just mean "end of transmission" or "zero tags left". 
		 */
	        /* NJB_ERROR(njb, EO_EOM); return -1; */
		__leave;
		return -2;
	} else if ( data[0] ) {
		NJB_STATUS(njb,data[0]);
		__leave;
		return -1;
	}

	tagh->trackid = njb1_bytes_to_32bit(&data[1]);
	tagh->size = njb1_bytes_to_32bit(&data[5]);

	__leave;
	return 0;
}

njb_songid_t *njb_get_track_tag (njb_t *njb, njbttaghdr_t *tagh)
{
	__dsub= "njb_get_track_tag";
	u_int16_t msw, lsw;
	ssize_t bread;
	void *data;
	njb_songid_t *song;
	unsigned char *dp;

	__enter;

	data= malloc(tagh->size+5);
	if ( data == NULL ) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return NULL;
	}
	memset(data, 0, tagh->size+5);

	lsw= get_lsw(tagh->trackid);
	msw= get_msw(tagh->trackid);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_GET_TRACK_TAG, msw, lsw, 0, NULL) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		free(data);
		__leave;
		return NULL;
	}

	if ( (bread= usb_pipe_read(njb, data, tagh->size+5)) == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		free(data);
		__leave;
		return NULL;
	}

	dp= (unsigned char *) data;

	song= songid_unpack(&dp[5], tagh->size);
	if ( song != NULL ) song->trid= tagh->trackid;

	free(data);
	
	__leave;
	return song;
}

/**
 * Gets a playlist header.
 *
 * @return 0 on OK, -1 on error, -2 means "retry", -3 means
 *         "not found" or "last playlist already retrieved".
 */
int njb_get_playlist_header (njb_t *njb, njbplhdr_t *plh, int cmd)
{
	__dsub= "njb_get_playlist_header";
	unsigned char data[9];

	__enter;

	memset(data, 0, 9);
	
	if ( usb_setup(njb, UT_READ_VENDOR_OTHER, cmd, 0, 0, 9, data)
		 == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data[0] == NJB_ERR_PLAYLIST_NOT_FOUND ) {
	        /*
	         * Ignore this "error" as of now, it probably
		 * just mean "end of transmission" or "zero tags left". 
		 */
	        /* NJB_ERROR(njb, EO_EOM); return -1; */
		__leave;
		return -3;
	} else if ( (data[0]) & 0xf ) { 
		NJB_STATUS(njb,data[0]);
		__leave;
		return -1;
	} else if ( data[0] ) {
		__leave;
		return -2;
	}

	plh->plid = njb1_bytes_to_32bit(&data[1]);
	plh->size = njb1_bytes_to_32bit(&data[5]);

	__leave;
	return 0;
}

njb_playlist_t *njb_get_playlist (njb_t *njb, njbplhdr_t *plh)
{
	__dsub= "njb_get_playlist";
	u_int16_t msw, lsw;
	ssize_t bread;
	void *data;
	njb_playlist_t *pl;
	unsigned char *dp;

	__enter;

	data = malloc(plh->size+5);
	if ( data == NULL ) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return NULL;
	}
	memset(data, 0, plh->size+5);

	lsw= get_lsw(plh->plid);
	msw= get_msw(plh->plid);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
	     NJB_CMD_GET_PLAYLIST, msw, lsw, 0, NULL) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		free(data);
		__leave;
		return NULL;
	}

	if ( (bread= usb_pipe_read(njb, data, plh->size+5)) == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		free(data);
		__leave;
		return NULL;
	} else if ( bread < plh->size+5 ) {
		NJB_ERROR(njb, EO_RDSHORT);
		free(data);
		__leave;
		return NULL;
	}

	dp = (unsigned char *) data;

	pl = playlist_unpack(&dp[5], plh->size);

	free(data);
	
	__leave;
	return pl;
}

/**
 * Get disk usage for the NJB1
 *
 * @return 0 on OK, -1 on failure, -2 means "try again"
 */
int njb_get_disk_usage (njb_t *njb, u_int64_t *total, u_int64_t *free_bytes)
{
	__dsub= "njb_get_disk_usage";
	unsigned char data[17];

	__enter;

	memset(data, 0, 17);
	
	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_GET_DISK_USAGE, 0, 0, 17, data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data[0] & 0xf ) {
		NJB_STATUS(njb,data[0]);
		__leave;
		return -1;
	} else if ( data[0] ) {
		__leave;
		return -2;
	}

	*total = njb1_bytes_to_64bit(&data[1]);
	*free_bytes = njb1_bytes_to_64bit(&data[9]);

	__leave;
	return 0;
}

int njb_get_owner_string (njb_t *njb, owner_string name)
{
	__dsub= "njb_get_owner_string";
	ssize_t bread;
	char data[OWNER_STRING_LENGTH+1];

	__enter;

	memset(data,0,OWNER_STRING_LENGTH+1);
       	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
       		NJB_CMD_GET_OWNER_STRING, 0, 0, 0, NULL) == -1 ) {

       		NJB_ERROR(njb, EO_USBCTL);
       		__leave;
       		return -1;
       	}

       	if ( (bread= usb_pipe_read(njb, data, OWNER_STRING_LENGTH+1))
       		== -1 ) {

       		NJB_ERROR(njb, EO_USBBLK);
       		__leave;
       		return -1;
       	} else if ( bread < OWNER_STRING_LENGTH+1 ) {
       		NJB_ERROR(njb, EO_RDSHORT);
       		__leave;
       		return -1;
       	}
       	if ( data[0] ) {
       		NJB_STATUS(njb,(unsigned char) data[0]);
       		__leave;
       		return -1;
       	}

       	strncpy((char *) name, &data[1], OWNER_STRING_LENGTH);
	name[OWNER_STRING_LENGTH]= '\0'; /* Just in case */

	__leave;
	return 0;
}

int njb_set_owner_string (njb_t *njb, owner_string name)
{
	__dsub= "njb_set_owner_string";
	ssize_t bwritten;

	__enter;

       	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
       		NJB_CMD_SET_OWNER_STRING, 0, 0, 0, NULL) == -1 ) {

       		NJB_ERROR(njb, EO_USBCTL);
       		__leave;
       		return -1;
       	}

       	if ( (bwritten= usb_pipe_write(njb, name, OWNER_STRING_LENGTH))
       		== -1 ) {

       		NJB_ERROR(njb, EO_USBBLK);
       		__leave;
       		return -1;
       	} else if ( bwritten < OWNER_STRING_LENGTH ) {
       		NJB_ERROR(njb, EO_WRSHORT);
       		__leave;
       		return -1;
       	}

	__leave;

	return 0;
}

/**
 * Retrieves a datafile header from the NJB1
 *
 * @return 0 on OK, -1 on error, -2 means the last header
 *         has already been returned.
 */
int njb_get_datafile_header (njb_t *njb, njbdfhdr_t *dfh, int cmd)
{
	__dsub= "njb_get_datafile_header";
	unsigned char data[9];

	__enter;

	memset(data, 0, 9);
	
	if ( usb_setup(njb, UT_READ_VENDOR_OTHER, cmd, 0, 0, 9, data)
		 == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return 0;
	}

	if ( data[0] == NJB_ERR_DATA_NOT_FOUND ) {
	        /*
	         * Ignore this "error" as of now, it probably
		 * just mean "end of transmission" or "zero tags left". 
		 */
	        /* NJB_ERROR(njb, EO_EOM); return -1; */
		__leave;
		return -2;
	} else if ( data[0] ) {
		NJB_STATUS(njb,data[0]);
		__leave;
		return -1;
	}

	dfh->dfid = njb1_bytes_to_32bit(&data[1]);
	dfh->size = njb1_bytes_to_32bit(&data[5]);

	__leave;
	return 0;
}

njb_datafile_t *njb_get_datafile_tag (njb_t *njb, njbdfhdr_t *dfh)
{
	__dsub= "njb_get_datafile_tag";
	u_int16_t msw, lsw;
	ssize_t bread;
	void *data;
	njb_datafile_t *df;
	unsigned char *dp;

	__enter;

	data= malloc(dfh->size+5);
	if ( data == NULL ) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return NULL;
	}
	memset(data, 0, dfh->size+5);
	
	lsw= get_lsw(dfh->dfid);
	msw= get_msw(dfh->dfid);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_GET_DATAFILE_TAG, msw, lsw, 0, NULL) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		free(data);
		__leave;
		return NULL;
	}

	if ( (bread= usb_pipe_read(njb, data, dfh->size+5)) == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		free(data);
		__leave;
		return NULL;
	} else if ( bread < dfh->size+5 ) {
		NJB_ERROR(njb, EO_RDSHORT);
		free(data);
		__leave;
		return NULL;
	}

	dp= (unsigned char *) data;

	df= datafile_unpack(&dp[5], dfh->size);
	if ( df != NULL ) df->dfid= dfh->dfid;

	free(data);
	__leave;
	return df;
}

/**
 * offset    is the offset into the file, starting at 0.
 * bsize     indicates the recieved buffer max size.
 * bp        points to the recieve buffer
 *           (atleast NJB_XFER_BLOCK_SIZE + NJB_XFER_BLOCK_HEADER_SIZE)
 * lastsort  indicates if last transfer was short (ended before
 *           requested number of bytes were recieved).
 *
 * If lastshort == 1, the last call to this function returned a
 * a short read. In that case, a new setup command shall not be sent,
 * the bus shall just keep retrieveing buffer contents from the
 * bulk pipe.
 */
u_int32_t njb_receive_file_block (njb_t *njb, u_int32_t offset, u_int32_t bsize, 
				  void *bp)
{
	__dsub= "njb_receive_file_block";
	ssize_t bread;
	unsigned char filedata[8];
	unsigned char status = '\0';

	__enter;

	/* Too large transfer requested */
	if ( bsize > NJB_XFER_BLOCK_SIZE ) {
		NJB_ERROR(njb, EO_TOOBIG);
		__leave;
		return -1;
	}

	memset(filedata, 0, 8);
	
	/* Logs indicate that you should send a new retrieve 
	 * command only if last read was not a short read, but
	 * in practice you should, as far as we can see. */
	from_32bit_to_njb1_bytes(offset, &filedata[0]);
	from_32bit_to_njb1_bytes(bsize, &filedata[4]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		       NJB_CMD_RECEIVE_FILE_BLOCK, 0, 0, 8, filedata) == -1 ) {
	  
	  NJB_ERROR(njb, EO_USBCTL);
	  __leave;
	  return -1;
	}

	/* Read the status and block header first */
	if ( (bread = usb_pipe_read(njb, bp, bsize + NJB_XFER_BLOCK_HEADER_SIZE)) == -1 ) {
	  NJB_ERROR(njb, EO_USBBLK);
	  __leave;
	  return -1;
	} else if ( bread < bsize + NJB_XFER_BLOCK_HEADER_SIZE ) {
	  /* This was a short transfer, OK... */
	  /* printf("Short transfer on pipe.\n"); */
	}

	/* Check status in header, should it be if (!lastshort)??? */
	status = ((unsigned char *)bp)[0];
	if (status) {
	  NJB_STATUS(njb,status);
	  __leave;
	  return -1;
	}

	__leave;
	return bread;
}

int njb_request_file (njb_t *njb, u_int32_t fileid)
{
	__dsub= "njb_request_track";
	unsigned char data[4];

	__enter;

	memset(data, 0, 4);
	from_32bit_to_njb1_bytes(fileid, &data[0]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_REQUEST_TRACK, 1, 0, 4, data) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_transfer_complete (njb_t *njb)
{
	__dsub= "njb_transfer_complete";
	unsigned char data = '\0';

	__enter;

	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_TRANSFER_COMPLETE, 0, 0, 1, &data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data && (data & 0xf) ) {
		NJB_STATUS(njb,data);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_send_track_tag (njb_t *njb, njbttaghdr_t *tagh, void *tag)
{
	__dsub= "njb_send_track_tag";
	unsigned char utagsz[4];
	unsigned char data[5];
	ssize_t bread, bwritten;

	__enter;

	memset(utagsz, 0, 4);
	memset(data, 0, 5);
	
	from_32bit_to_njb1_bytes(tagh->size, &utagsz[0]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_SEND_TRACK_TAG, 0, 0, 4, utagsz) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	bwritten = usb_pipe_write(njb, tag, tagh->size);
	if ( bwritten == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		return -1;
	} else if ( bwritten < tagh->size ) {
		NJB_ERROR(njb, EO_WRSHORT);
		__leave;
		return -1;
	}

	bread = usb_pipe_read(njb, data, 5);
	if ( bread == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		return -1;
	} else if ( bread < 5 ) {
		NJB_ERROR(njb, EO_RDSHORT);
		__leave;
		return -1;
	}

	if ( data[0] ) {
		NJB_STATUS(njb,data[0]);
		__leave;
		return -1;
	}

	tagh->trackid = njb1_bytes_to_32bit(&data[1]);

	__leave;
	return 0;
}

int njb_send_datafile_tag (njb_t *njb, njbdfhdr_t *dfh, void *tag)
{
	__dsub= "njb_send_datafile_tag";
	unsigned char utagsz[4];
	unsigned char data[5];
	unsigned char *padded_tag;
	ssize_t bread, bwritten;

	__enter;

	memset(utagsz, 0, 4);
	memset(data, 0, 5);

	/* Add five here for the header overhead */
	from_32bit_to_njb1_bytes(dfh->size+5, &utagsz[0]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_SEND_DATAFILE_TAG, 0, 0, 4, utagsz) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}
	
	/* Pad the tag with 0x0000 0x0000 <tag> 0x00
	 * The purpose of these zeroes is unknown. */

	padded_tag = (unsigned char *) malloc(dfh->size+5);
	if ( padded_tag == NULL ) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return -1;
	}

	memset(padded_tag, 0, dfh->size+5);
	memcpy(padded_tag+4, tag, dfh->size);

	/* Write the tag */

	bwritten= usb_pipe_write(njb, padded_tag, dfh->size+5);
	if ( bwritten == -1 ) {
		free(padded_tag);
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		return -1;
	} else if ( bwritten < dfh->size+5 ) {
		free(padded_tag);
		NJB_ERROR(njb, EO_WRSHORT);
		__leave;
		return -1;
	}

	free(padded_tag);

	/* Read back datafile ID */

	bread = usb_pipe_read(njb, data, 5);
	if ( bread == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		return -1;
	} else if ( bread < 5 ) {
		NJB_ERROR(njb, EO_RDSHORT);
		__leave;
		return -1;
	}

	if ( data[0] ) {
		NJB_STATUS(njb,data[0]);
		__leave;
		return -1;
	}

	dfh->dfid = njb1_bytes_to_32bit(&data[1]);

	__leave;
	return 0;
}

/*
 * This function transfers a block of <= NJB_XFER_BLOCK_SIZE to the
 * jukebox and returns the number of bytes actually sent. Short transfers
 * are OK but must be accounted for.
 */
u_int32_t njb_send_file_block (njb_t *njb, void *data, u_int32_t blocksize)
{
	__dsub= "njb_send_file_block";
	u_int32_t bwritten;
	u_int32_t msw, lsw;
	unsigned char status = 0;
	/* We may need to retry this command if the device is not ready
	 * to receive the file block. */
	int retry = 1;
	int retries = 20;

	__enter;

	if ( blocksize > NJB_XFER_BLOCK_SIZE ) {
		NJB_ERROR(njb, EO_TOOBIG);
		__leave;
		return -1;
	}

	/* THIS IS WEIRD. NOT LIKE OTHER COMMANDS THAT SIMPLY USE get_msw/get_lsw */
	msw = get_msw(blocksize);
	lsw = get_lsw(blocksize);

	while (retry) {

	  /* Only if something fails, retry */
	  retry = 0;

	  if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
			 NJB_CMD_SEND_FILE_BLOCK, lsw, msw, 1, &status)
	       == -1 ) {
	    
	    NJB_ERROR(njb, EO_USBCTL);
	    __leave;
	    return -1;
	  }

	  /* Check return status, retry if failed */
	  if ( status ) {
	    /* printf("Bad status byte in njb_send_file_block(): 0x%02x\n", status); */
	    retry = 1;
	    /* The NJB device is not following us. Sleep for a while
	     * and retry. If usleep is not available to sleep for 200ms,
	     * we'll just sleep for a second instead, just so it works
	     * either way. Tony Smolar noticed that sometimes, if the file
	     * exceeds 3 MB this need to wait for ready usually appears. 
	     * Generally, the nomad needs to wait 200-800 ms. */
#ifdef HAVE_USLEEP
	    usleep(200000);
#else
#ifdef __WIN32__
	    /* 
	     * This is because sleep() is actually deprecated in favor of
	     * Sleep() on Windows.
	     */
	    _sleep(1);
#else
	    sleep(1);
#endif
#endif
	    retries --;
	    if ( !retries ) {
	      NJB_ERROR(njb, EO_BADSTATUS);
	      __leave;
	      return -1;
	    }
	  }
	}

	bwritten = usb_pipe_write(njb, data, blocksize);

	if ( bwritten == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		return -1;
	}

	__leave;
	return bwritten;
}

int njb_stop_play (njb_t *njb)
{
	__dsub= "njb_stop_play";
	unsigned char data = '\0';

	__enter;

	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_STOP_PLAY, 0, 0, 1, &data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data ) {
		NJB_STATUS(njb,data);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_get_eax_size (njb_t *njb, u_int32_t *size)
{
	__dsub= "njb_get_eax_size";
	unsigned char data[5];

	__enter;

	memset(data, 0, 5);
	
	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_GET_EAX_SIZE, 0, 0, 5, data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data[0] ) {
		NJB_ERROR(njb, EO_BADSTATUS);
		__leave;
		return -1;
	}

	*size = njb1_bytes_to_32bit(&data[1]);

	__leave;
	return 0;
}

/**
 * This reads in the block with EAX types. It is a byte
 * chunk which is then interpreted and the EAX effects are
 * added to a list of effects which is then retrieved by
 * calls to the <code>njb_get_nexteax()</code> function.
 * 
 * @param njb a pointer to the NJB object to use
 * @param size the previously determined size of the EAX block
 */
void njb_read_eaxtypes (njb_t *njb, u_int32_t size)
{
  __dsub= "njb_read_eaxtypes";

  njb_state_t *state = (njb_state_t *) njb->protocol_state;
  unsigned char *data;
  ssize_t bread;
  u_int32_t actsize;
  unsigned char usize[4];
  __enter;

  data = (unsigned char *) malloc(size);
  if ( data == NULL ) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return;
  }
  memset(data, 0, size);
  memset(usize, 0, 4);
	
  from_32bit_to_njb1_bytes(size, &usize[0]);

  if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER, NJB_CMD_GET_EAX,
		 0, 0, 4, usize) == -1 ) {
    
    NJB_ERROR(njb, EO_USBCTL);
    free(data);
    __leave;
    return;
  }

  bread = usb_pipe_read(njb, data, size+5);
  if ( bread == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    free(data);
    __leave;
    return;
  } else if ( bread < size ) {
    NJB_ERROR(njb, EO_RDSHORT);
    free(data);
    __leave;
    return;
  }

  if ( data[0] ) {
    NJB_STATUS(njb,(int) data[0]);
    free(data);
    __leave;
    return;
  }

  actsize = njb1_bytes_to_32bit(&data[1]);

  /* Unpack the EAX structure */
  eax_unpack(&data[5], actsize, state);

  free(data);

  __leave;
  return;
}

/**
 * This retrieves an EAX effect from the list of
 * effects read in by the previous functions.
 *
 * @param njb a pointer to the NJB object to use
 */
njb_eax_t *njb_get_nexteax(njb_t *njb)
{
  njb_state_t *state = (njb_state_t *) njb->protocol_state;
  njb_eax_t *ret;

  ret = state->next_eax;
  if (ret != NULL) {
    state->next_eax = ret->next;
  } else {
    /* 
     * When we have returned the last EAX type, make sure
     * there are no dangerous pointers into the void
     */
    state->first_eax = NULL;
  }
  return ret;
}

/*
 * This routine is dangerous. At one time, the NJB1 would
 * use this to update the status of the EAX settings, but
 * with newer firmware it has become increasingly unreliable.
 * So we simply do not use it, re-read the EAX types instead. 
 */

/*
int njb_refresh_eax(njb_t *njb)
{
	__dsub= "njb_refresh_eax";
	unsigned char *data;
	u_int32_t actsize = 10;
	u_int32_t bread;

	__enter;

	data= malloc(4);
	if (data == NULL) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return -1;
	}
	memset(data, 0, 4);
	
	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
			NJB_CMD_GET_EAX, 1, 0, 4, data) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		free(data);
		__leave;
		return -1;
	}

	// In newer firmware, the four bytes returned in data
	// contain rubbish, and can not be used for determining
	// the size of the EAX parameters. You have to discard it.
	free(data);

	// 15 bytes + Status byte + 4 bytes size (again)
	data= malloc(20);
	if (data == NULL) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return -1;
	}

	// Read from bulk
	if ( (bread= usb_pipe_read(njb, data, actsize+5))
		== -1 ) {

		NJB_ERROR(njb, EO_USBBLK);
		free(data);
		__leave;
		return -1;
	} else if ( bread < actsize+5 ) {
		NJB_ERROR(njb, EO_RDSHORT);
		free(data);
		__leave;
		return -1;
	}
	
	if ( data[0] ) {
		NJB_ERROR(njb, EO_BADSTATUS);
		free(data);
		__leave;
		return -1;
	}

	actsize = njb1_bytes_to_32bit(&data[1]);

	eax_refresh(eax, data, actsize);
	free(data);

	__leave;
	return 0;
}
*/

njb_time_t *njb_get_time(njb_t *njb)
{
	__dsub= "njb_get_time";
	njb_time_t *time = NULL;
	unsigned char *data;

	__enter;

	data= malloc(17);
	if (data == NULL) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return NULL;
	}
	memset(data, 0, 17);
	
       	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
       			NJB_CMD_GET_TIME, 0, 0, 17, data) == -1 ) {
       		NJB_ERROR(njb, EO_USBCTL);
		free(data);
       		__leave;
       		return NULL;
       	}
       	if ( data[0] ) {
       		NJB_ERROR(njb, EO_BADSTATUS);
		free(data);
       		__leave;
       		return NULL;
       	}
       	time = time_unpack(&data[1]);
	
	free(data);

	__leave;
	return time;
}

int njb_set_time(njb_t *njb, njb_time_t *time)
{
	__dsub= "njb_set_time";
	unsigned char *data;
	
	__enter;

       	data= time_pack(time);

       	if ( data == NULL ) {
       		__leave;
       		return -1;
       	}

       	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
       		NJB_CMD_SET_TIME, 0, 0, 16, data) == -1 ) {

       		NJB_ERROR(njb, EO_USBCTL);
       	free(data);
       		__leave;
       		return -1;
       	}
       	free(data);

	__leave;
	return 0;
}

int njb_create_playlist (njb_t *njb, char *name, u_int32_t *plid)
{
	__dsub= "njb_create_playlist";
	ssize_t size, bread, bwritten;
	u_int16_t msw, lsw;
	unsigned char retdata[5];

	__enter;

	size= strlen(name) + 1;
	if ( size > 0xffffffff ) {
		NJB_ERROR(njb, EO_TOOBIG);
		__leave;
		return 0;
	}

	memset(retdata, 0, 5);
	
	msw= get_msw( (u_int32_t) size );
	lsw= get_lsw( (u_int32_t) size );

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_CREATE_PLAYLIST, lsw, msw, 0, NULL) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	bwritten= usb_pipe_write(njb, name, size);
	if ( bwritten == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		return -1;
	} else if ( bwritten < size ) {
		NJB_ERROR(njb, EO_WRSHORT);
		__leave;
		return -1;
	}

	bread= usb_pipe_read(njb, retdata, 5);
	if ( bread == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		return -1;
	} else if ( bread < 5 ) {
		NJB_ERROR(njb, EO_RDSHORT);
		__leave;
		return -1;
	}

	if ( retdata[0] ) {
		NJB_STATUS(njb,retdata[0]);
		__leave;
		return -1;
	}

	*plid = njb1_bytes_to_32bit(&retdata[1]);

	__leave;
	return 0;
}

int njb_delete_playlist (njb_t *njb, u_int32_t plid)
{
	__dsub= "njb_delete_playlist";
	u_int16_t msw, lsw;
	unsigned char data = '\0';

	__enter;

	msw= get_msw(plid);
	lsw= get_lsw(plid);

	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_DELETE_PLAYLIST, msw, lsw, 1, &data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data ) {
		NJB_STATUS(njb,data);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_rename_playlist (njb_t *njb, u_int32_t plid, char *name)
{
	__dsub= "njb_rename_playlist";
	size_t size;
	ssize_t bwritten;
	unsigned char data[8];

	__enter;

	memset(data, 0, 8);
	
	size= strlen(name) + 1;
	if ( size > 0xffffffff ) {
		NJB_ERROR(njb, EO_TOOBIG);
		__leave;
		return 0;
	}

	from_32bit_to_njb1_bytes(plid, &data[0]);
	from_32bit_to_njb1_bytes(size, &data[4]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_RENAME_PLAYLIST, 0, 0, 8, data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	bwritten= usb_pipe_write(njb, name, size);
	if ( bwritten == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		return -1;
	} else if ( bwritten < size ) {
		NJB_ERROR(njb, EO_WRSHORT);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_add_track_to_playlist (njb_t *njb, u_int32_t plid, u_int32_t trid)
{
	__dsub= "njb_add_track_to_playlist";
	unsigned char data[10];

	__enter;

	memset(data, 0, 10);
	from_32bit_to_njb1_bytes(trid, &data[2]);
	from_32bit_to_njb1_bytes(plid, &data[6]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_ADD_TRACK_TO_PLAYLIST, 0, 0, 10, data) == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_add_multiple_tracks_to_playlist (njb_t *njb, u_int32_t plid, 
	u_int32_t *trids, u_int16_t ntracks)
{
	__dsub= "njb_add_multiple_tracks_to_playlist";
	unsigned char data[6];
	unsigned char *block, *bp;
	u_int16_t i;
	u_int32_t bsize;
	ssize_t bwritten;

	__enter;

	bsize= ntracks*6;
	block= (unsigned char *) malloc(bsize);
	if ( block == NULL ) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return -1;
	}
	memset(block, 0, bsize);
	memset(data, 0, 6);
	
	bp= block;
	for (i= 0; i< ntracks; i++) {
		memset(bp, 0, 2);
		bp+= 2;
		from_32bit_to_njb1_bytes(trids[i], &bp[0]);
		bp+= 4;
	}

	from_32bit_to_njb1_bytes(plid, &data[0]);
	from_16bit_to_njb1_bytes(ntracks, &data[4]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_ADD_MULTIPLE_TRACKS_TO_PLAYLIST, 0, 0, 6, data) 
		== -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		free(block);
		__leave;
		return -1;
	}

	bwritten= usb_pipe_write(njb, block, bsize);
	if ( bwritten == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		free(block);
		__leave;
		return -1;
	} else if ( bwritten < bsize ) {
		NJB_ERROR(njb, EO_WRSHORT);
		free(block);
		__leave;
		return -1;
	}

	free(block);

	__leave;
	return 0;
}

int njb_delete_track (njb_t *njb, u_int32_t trackid)
{
	__dsub= "njb_delete_track";
	u_int16_t msw, lsw;
	unsigned char data = '\0';

	__enter;

	msw= get_msw(trackid);
	lsw= get_lsw(trackid);

	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_DELETE_TRACK, msw, lsw, 1, &data) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data ) {
		NJB_STATUS(njb,data);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_delete_datafile (njb_t *njb, u_int32_t fileid)
{
	__dsub= "njb_delete_track";
	u_int16_t msw, lsw;
	unsigned char data = '\0';

	__enter;

	msw= get_msw(fileid);
	lsw= get_lsw(fileid);

	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_DELETE_DATAFILE, msw, lsw, 1, &data) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data ) {
		NJB_STATUS(njb,data);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_play_or_queue (njb_t *njb, u_int32_t trackid, int cmd)
{
	__dsub= "njb_play_or_queue";
	unsigned char data[4];

	__enter;

	memset(data, 0, 4);
	
	from_32bit_to_njb1_bytes(trackid, &data[0]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER, cmd, 0, 0, 4, data)
		 == -1 ) {

		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

int njb_elapsed_time (njb_t *njb, u_int16_t *elapsed, int *change)
{
	__dsub= "njb_get_elapsed_time";
	unsigned char data[3];

	__enter;

	memset(data, 0, 3);
	
	if ( usb_setup(njb, UT_READ_VENDOR_OTHER,
		NJB_CMD_ELAPSED_TIME, 0, 0, 3, data) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	if ( data[0] == NJB_MSG_QUEUED_AUDIO_STARTED ) {
		*change= 1;
	} else if ( data[0] == 0 ) {
		*change= 0;
	} else {
		NJB_STATUS(njb,data[0]);
		__leave;
		return -1;
	}

	*elapsed = njb1_bytes_to_16bit(&data[1]);

	__leave;
	return 0;
}

int njb_replace_track_tag (njb_t *njb, njbttaghdr_t *tagh, void *tag)
{
	__dsub = "njb_replace_track_tag";
	u_int16_t msw, lsw;
	unsigned char *data;
	ssize_t bwritten;

	__enter;

	msw = get_msw(tagh->size);
	lsw = get_lsw(tagh->size);

	data = (unsigned char *) malloc(tagh->size+4);
	if ( data == NULL ) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return -1;
	}
	memset(data, 0, tagh->size+4);

	from_32bit_to_njb1_bytes(tagh->trackid, &data[0]);
	memcpy(&data[4], tag, tagh->size);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_REPLACE_TRACK_TAG, lsw, msw, 0, NULL) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		free(data);
		__leave;
		return -1;
	}

	bwritten = usb_pipe_write(njb, data, tagh->size+4);
	if ( bwritten == -1 ) {
		NJB_ERROR(njb, EO_USBBLK);
		__leave;
		free(data);
		return -1;
	} else if ( bwritten < (tagh->size+4) ) {
		NJB_ERROR(njb, EO_WRSHORT);
		free(data);
		__leave;
		return -1;
	}

	free(data);
	__leave;
	return 0;
}

int njb_adjust_sound(njb_t *njb, u_int8_t effect, int16_t value)
{
	__dsub= "njb_adjust_sound";
	unsigned char data[3];
	u_int16_t sendvalue = ((u_int16_t) value) & 0xff;

	__enter;

	memset(data, 0, 3);
	data[0] = effect;

	/* printf("Effect %d, sending value %d (%04X)\n", effect, sendvalue, sendvalue); */

	from_16bit_to_njb1_bytes(sendvalue, &data[1]);

	if ( usb_setup(njb, UT_WRITE_VENDOR_OTHER,
		NJB_CMD_ADJUST_SOUND, 0, 0, 3, data) == -1 ) {
		NJB_ERROR(njb, EO_USBCTL);
		__leave;
		return -1;
	}

	__leave;
	return 0;
}

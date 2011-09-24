/**
 * \file protocol3.c
 *
 * This file contains the protocol code for the series 3 devices.
 * This protocol is believed to have the internal name "PDE" at Creative.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "libnjb.h"
#include "protocol.h"
#include "protocol3.h"
#include "unicode.h"
#include "byteorder.h"
#include "njb_error.h"
#include "usb_io.h"
#include "ioutil.h"
#include "defs.h"
#include "base.h"
#include "eax.h"
#include "datafile.h"
#include "njbtime.h"

extern int __sub_depth;

/*
 * NJB2,3,Zen,Zen USB 2.0, Zen NX, Zen Xtra and Dell Digital DJ
 * specific functions in the protocol goes 
 * in here. This is a work in progress, not all of the NJB3-style 
 * protocol instructions are yet implemented.
 */

/**
 * Initializes the basic state of the njb->protocol_state for the
 * PROTOCOL3-devices.
 */
int njb3_init_state (njb_t *njb) {
  __dsub= "njb3_init_state";

  njb3_state_t *state;

  __enter;

  state = malloc(sizeof(njb3_state_t));
  if (state == NULL) {
    __leave;
    return -1;
  }
  state->get_extended_tag_info = 0;
  state->first_songid = NULL;
  state->next_songid = NULL;
  state->first_plid = NULL;
  state->next_plid = NULL;
  state->first_dfid = NULL;
  state->next_dfid = NULL;
  state->current_playing_track = 0;
  state->first_key = NULL;
  state->next_key = NULL;
  state->first_eax = NULL;
  state->next_eax = NULL;
  state->eax_processor_active = 0;
  state->product_name = NULL;
  njb->protocol_state = (unsigned char *) state;
  state->fwMajor = 0;
  state->fwMinor = 0;
  state->fwRel = 0;
  state->hwMajor = 0;
  state->hwMinor = 0;
  state->hwRel = 0;
  state->turbo_mode = NJB_TURBO_ON;

  __leave;
  return 0;
}

/**
 * This sends a raw command to the device. A command header "command follows"
 * with the length of the actual command is sent first, then a second bulk 
 * transfer will send the actual command. The command is used also for sending 
 * things like file chunks and firmware upgrade chunks.
 *
 * @param njb a pointer to the device object to use
 * @param command a pointer to some raw bytes representing a command
 * @param clength the length of the command in the raw byte buffer
 * @return 0 on success, -1 on failure
 */
static int send_njb3_command(njb_t *njb, unsigned char *command, u_int32_t clength)
{
  __dsub= "send_njb3_command";
  
  unsigned char *data;

  /* The header differs for NJB2 and NJB Zen USB 2.0 */
  unsigned char usb11_cmd_header[]={0x43,0x42,0x53,0x55,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
				   0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
  unsigned char usb20_cmd_header[]={0x55,0x53,0x42,0x43,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0, 0x0,0x0,0x0,0x0,
				   0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
  ssize_t cmd_len;

  /*
   * Structure of "command follows" command: 0x20 bytes
   * 4 bytes unknown 0x43 0x42 0x53 0x55 comprising the string "CBSU", for NJB2 "USBC" (byte order??)
   * 4 bytes unknown 0x00 * 4
   * 4 bytes total command length (or is it 8 bytes even?)
   * 20 bytes unknown 0x00 * 20
   */
  
  __enter;
  
  data = (unsigned char *) malloc (0x20);
  if ( data == NULL ) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }

  /* Use the apropriate header on USB 2.0 devices as
   * opposed to USB 1.1 devices. */
  if (njb_device_is_usb20(njb)) {
    memcpy(data, usb20_cmd_header, 0x20);
    cmd_len = 0x1F;
  } else {
    memcpy(data, usb11_cmd_header, 0x20);
    cmd_len = 0x20;
  }

  /* Set command length parameter */
  from_32bit_to_njb3_bytes(clength, &data[8]);
  
  if (usb_pipe_write(njb, data, cmd_len) == -1) {
    free(data);
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  }
  
  free(data);

  if (usb_pipe_write(njb, command, clength) == -1) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  }
  
  __leave;
  return 0;
}

/**
 * This helper function just reads in the two status bytes
 * returned by many operations.
 * @param njb a pointer to the device object to use
 * @param status a pointer to the status number
 * @return 0 on success, -1 on failure
 */
static int njb3_get_status(njb_t *njb, u_int16_t *status) {
  __dsub = "njb3_get_status";
  unsigned char statusbytes[]={0x00,0x00};
  ssize_t bread;

  __enter;
  /* Read back status (command result) */
  if ( (bread = usb_pipe_read (njb, statusbytes, 2)) == -1 ) {
    NJB_ERROR(njb,EO_USBBLK);
    __leave;
    return -1;
  }
  else if (bread < 2) {
    NJB_ERROR(njb,EO_RDSHORT);
    __leave;
    return -1;
  }
  *status = njb3_bytes_to_16bit(&statusbytes[0]);
  __leave;
  return 0;
}


int njb3_capture (njb_t *njb)
{
  __dsub= "njb3_capture";
  
  __enter;
  
  if ( usb_setup(njb, UT_CLASS, NJB_CMD_CAPTURE_NJB3, 0, 0, 0, NULL) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    goto errcap;
  }
  
  __leave;
  return 0;
  
 errcap:
  usb_setup(njb, UT_CLASS, NJB_CMD_RELEASE_NJB3, 0, 0, 0, NULL);
  __leave;
  return -1;
}


int njb3_release(njb_t *njb)
{
  __dsub= "njb3_release";
  
  __enter;
  
  if (usb_setup(njb, UT_CLASS, NJB_CMD_RELEASE_NJB3, 0, 0, 0, NULL) == -1) {
    __leave;
    return -1;
  }
  
  __leave;
  return 0;
}

/**
 * Reads the supported audio file types.
 */
int njb3_get_codecs(njb_t *njb)
{
  __dsub= "njb3_read_codecs";
  ssize_t bread;
  unsigned char data[256];
  unsigned char njb3_read_codecs[]={0x00,0x08,0x00,0x01,0xff,0xfe,0x00,0x02,0x00,0x01,0x00,0x00};
  u_int16_t status;
  u_int16_t codecs;
  u_int16_t supports_wav;
  u_int16_t supports_mp3;
  u_int16_t supports_wma;
  int i;
  /* Structure
   * 2 bytes command 0x0008
   * 2 bytes unknown 0x0001 (part of command?)
   * 2 bytes unknown 0xfffe - transfer max length, including status = 64KB?
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x0001
   * 2 bytes termination 0x0000
   */
  
  /* From old NJB3:  0000 0008 0001 0001 0001 ffff 0000 */
  /*                 Stat  Len  yes  yes  yes term      */
  /* Audio file type support:   WAV  MP3  WMA           */
  
  __enter;
  
  if (send_njb3_command(njb, njb3_read_codecs, 0x0c) == -1){
    __leave;
    return -1;
  }
  if ( (bread= usb_pipe_read(njb, data, 256)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 2 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }
  status = njb3_bytes_to_16bit(&data[0]);
  if ( status != NJB3_STATUS_OK ) {
    printf("LIBNJB Panic: njb3_read_codecs returned status code %04x!\n", status);
    /* Value 0x0002 = busy, same as on NJB1 */
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  /* Count the codecs */
  codecs = 0;
  i = 4;
  while (data[i] != 0xFFU && data[i+1] != 0xFF) {
    i += 2;
    codecs ++;
  }
  /*
   * I really, really wonder if those who created the RealAudio
   * Helix DRM and the Audible.com firmware cared to modify the
   * stuff returned by this command. Presumably not.
   */
  if (codecs > 3) {
    printf("LIBNJB notification: this device supports more than 3 codecs!\n");
  }
  supports_wav = njb3_bytes_to_16bit(&data[4]);
  supports_mp3 = njb3_bytes_to_16bit(&data[6]);
  supports_wma = njb3_bytes_to_16bit(&data[8]);
  /* Read OK */
  __leave;
  return 0;
}

int njb3_ping (njb_t *njb, int type)
{
  __dsub= "njb3_ping";
  ssize_t bread;
  unsigned char data[256];
  /*                                            -0x00 0x80- on first call           */
  /*                                            -0xff 0xfe- on subsequent calls...? */
  unsigned char njb3_ping0[]={0x00,0x08,0x00,0x01,0xff,0xfe,0x00,0x02,0x00,0x03,0x00,0x00,0x00,0x00};
  unsigned char njb3_ping1[]={0x00,0x08,0x00,0x01,0xff,0xfe,0x00,0x02,0x00,0x03,0x00,0x00};
  u_int16_t status;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
  
  /* Structure
   * 2 bytes unknown 0x0008 (could mean "read operation")
   * 2 bytes unknown 0x0001
   * 2 bytes unknown 0x0080
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x0003 = Product ID, FW rel, HW rel, prodid string
   * 2 bytes termination 0x0000
   */
  
  __enter;
  
  /*
   * Some special handling for USB 2.0 devices, perhaps this should be 
   * used for NJB3/ZEN too?
   */
  if (njb_device_is_usb20(njb)) {
    if (njb3_capture(njb) == -1)
      goto err1;
  }
  /* Here we used to retrieve the play status - useless. */
  
  /* Retrieve the device identification, 52 bytes for NJB2/3
   * 56 bytes for NJB Zen / Zen 2.0 */
  if (type == 0) {
    if (send_njb3_command(njb, njb3_ping0, 0x0c) == -1)
      goto err1;
  } else {
    if (send_njb3_command(njb, njb3_ping1, 0x0c) == -1)
      goto err1;
  }
  
  /* We don't check for the length of this */
  if ( (bread= usb_pipe_read(njb, data, 256)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    goto err1;
  }
  
  status = njb3_bytes_to_16bit(&data[0]);
  if ( status != NJB3_STATUS_OK ) {
    /* Value 0x0002 = busy, same as on NJB1 */
    printf("LIBNJB Panic: njb3_ping returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    if (njb_device_is_usb20(njb)) {
      njb3_release(njb);
      /* 
       * This will still lock up the NJB, 
       * don't know how to do this properly.
       */
    }
    goto err1;
  }
  
  /* Special NJB2 fix from Berni to release/capture NJB2. Is this applicable
   * for NJB3/ZEN too? We applied it to all USB 2.0 devices. */
  if (njb_device_is_usb20(njb)) {
    if (njb3_release(njb) == -1)
      goto err1;
    if (njb3_capture(njb) == -1)
      goto err1;
  }

  /* Varibles kept in the device state */  
  /* Fetch software revision */
  state->fwMajor = (u_int8_t) data[7];
  state->fwMinor = (u_int8_t) data[9];
  state->fwRel = (u_int8_t) data[11];
  /* Fetch hardware revision */
  state->hwMajor = (u_int8_t) data[13];
  state->hwMinor = (u_int8_t) data[15];
  state->hwRel = (u_int8_t) data[17];
  if (state->product_name != NULL) {
    free(state->product_name);
  }
  state->product_name = ucs2tostr(&data[18]);
  
  __leave;
  return 0;
 err1:
  __leave;
  return -1;
}

/**
 * This function reads out the current battery level and charging
 * status from a series 3 device.
 * @param battery_level a variable that will hold the current level after
 *        the call.
 * @param charging if the battery is charging, this variable 
 *        will hold 1 after the call, else 0.
 * @param ac_power if the charger is connected, this variable 
 *        will hold 1 after the call, else 0.
 * @return 0 on success, -1 on failure. If the call fails, all other return
 *        values are invalid.
 */
int njb3_power_status (njb_t *njb, int *battery_level, int *charging, int *ac_power)
{
  __dsub= "njb3_battery_status";
  ssize_t bread;
  u_int16_t status;
  u_int8_t powerstat;
  unsigned char data[256];
  unsigned char njb3_battery_cmd[]={0x00,0x08,0x00,0x01,0x01,0x00,0x00,0x02,0x01,0x14,0x00,0x00};
  /* Structure
   * 2 bytes 0x0008 Read device info
   * 2 bytes unknown 0x0001
   * 2 bytes 0x0100 Recieve buffer size
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x0114 = Battery status
   * 2 bytes termination 0x0000
   */
  
  __enter;
  
  /* This retrieves the jukebox ID */
  if (send_njb3_command(njb, njb3_battery_cmd, 0x0c) == -1){
    __leave;
    return -1;
  }
  if ( (bread = usb_pipe_read(njb, data, 0x0100U)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 12 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }
  status = njb3_bytes_to_16bit(&data[0]);
  if ( status != NJB3_STATUS_OK ) {
    /* Value 0x0002 = busy, same as on NJB1 */
    printf("LIBNJB Panic: njb3_battery_status returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  
  /*
   * Data is contained in data[6] ... data[9] (4 bytes)
   * The first byte appear to always contain 0x14.
   * The second value is interpreted as follows:
   * 0x00 = power connected and charging
   * 0x01 = power connected, battery fully charged
   * 0x02 = power disconnected
   * Then follows the battery level as a 16bit value.
   */
  /*
    printf("Battery status bytes:\n");
    data_dump(stdout, &data[6], bread-8);
    printf("\n");
  */
  powerstat = data[7];
  if (powerstat == 0x00) {
    *charging = 1;
    *ac_power = 1;
  } else if (powerstat == 0x01) {
    *charging = 0;
    *ac_power = 1;
  } else if (powerstat == 0x02) {
    *charging = 0;
    *ac_power = 0;
  } else {
    *charging = 0;
    *ac_power = 0;
    printf("LIBNJB panic: unknown power status %02x\n", powerstat);
  }
  *battery_level = njb3_bytes_to_16bit(&data[8]);
  
  __leave;
  return 0;
}

int njb3_readid (njb_t *njb, u_int8_t *sdmiid)
{
  __dsub= "njb3_readid";
  ssize_t bread;
  u_int16_t status;
  unsigned char data[256];

  /*                                              -0x00 0x80- on first call            */
  /*                                              -0xff 0xfe- on subsequent calls...?  */
  /*                                              - Must be recieveing buffer size */
  unsigned char njb3_readid[]={0x00,0x08,0x00,0x01,0xff,0xfe,0x00,0x02,0x00,0x15,0x00,0x00};
  /* Structure
   * 2 bytes 0x0008 Read device info
   * 2 bytes unknown 0x0001
   * 2 bytes 0x0080/0xfffe Buffer size
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x0015 = NJB device ID, 16 bytes
   * 2 bytes termination 0x0000
   */
  
  __enter;
  
  /* This retrieves the jukebox ID */
  if (send_njb3_command(njb, njb3_readid, 0x0c) == -1){
    __leave;
    return -1;
  }
  if ( (bread = usb_pipe_read(njb, data, 256)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 24 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }
  status = njb3_bytes_to_16bit(&data[0]);
  if ( status != NJB3_STATUS_OK ) {
    /* FIXME: handle status codes more correctly */
    /* Value 0x0002 = busy, same as on NJB1 */
    printf("LIBNJB Panic: njb3_read_string_frame returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  
  memcpy(sdmiid, &data[6], 16);
  
  __leave;
  return 0;
}


int njb3_get_disk_usage (njb_t *njb, u_int64_t *totalbytes, u_int64_t *freebytes)
{
  __dsub= "njb3_get_disk_usage";
  u_int32_t lsdw, msdw;
  unsigned char data[0x14];
  
  /*
    unsigned char get_files_and_dirs[]={0x00,0x08,0x00,0x01,0xff,0xfe,0x00,0x02,0x00,0x13,0x00,0x00};
  */
  /* Structure
   * 2 bytes unknown 0x0008 (could mean "read operation")
   * 2 bytes unknown 0x0001
   * 2 bytes unknown 0xfffe
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x0013
   * 2 bytes termination 0x0000
   */
  unsigned char get_disk_usage[]={0x00,0x08,0x00,0x01,0xff,0xfe,0x00,0x02,0x00,0x02,0x00,0x00};
  /* Structure
   * 2 bytes unknown 0x0008 (could mean "read operation")
   * 2 bytes unknown 0x0001
   * 2 bytes unknown 0xfffe
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x0002
   * 2 bytes termination 0x0000
   */
  
  ssize_t bread;
  
  __enter;

  if (send_njb3_command(njb, get_disk_usage, 0x0c) == -1) {
    __leave;
    return -1;
  }
  if ( (bread = usb_pipe_read(njb, data, 0x14)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 0x14 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }
  
  msdw = 0;
  
  lsdw = njb3_bytes_to_32bit(&data[0x0a]);
  *totalbytes=  make64(msdw, lsdw) * 1024;
  lsdw = njb3_bytes_to_32bit(&data[0x0e]);
  *freebytes=  make64(msdw, lsdw) * 1024;
  
  /* FIXME: This command returns the files and playlist counts etc, it is
   *        currently not used for anything useful. Commented out in order
   *        not to cause problems.
   */
  /*
    if (send_njb3_command(njb, get_files_and_dirs, 0x0c) == -1) {
    __leave;
    return -1;
    }
    if ( (bread= usb_pipe_read(njb, data, 0x0c)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
    } else if ( bread < 0x0c ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
    }
  */

  __leave;
  return 0;
}

// This is probably the way to retrieve singe string frames. 
// Short reads on a large buffer. Currently only used for
// getting the owner string.
static char* njb3_read_string_frame(njb_t *njb, u_int16_t frameid)
{
  __dsub= "njb3_read_string_frame";
  unsigned char get_frame[]={0x00,0x08,0x00,0x01,0xff,0xfe,0x00,0x02,0x00,0x00,0x00,0x00};
  /* Structure
   * 2 bytes unknown 0x0008 (could mean "read string operation")
   * 2 bytes unknown 0x0001
   * 2 bytes unknown 0xfffe - magic numbers... first 0x0080 then 0xfffe?
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name (supplied as parameter)
   * 2 bytes termination 0x0000
   *
   * returned structure is:
   * 2 bytes status 0x0000
   * 2 bytes length, in bytes of return
   * 2 bytes frame ID
   * n bytes of data, usually terminated by 0000
   */
  ssize_t bread;
  // Reading buffer
  unsigned char data[NJB3_SHORTREAD_BUFSIZE];
  u_int16_t status;
  u_int16_t strlen;
  char *tmpstr;

  __enter;

  from_16bit_to_njb3_bytes(frameid, &get_frame[8]);

  if (send_njb3_command(njb, get_frame, 0x0c) == -1){
    __leave;
    return NULL;
  }

  // fprintf(stdout, 
  // "Reading varlen string frame 0x%04x max 1024 bytes...\n", frameid);

  if ( (bread= usb_pipe_read(njb, data, NJB3_SHORTREAD_BUFSIZE)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return NULL;
  } else if ( bread < 2 ) {
    // We need atleast the string length!
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return NULL;
  }

  status = njb3_bytes_to_16bit(&data[0]);
  strlen = njb3_bytes_to_16bit(&data[2]);
  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_read_string_frame returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return NULL;
  }

  // Extract string value
  if (strlen == 0) {
    __leave;
    return NULL;
  }  
  tmpstr = ucs2tostr(&data[6]);
  
  __leave;
  return tmpstr;
}

/**
 * This dumps the contents of any NJB device register
 */
static int njb3_dump_device_register(njb_t *njb, u_int16_t njbreg)
{
  __dsub= "njb3_dump_device_register";
  unsigned char read_register[] =
    {0x00, 0x08, 0x00, 0x01, 0xff, 0xfe, 
     0x00, 0x02, 0x01, 0x1a, 0x00, 0x00};
  unsigned char data[256];
  size_t bread;
  u_int16_t status;

  __enter;

  from_16bit_to_njb3_bytes(njbreg, &read_register[8]);  

  if (send_njb3_command (njb, read_register, 0x0c) == -1) {
    __leave;
    return -1;
  }

  if ((bread = usb_pipe_read(njb, data, 256)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 2 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }
  status = njb3_bytes_to_16bit(&data[0]);

  // Then dump it out
  printf("Return from register %04x:\n", njbreg);
  data_dump_ascii(stdout, data, bread, 0);
  
  __leave;
  return 0;
}

int njb3_turnoff_flashing(njb_t *njb)
{
  __dsub= "njb3_turnoff_flashing";

  unsigned char njb3_turnoff_flashing[] = {
    0x00, 0x07, 0x00, 0x01, 0x00, 0x04, 0x01, 0x1a,
    0x1a, 0x00, 0x00, 0x00
  };
  /* Structure: 12 bytes
   * 2 bytes command 0x0007 (device control)
   * 2 bytes command 0x0001
   * 2 bytes length 0x0004
   * 2 bytes paramenter 0x011a
   * 2 bytes value 0x1a00
   * 2 bytes termination 0x0000
   */
  u_int16_t status;
  u_int16_t spoon;

  for (spoon = 0x0000; spoon < 0x200; spoon++) {
    njb3_dump_device_register(njb, spoon);
  }
  
  if (send_njb3_command (njb, njb3_turnoff_flashing, 0x0c) == -1) {
    __leave;
    return -1;
  }

  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }

  if ( status != NJB3_STATUS_OK ) {
    printf("LIBNJB Panic: njb3_turnoff_flashing() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }

  __leave;
  return 0;
}

int njb3_get_owner_string (njb_t *njb, char *name)
{
  __dsub= "njb3_get_owner_string";
  char *tmp;
  
  __enter;
  
  tmp = njb3_read_string_frame(njb, NJB3_OWNER_FRAME_ID);
  if (tmp == NULL) {
    __leave;
    return -1;
  }
  
  /* The destination field is no bigger than OWNER_STRING_LENGTH
   * so this is the practical limit. The result if ucs2tostr() may
   * be > 100 because of Unicode characters as UTF-8 */
  strncpy(name, tmp, OWNER_STRING_LENGTH);
  free(tmp);
  name[OWNER_STRING_LENGTH]= '\0'; /* Just in case */
  __leave;
  return 0;
}

int njb3_set_owner_string (njb_t *njb, const char *name)
{
  __dsub= "njb3_set_owner_string";
  unsigned char njb3_set_owner[]={0x00,0x07,0x00,0x01,0x00,0x00,0x01,0x13};
  
  /* TODO: this seems to overwrite just as many characters there are in
   * the supplied string argument, perhaps it has to be padded with trailing
   * spaces up to 100 bytes (max string length)?
   *
   * Structure: 8 bytes
   * 2 bytes unknown 0x0007 (could mean "write operation")
   * 2 bytes unknown 0x0001
   * 2 bytes string length
   * 2 bytes owner string tag 0x0113
   * then follows the n bytes of the owner string.
   */
  unsigned char *unistr;
  unsigned char *data;
  u_int16_t status;
  int unilen;
  int cmdlen;
  
  __enter;
  
  /* 8 bytes header
   * Unicode string length + extra 0x00 0x00
   */
  unistr = strtoucs2((unsigned char *) name);
  unilen = ucs2strlen(unistr)*2;
  cmdlen = 8 + unilen + 4;
  
  data = (unsigned char *) malloc (cmdlen);
  if ( data == NULL ) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  memset(data, 0, cmdlen);
  
  memcpy(&data[0], njb3_set_owner, 0x08);
  memcpy(&data[8], unistr, unilen+2);
  from_16bit_to_njb3_bytes(unilen+4, &data[4]);
  
  if (send_njb3_command(njb, data, cmdlen) == -1){
    free(data);
    __leave;
    return -1;
  }
  
  free(data);
  
  /* Then read two confirmation bytes back as status, only
   * verified for NJB Zen USB 2.0 */
  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }
  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_set_owner returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  
  __leave;
  return 0;
}

njb_time_t *njb3_get_time(njb_t *njb)
{
  __dsub= "njb3_get_time";
  unsigned char njb3_get_time[]={0x00,0x08,0x00,0x01,0xff,0xfe,0x00,0x02,0x01,0x10,0x00,0x00};
  /* Structure: 8 bytes
   * 2 bytes unknown 0x0008 (could mean "read operation")
   * 2 bytes unknown 0x0001
   * 2 bytes unknown 0xfffe
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x0110
   * 2 bytes termination 0x0000
   */
  njb_time_t *time = NULL;
  unsigned char *data;
  ssize_t bread;
  
  
  __enter;
  
  data = malloc(16);
  if (data == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return NULL;
  }
  
  if (send_njb3_command(njb, njb3_get_time, 0x0c) == -1){
    __leave;
    return NULL;
  }
  if ( (bread = usb_pipe_read(njb, data, 16)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return NULL;
  }
  
  /* NOT USED? SHORT READ? This need to be investigated.
     else if ( bread < 16 ) {
     NJB_ERROR(njb, EO_RDSHORT);
     __leave;
     return NULL;
     }
  */
  
  time = time_unpack3(data);
  
  free(data);
  
  __leave;
  return time;
}

/**
 * NJB2 only (shall be expanded for all jukeboxes).
 * Set up the black/white bitmap shown on shutdown.
 *
 * The bitmap must be exactly 1088 bytes large,
 * coded as a serial bitmap of 136x64 pixels. The
 * final 4 bits per line are not shown, because the
 * the display of the NJB2 has only 132x64 pixels.
 *
 * A set pixel (1) means dark, an unset means white.
 *
 * To create a compatible bitmap, take a 132x64 PBM file
 * (of course not a ASCII-coded, but a binary "P4") and
 * cut off the header.
 *
 * Explaination of the JBM1 file format:
 * <pre>
 * Byte offset:     Contents:
 * 0x00 - 0x03      "JBM1"
 * 0x04 - 0x05      width of bitmap in pixels 16bit 
 *                  unsigned bigendian integer
 * 0x06 - 0x07      height of bitmap in pixels 16bit
 *                  unsigned bigendian integer
 * 0x08 - 0x0b      file format version? contains 0x00000001
 *                  as a 32bit unsigned bigendian integer
 * 0x0c - fileend   actual bitmap data
 * </pre>
 *
 * The bitmap data is black-and-white and each bit represents
 * a single pixel. A set pixel is 1 and a cleared pixel is 0.
 * The bitmap is stored in 16bit unsigned integers which are 
 * little-endian, and each of these 16bit integers form a 
 * vertical, eight-pixel high and two-pixel wide "bar".
 */
int njb3_set_bitmap(njb_t *njb, u_int16_t x_size, u_int16_t y_size, 
		    const unsigned char *bitmap)
{
  __dsub= "njb3_set_bitmap";

  unsigned char announce_image[0x0A] = {
    0x00, 0x0b, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00,
    0x04, 0x2c
  };
  /* Structure:
   * 2 bytes unknown 0x000b
   * 2 bytes unknown 0x0001
   * 2 bytes unknown 0x0002 - send bitmap, (0x0001 = send firmware)
   * 4 bytes image data length 0x0000042c (1068 = 132x64 + 12)
   */
  unsigned char image_data[0x0C] = {
    0x4a, 0x42, 0x4d, 0x31, 0x00, 0x84, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x01 // followed by transcoded bitmap
  };
  /* Image header:
   * 4 bytes signature "JBM1"
   * 2 bytes width  0x0084 = 132
   * 2 bytes height 0x0040 =  64
   * 4 bytes unknown 0x00000001
   */
  /* The real image data starts at ... */
  u_int32_t totlen;
  unsigned char *totimage;
  unsigned char *image; // Pointer for transcoding
  u_int16_t status;
  int x, y;

  __enter;

  /* Set up transaction data, one byte per pixel plus 12 */
  totlen = x_size * y_size + 12;
  from_32bit_to_njb3_bytes(totlen, &announce_image[6]);

  /* Allocate a bitmap buffer */
  totimage = malloc(totlen);
  /* Check for NULL */
  if (totimage == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  memcpy(totimage, image_data, 0x0C);
  from_16bit_to_njb3_bytes(x_size, &totimage[4]);
  from_16bit_to_njb3_bytes(y_size, &totimage[6]);
  image = totimage + 0x0C;

  /* Transcode the bitmap.
   *
   * The NJB2 uses a "packed" bitmap format, therefore the serial
   * bitmap has to be transcoded:
   * The NJB2-bitmap-format is 16bit-word based. Every word defines
   * the pixels of a 2x8 area, starting with the most-significant-bit
   * in the lower left corner and ending with the lsb in the upper
   * right corner.
   * The bitmap consists of 528 of these areas. The first word defines
   * the area in the upper left corner.
   *
   * CHECK THIS CODE FOR PORTABILITY AND SUCH.
   */

  /* For better readable code ;) I broke up the transcoding into columns
   * and rows. Note that a I am talking of columns and rows of 2x8 subareas.
   *
   * Instead of 16bit-word bases transcoding 8bit samples are used (that
   * means the two columns of the 2x8 area are transcoded individually).
   */
  for (y = 0; y < 8; y++) {
    for (x = 0; x < 66; x++) {
      u_int8_t low_byte_mask  = 0x80 >> (2*(x%4));
      u_int8_t high_byte_mask = 0x40 >> (2*(x%4));

      image[y*132+x*2]
	= ((bitmap[y*136+0*17+x/4] & (high_byte_mask)) ? 0x01 : 0x00)
	| ((bitmap[y*136+1*17+x/4] & (high_byte_mask)) ? 0x02 : 0x00)
	| ((bitmap[y*136+2*17+x/4] & (high_byte_mask)) ? 0x04 : 0x00)
	| ((bitmap[y*136+3*17+x/4] & (high_byte_mask)) ? 0x08 : 0x00)
	| ((bitmap[y*136+4*17+x/4] & (high_byte_mask)) ? 0x10 : 0x00)
	| ((bitmap[y*136+5*17+x/4] & (high_byte_mask)) ? 0x20 : 0x00)
	| ((bitmap[y*136+6*17+x/4] & (high_byte_mask)) ? 0x40 : 0x00)
	| ((bitmap[y*136+7*17+x/4] & (high_byte_mask)) ? 0x80 : 0x00);
      image[y*132+x*2+1]
	= ((bitmap[y*136+0*17+x/4] & (low_byte_mask)) ? 0x01 : 0x00)
	| ((bitmap[y*136+1*17+x/4] & (low_byte_mask)) ? 0x02 : 0x00)
	| ((bitmap[y*136+2*17+x/4] & (low_byte_mask)) ? 0x04 : 0x00)
	| ((bitmap[y*136+3*17+x/4] & (low_byte_mask)) ? 0x08 : 0x00)
	| ((bitmap[y*136+4*17+x/4] & (low_byte_mask)) ? 0x10 : 0x00)
	| ((bitmap[y*136+5*17+x/4] & (low_byte_mask)) ? 0x20 : 0x00)
	| ((bitmap[y*136+6*17+x/4] & (low_byte_mask)) ? 0x40 : 0x00)
	| ((bitmap[y*136+7*17+x/4] & (low_byte_mask)) ? 0x80 : 0x00);
    }
  }

  if (send_njb3_command (njb, announce_image, 0x0a) == -1) {
    free(totimage);
    __leave;
    return -1;
  }

  if (send_njb3_command (njb, totimage, totlen) == -1) {
    free(totimage);
    __leave;
    return -1;
  }

  if (njb3_get_status(njb, &status) == -1) {
    free(totimage);
    __leave;
    return -1;
  }

  if ( status != NJB3_STATUS_OK ) {
    printf("LIBNJB Panic: njb2_set_bitmap() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    free(totimage);
    __leave;
    return -1;
  }

  free(totimage);
  __leave;
  return 0;
}

/**
 * Internal function to retrieve the time in seconds from 
 * the beginning of the currently playing track.
 */
static int get_elapsed_time(njb_t *njb, u_int16_t * elapsed)
{
  __dsub= "get_elapsed_time";

  unsigned char get_elapsed_time [0x04] = { 0x01, 0x01, 0x00, 0x01 };
  unsigned char data[4];
  ssize_t bread;

  __enter;

  if (send_njb3_command (njb, get_elapsed_time, 0x04) == -1) {
    __leave;
    return -1;
  }

  if ( (bread = usb_pipe_read (njb, data, 4)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  }
  else if (bread < 4) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }

  *elapsed = njb3_bytes_to_16bit(&data[2]);
  
  __leave;
  return 0;
}

/*
 * Internal function that updates the the protocol_state
 * to the last elapsed time, which is used for change
 * notification in njb3_elapsed_time. It is used in
 * njb3_seek_track, njb3_play_track, njb3_ctrl_playing.
 */
static int update_elapsed(njb_t *njb)
{
  __dsub = "update_elapsed";

  u_int16_t elapsed;
  
  njb3_state_t *state = njb->protocol_state;

  __enter;
      
  if (get_elapsed_time(njb, &elapsed) == -1) {
      __leave;
      return -1;
  }

  state->last_elapsed = elapsed;

  __leave;
  return 0;
}

/*
 * This function only returns the position in the current
 * play queue.
 */
int njb3_current_track (njb_t *njb, u_int16_t *position)
{
  __dsub= "njb3_current_track";

  unsigned char njb3_query [0x0c] = {
    0x00, 0x08, 0x00, 0x01, 0xff, 0xfe, 0x00, 0x02,
    0x01, 0x19, 0x00, 0x00
  };
  unsigned char status[0x0a];
  ssize_t bread;

  __enter;

  if (send_njb3_command (njb, njb3_query, 0x0c) == -1) {
    __leave;
    return -1;
  }

  if ( (bread= usb_pipe_read (njb, status, 0x0a)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  }
  else if (bread < 0x0a) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }

  *position = (u_int16_t) (status[8]) << 8 | status[7];
  
  /*
   * FIXME: bogus code, should probably be:
   *   *position = njb3_bytes_to_16bit(&status[8]);
   * plus status bits (first 16) need to be checked.
   */

  /*
   * The current track index actually starts at byte 6
   */
  *position = njb3_bytes_to_16bit(&status[6]);
  
  __leave;
  return 0;
}

/*
 * Internal function to figure out if the tune changed.
 * during a call to njb3_elapsed_time. It is assumed that
 * the elapsed time is current and has not get been updated
 * in the protocol state.
 *
 * This is a HACK!
 * We ssume that the elapsed time goes to a lesser number 
 * (zero if called quickly enough) from
 * a high number during a song change.
 * We have to set elapsed_time during a njb3_seek_track
 * just in case.
 * We also have a change if we noticed the current track
 * changed. However, in most cases the current_track is
 * only zero, so we are trying to find the end of the
 * current song that is being played.
 */
static int get_change(njb_t *njb, u_int16_t elapsed, int *change)
{
  __dsub = "get_change";
  
  int chg = 0;
  u_int16_t 	track;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  __enter;
  
  /*
   * You don't want a change at the very first song, because
   * the players seem to listen to this right off, causing them
   * to skip the first tune. So, njb3_play_track calls update_elapsed
   * immediately, which should set it at zero.
   */ 
  if (state->last_elapsed > elapsed) {
    /* printf("changed = 1 due to %d > %d\n",state->last_elapsed, elapsed);
     */
	chg = 1;
  }

  /** update protocol state */
  state->last_elapsed = elapsed;

  /*
   * Check to see if we went to another tune
   * in the queue.
   */
  if (njb3_current_track (njb, &track) == -1) {
    __leave;
    return -1;
  }

  if (track != state->current_playing_track) {
    
    /* printf("change = 1 due to track %d != %d\n",
    		  state->current_playing_track, track);
     */
	state->current_playing_track = track;
    
    chg = 1;
  }
 
  *change  = chg;
  
  __leave;
  return 0;
}


int njb3_elapsed_time(njb_t *njb, u_int16_t * elapsed, int * change)
{
  __dsub= "njb3_elapsed_time";

  u_int16_t etime;
  int       chg = 0;
  
  __enter;

  if (get_elapsed_time(njb, &etime) == -1) {
    __leave;
    return -1;
  }
  
  if (get_change(njb, etime, &chg) == -1 ) {
  	__leave;
	return -1;
  }
  
  *elapsed = etime;
  *change  = chg;

  /* printf("elapsed time %d change %d\n", etime, chg);  
   */
  __leave;
  return 0;
}

int njb3_queue_track(njb_t *njb, u_int32_t trackid)
{
  __dsub= "njb3_queue_track";

  unsigned char njb3_enqueue_track[] = {
    0x01, 0x04, 0x00, 0x01, 0xFF, 0xFF, 0x00, 0x06,
    0x01, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  /* Structure njb3_enqueue_track: 16 bytes
   * 2 bytes unknown 0x0104
   * 2 bytes unknown 0x0001
   * 2 bytes unknown 0xFFFF
   * 2 bytes request code length 0x0006
   * 2 bytes metadata frame name 0x011c
   * 4 bytes track id
   * 2 bytes termination 0x0000
   */
  u_int16_t status;
  __enter;

  from_32bit_to_njb3_bytes(trackid, &njb3_enqueue_track[0x0a]);

  if (send_njb3_command (njb, njb3_enqueue_track, 0x10) == -1) {
    __leave;
    return -1;
  }
  
  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }
  /* We don't interpret the status as of now */

  __leave;
  return 0;
}

int njb3_play_track(njb_t *njb, u_int32_t trackid)
{
  __dsub= "njb3_play_track";

  unsigned char njb3_start_play [0x10] = {
    0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x01, 0x19, 0x00, 0x00, 0x00, 0x00
  };
  /* Structure njb3_start_play: 16 bytes
   * 2 bytes unknown 0x0100
   * 2 bytes unknown 0x0001
   * 4 bytes track id
   * 2 bytes request code length 0x0004
   * 2 bytes metadata frame name 0x0119
   * 2 bytes unknown 0x0000
   * 2 bytes termination 0x0000
   */
  u_int16_t status;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
  __enter;

  from_32bit_to_njb3_bytes(trackid, &njb3_start_play[0x04]);

  if (send_njb3_command (njb, njb3_start_play, 0x10) == -1) {
    __leave;
    return -1;
  }

  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }

  /* Update njb3_current_playing_track. */
  /* The play_track call always puts the track on the front of the playlist. */
  state->current_playing_track = 0;
  
  /* Set the elapsed time on the currently playing track */
  update_elapsed(njb);
  
  __leave;
  return 0;
}

int njb3_seek_track (njb_t * njb, u_int32_t pos)
{
  __dsub= "njb3_seek_track";
  u_int16_t status;
  unsigned char njb3_seek[] = {
    0x00, 0x07, 0x00, 0x01, 0x00, 0x06, 0x01, 0x0c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  /* Structure: 14 bytes
   * 2 bytes command 0x0007 (device control)
   * 2 bytes command 0x0001
   * 2 bytes length of following record 0x0006
   * 2 bytes metadata frame name 0x010c
   * 4 bytes track position to seek to (in ms)
   * 2 bytes termination 0x0000
   */
  __enter;

  from_32bit_to_njb3_bytes(pos, &njb3_seek[8]);

  if (send_njb3_command (njb, njb3_seek, 0x0e) == -1) {
    __leave;
    return -1;
  }

  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }
  
  if ( status != NJB3_STATUS_OK ) {
    printf("LIBNJB Panic: njb3_seek_track() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  
  /* 
   * Set the elapsed time on the currently playing track, since we seeked 
   * to a new position (i.e. elapsed time).
   */
   update_elapsed(njb);
   
  __leave;
  return 0;
}

int njb3_clear_play_queue(njb_t *njb)
{
  __dsub= "njb3_clear_play_queue";
  unsigned char njb3_clear_queue[] = {
    0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0xff, 0xff
  };
  /* Structure: 8 bytes
   * 2 bytes command 0x0103 (clear queue)
   * 2 bytes command 0x0001
   * 2 bytes termination 0x0000
   * 2 bytes termination 0xffff
   */
  u_int16_t status;
  
  __enter;

  if (send_njb3_command (njb, njb3_clear_queue, 0x08) == -1) {
    __leave;
    return -1;
  }

  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }

  if ( status != NJB3_STATUS_EMPTY_CHUNK ) {
    printf("LIBNJB Panic: njb3_clear_play_queue() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }

  __leave;
  return 0;
}

int njb3_ctrl_playing(njb_t *njb, int cmd)
{
  __dsub= "njb3_ctrl_playing";
  ssize_t bread;
  unsigned char njb3_request_playing[] = {
    0x00, 0x08, 0x00, 0x01, 0xFF, 0xFE, 0x00, 0x02,
    0x01, 0x0b, 0x00, 0x00
  };
  /* Structure: 14 bytes
   * 2 bytes command 0x0008 (read device properties)
   * 2 bytes command 0x0001
   * 2 bytes unknown 0xFFFE
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x010b
   * 2 bytes termination 0x0000
   */
  unsigned char njb3_play_ctrl[] = {
    0x00, 0x07, 0x00, 0x01, 0x00, 0x04, 0x01, 0x0b,
    0x0b, 0x00, 0x00, 0x00
  };
  /* Structure: 12 bytes
   * 2 bytes command 0x0007 (device control)
   * 2 bytes unknown 0x0001
   * 2 bytes length  0x0004
   * 2 bytes parameter 0x010b (play control)
   * 1 byte unknown 0x0b
   * 1 byte command (0x00,0x01,0x02,0x03)
   * 2 bytes termination 0x0000
   */
  unsigned char play_status[0x0a];
  u_int16_t status;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  __enter;

  if (send_njb3_command (njb, njb3_request_playing, 0x0c) == -1) {
    __leave;
    return -1;
  }

  if ( (bread = usb_pipe_read(njb, play_status, 0x0a)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  }  
  else if (bread < 0x0a) {
    NJB_ERROR(njb,EO_RDSHORT);
    __leave;
    return -1;
  }
  
  njb3_play_ctrl[9] = cmd;

  /* Don't send redundant commands. */
  switch (cmd) {
  case NJB3_START_PLAY:
    if (play_status[7] != NJB3_STOP_PLAY) {
      __leave;
      return 0;
    }
    break;
  case NJB3_STOP_PLAY:
    state->current_playing_track = 0;
    if (play_status[7] == NJB3_STOP_PLAY) {
      __leave;
      return 0;
    }
    break;
  case NJB3_PAUSE_PLAY:
    if (play_status[7] != NJB3_START_PLAY) {
      __leave;
      return 0;
    }
    break;
  case NJB3_RESUME_PLAY:
    if (play_status[7] != NJB3_PAUSE_PLAY) {
      __leave;
      return 0;
    }
    break;
  }

  if (send_njb3_command (njb, njb3_play_ctrl, 0x0c) == -1) {
    __leave;
    return -1;
  }

  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }

  if ( status != NJB3_STATUS_OK ) {
    printf("LIBNJB Panic: njb3_ctrl_playing() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }

  /*
   * Set the last elapsed time on these operations.
   */
  update_elapsed(njb);
  
  __leave;
  return 0;
}

int njb3_set_time(njb_t *njb, njb_time_t *time)
{
  __dsub= "njb3_set_time";
  unsigned char *data;
  u_int16_t status;
  /* OLD template:
   * unsigned char njb3_set_time_command[]={0x00,0x07,0x00,0x01,0x00,0x0a,0x01,0x10,
   *                                        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
   */

  /* Structure: 18 bytes
   * 2 bytes command 0x0007 (device control)
   * 2 bytes command 0x0001
   * 2 bytes length 0x000a
   * 2 bytes metadata frame for time 0x0110
   * 8 bytes time frame
   * 2 bytes termination 0x0000
   */
  
  __enter;
  
  data = time_pack3(time);
  
  if (send_njb3_command(njb, data, 0x12) == -1){
    free(data);
    __leave;
    return -1;
  }
  free(data);

  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }
  
  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_set_time returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }

  __leave;
  return 0;
}

/**
 * Destroys remaining songid list.
 *
 * The structures retrieved up to njb->next_songid may
 * still be in use by calling application and thus
 * cannot be destroyed.
 */
static void destroy_song_from_njb(njb_t *njb)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  if (state->next_songid != NULL) {
    njb_songid_t *song = state->next_songid;
    while (song != NULL) {
      njb_songid_t *destroysong = song;
      song = song->next;
      NJB_Songid_Destroy(destroysong);
    }
  }
  state->first_songid = NULL;
  state->next_songid = NULL;
}

/**
 * Adds a song to the state holder in the njb_t struct.
 */
static void add_song_to_njb(njb_t *njb, njb_songid_t *song)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  /* Insert this into the list */
  if (state->first_songid == NULL) {
    state->first_songid = song;
    state->next_songid = song;
  } else {
    state->next_songid->next = song;
    state->next_songid = song;
  }
}

/**
 * Routine for reading large metadata chunk transfers as used by
 *
 * - Track listings
 * - Playlist listings
 * - Datafile listings
 *
 * The metadata scan routines supply three functions that are
 * to be called at:
 *
 * - start of metadata post, with a post ID
 * - addition of tags, with a frame ID number and length tag,
 *   as well as a raw data pointer to decode the data
 * - end of metadata post
 * 
 * Additionally each function has a "target" argument that may
 * be dereferenced and used as a storage holder for metadata
 * structures when building them in memory. This structure should
 * be created by the first routine and consumed by the last one,
 * after the middle routine has filled it with information.
 *
 * These functions should return 0 on success and -1 on failure.
 *
 * Here are the function prototypes:
 *
 * int create_metadata_post(postid, target)
 *    u_int32_t postid;
 *    unsigned char **target;
 * {
 *    return 0;
 * }
 *
 * int add_to_metadata_post(frameid, framelen, data, target)
 *    u_int16_t frameid;
 *    u_int16_t framelen;
 *    unsigned char *data;
 *    unsigned char **target;
 * {
 *    return 0;
 * }
 *
 * int terminate_metadata_post(njb, target)
 *    njb_t *njb;
 *    unsigned char **target;
 * {
 *    return 0;
 * }
 */
static int read_metadata_chunk(njb,
			       data, 
			       command_block,
			       command_block_size)
     njb_t *njb;
     unsigned char *data;
     unsigned char *command_block;
     int command_block_size;
{
  __dsub= "read_metadata_chunk";
  int bread = 0;
  int keep_reading_blocks = 1;

  __enter;

  /* Send the metadata retrieveal command */
  if (send_njb3_command(njb, command_block, command_block_size) == -1){
    __leave;
    return -1;
  }
  
  /* Read in all blocks until end marker is found */
  while (keep_reading_blocks == 1) {
    /* End markers for metadata blocks are weird!
     * Typical look:
     *            -e-d -c-b -a-9 -8-7 -6-5 -4-3 -2-1
     * Playlists: 0000 0000 0000 0011 0000 0000 0001
     *            |    |            |              |
     *            |    |         No of playlists   |
     *            |    |                           |
     * Tracks   : 0000 0000 03b8 055f 0001 0000 0000
     *            0000 0000 079c 0b62 0001 0000 0001
     *            |    |       |    |              |
     *            |    |         ???????           |
     *            |    |                           |
     * Files    : 0000 0000 0000 000f 0000 0000 0001
     *            |    |            |              |
     *            |    |         No of files       |
     *            |    |                           |
     * Empty set: 0000 0000 ffff ffff ffff ffff 0001
     *            |    |                           |
     *            Database                         0000 = more frames
     *            Terminator                       0001 = last frame
     *
     * Legend: word1, word2, word3 16bit - we
     *         know all these to be independent
     *         values.
     *
     *         word4 32bit - it seems like a
     *               long 32bit integer
     *
     *         word5 16bit last word.
     */
    int chunklen;
    u_int16_t word1, word2, word3, word5;
    u_int32_t word4;
    u_int32_t chunksize = NJB3_CHUNK_SIZE;

    /* This is a workaround for a bug somewhere deep inside Darwin */    
#ifdef USE_DARWIN
    if (njb_device_is_usb20(njb)) {
      chunksize = 0x1400U;
    }
#endif
    if ( (chunklen = usb_pipe_read(njb, data+bread, chunksize)) == -1 ) {
      NJB_ERROR(njb, EO_USBBLK);
      __leave;
      return -1;
    }
    bread += chunklen;
    if (bread < 16) {
      __leave;
      return bread;
    }
    word1 = njb3_bytes_to_16bit(&data[bread-12]);
    word2 = njb3_bytes_to_16bit(&data[bread-10]);
    word3 = njb3_bytes_to_16bit(&data[bread-8]);
    word4 = njb3_bytes_to_32bit(&data[bread-6]);
    word5 = njb3_bytes_to_16bit(&data[bread-2]);
    /* Break rule, may need some trimming ... 
     * YOU DON'T SAY! :-) This identification hack
     * didn't work at all on Dan Shookowsky's device. Now
     * it is (hopefully) fixed... Current layout suggested
     * by Friso Brugmans. */
    if (word1 == 0 && (word5 == 0 || word5 == 1) && 
	(word4 == 0 || word4 == 0x10000U || word4 == 0xFFFFFFFFU)) {
      if (bread >= 16) {
	/*
	 * If this is an end sequence, and not only 
	 * a 12-byte end sequence on it's own 
	 * (2 bytes status + terminator = 14, and thus this 
	 * has to be atleast 16 bytes to permit for a 0x0000 
	 * terminator before end sequence), it has to 
	 * be preceded by a 0000 end tag. Ugly hack, but
	 * sort of works.
	 */
	u_int16_t precede = njb3_bytes_to_16bit(&data[bread-14]);
	if (precede == 0x0000U) {
	  keep_reading_blocks = 0;
	}
      } else {
	/* The case with a single terminator */
	keep_reading_blocks = 0;
      }
    }
  }
  __leave;
  return bread;
}

static int get_metadata_chunks(njb, 
			       command_block, 
			       command_block_size,
			       create_metadata_post,
			       add_to_metadata_post,
			       terminate_metadata_post)
     njb_t *njb;
     unsigned char *command_block;
     int command_block_size;
     int (*create_metadata_post) ();
     int (*add_to_metadata_post) ();
     int (*terminate_metadata_post) ();
{
  __dsub= "get_metadata_chunks";

  unsigned char *data;
  unsigned char *target;
  int keep_reading_chunks = 1;
  int i=0,j=0;
  u_int16_t status;
  int next_chunk = 1;
  int read_again = 0;
  int started_frame = 0;
  int32_t bread;
  u_int16_t framelen;
  
  __enter;
  
  /* Allocate a metadata scanning buffer */
  if((data = (unsigned char *) malloc(NJB3_CHUNK_SIZE+0x100)) == NULL){
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  
  bread = read_metadata_chunk(njb,
			      data,
			      command_block,
			      command_block_size);
  
  /* Read at least 16 bits of status */
  if (bread < 2) {
    NJB_ERROR(njb, EO_RDSHORT);
    free(data);
    __leave;
    return -1;
  }
  
  /* Check the status word */
  status = njb3_bytes_to_16bit(&data[i]);
  if (status != NJB3_STATUS_OK) {
    free(data);
    /* FIXME: classify more errors */
    if (status == NJB3_STATUS_EMPTY_CHUNK) {
      /* This means "empty metadata/end of transmission */
      __leave;
      return 0;
    }
    /* Everything else interpreted as "error, somekind" */
    printf("LIBNJB Panic: get_metadata_chunks() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  /* This is returned sometimes. We break here,
   * because there is no metadata to read, we 
   * already read everything. */
  framelen = njb3_bytes_to_16bit(&data[2]);
  if (framelen == 0) {
    free(data);
    __leave;
    return 0; /* Should it be -1 ? */
  }
  
  i += 2;
  while(i < bread && keep_reading_chunks == 1){
    /* Now we recognize the frames */
    next_chunk = 0;
    while (!next_chunk) {
      u_int16_t frameid;
      
      /* Length of the next frame */
      framelen = njb3_bytes_to_16bit(&data[i]) + 2;
      
      /* What are the contents of the next frame? */
      frameid = njb3_bytes_to_16bit(&data[i+2]);
      /* printf("Found frame with ID: %04x and Length: %04x\n", frameid, framelen); */
      
      /* Zero length frame and frameid zero means 
	 "new chunk, read again" or "last chunk, stop reading" */
      if ((framelen == 2) && (frameid == 0)) {
	int result;
	next_chunk = 1;
	framelen = 4;
	/* printf("Calling metadata termination routine (1)\n"); */
	if (started_frame == 1) {
	  result = (*terminate_metadata_post) (njb, &target);
	  if (result == -1) {
	    __leave;
	    free(data);
	    return -1;
	  }
	  started_frame = 0;
	}
      }
      /* A zero length terminates the tag */
      else if (framelen == 2) {
	int result;
	/* Call metadata post termination routine
	 * (ends current metadata)
	 */
	/* printf("Calling metadata termination routine (2)\n"); */
	if (started_frame == 1) {
	  result = (*terminate_metadata_post) (njb, &target);
	  if (result == -1) {
	    __leave;
	    free(data);
	    return -1;
	  }
	  started_frame = 0;
	}
      }
      /* Recognize a new post */
      else if (frameid == NJB3_POSTID_FRAME_ID) {
	u_int32_t postid = njb3_bytes_to_32bit(&data[i+4]);
	int result;
	/* Call metadata post creation routine 
	 * (opens a new metadata post)
	 */
	/* printf("Calling metadata creation routine\n"); */
	result = (*create_metadata_post) (postid, &target);
	if (result == -1) {
	  __leave;
	  free(data);
	  return -1;
	}
	started_frame = 1;
      } else {
	unsigned char *data1 = &data[i+4];
	int result;
	/* Call metadata processing routine:
	 * frameid = ID of frame
	 * framelen = Length of frame
	 * data = pointer to data
	 */
	/* printf("Calling metadata adding routine\n"); */
	if (started_frame == 1) {
	  result = (*add_to_metadata_post) (frameid, framelen, data1, &target);
	  if (result == -1) {
	    __leave;
	    free(data);
	    return -1;
	  }
	}
      }
      /* Increase the counter */
      i += framelen;
    }
    /*
     * The last 16 bit number of the chunk 
     * is 0x0000 if we shall read again, and 
     * 0x0001 if the end of reading is reached.
     */
    read_again = njb3_bytes_to_16bit(&data[i+8]);
    
    /* If this is zero, read again */
    if (read_again == 0) {
      for(j = i; j < i+8; j++) {
	command_block[j-i+8]=data[j];
      }
      bread = read_metadata_chunk(njb,
				  data,
				  command_block,
				  command_block_size);
      
      /* Atleast we need status */
      if (bread < 2) {
	free(data);
	NJB_ERROR(njb, EO_RDSHORT);
	__leave;
	return -1;
      }
      /* Check the status word */
      status = njb3_bytes_to_16bit(&data[0]);
      
      /*
       * NJB2 sometimes(?) sends a 0x000e 0x0000 0x01f4 0x001b 0x0001 0x0000 0x0001
       * on end-of-data.
       */
      if (status == NJB3_STATUS_EMPTY_CHUNK) /* Means "end of file" */
	keep_reading_chunks = 0;
      else if (status != NJB3_STATUS_OK) {
	free(data);
	printf("LIBNJB Panic: get_metadata_chunks() returned unknown status code %04x!\n", status);
	NJB_ERROR(njb, EO_BADSTATUS);
	__leave;
	return -1;
      }
      i=2;
    } else {
      /* Should be for 0x0001 only, but we haven's seen
       * anything else.
       *
       * When reading playlists, the 16-bit number at data[i+2] 
       * == number of playlists here.
       */
      if (read_again != 0x0001) {
	printf("LIBNJB: Weird end marker of metadata chunk: %04x\n", read_again);
      }
      keep_reading_chunks = 0;
    }
  }
  free(data);
  
  __leave;
  
  return 0;
}

/**
 * This is a budget version of the previous routine which will not
 * read in blocks, but just parse monolithic blocks in memory.
 * Currently only used by the key parser.
 */
static int parse_metadata_block(njb,
				data,
				size,
				create_metadata_post,
				add_to_metadata_post,
				terminate_metadata_post)
     njb_t *njb;
     unsigned char *data;
     u_int32_t size;
     int (*create_metadata_post) ();
     int (*add_to_metadata_post) ();
     int (*terminate_metadata_post) ();
{
  u_int32_t i = 0;
  unsigned char *target;
  int started_frame = 0;
  
  while(i < size){
    /* Now we recognize the frames */
    u_int16_t framelen;
    u_int16_t frameid;
      
    /* Length of the next frame */
    framelen = njb3_bytes_to_16bit(&data[i]) + 2;

    /* Contents of the next frame */
    if (framelen > 2) {
      frameid = njb3_bytes_to_16bit(&data[i+2]);
      /* printf("Found frame with ID: %04x and Length: %04x\n", frameid, framelen); */
    } else {
      frameid = 0;
    }
    
    if (framelen == 2) {
      int result;
      /* Call metadata post termination routine
       * (ends current metadata)
       */
      if (started_frame == 1) {
	/* printf("Calling metadata termination routine (2)\n"); */
	result = (*terminate_metadata_post) (njb, &target);
	if (result == -1) {
	  /* FIXME: deallocate memory */
	  return -1;
	}
	started_frame = 0;
      }
    }
    /* Recognize a new post */
    else if (frameid == NJB3_POSTID_FRAME_ID) {
      u_int32_t postid = njb3_bytes_to_32bit(&data[i+4]);
      int result;
      /* Call metadata post creation routine 
       * (opens a new metadata post)
       */
      /* printf("Calling metadata creation routine\n"); */
      started_frame = 1;
      result = (*create_metadata_post) (postid, &target);
      if (result == -1) {
	/* FIXME: deallocate memory */
	return -1;
      }
    } else {
      unsigned char *data1 = &data[i+4];
      int result;
      /* Call metadata processing routine:
       * frameid = ID of frame
       * framelen = Length of frame
       * data = pointer to data
       */
      /* printf("Calling metadata adding routine\n"); */
      if (started_frame == 1) {
	result = (*add_to_metadata_post) (frameid, framelen, data1, &target);
	if (result == -1) {
	  /* FIXME: deallocate memory */
	  return -1;
	}
      }
    }
    /* Increase the counter */
    i += framelen;
  }
  return 0;
}



/*
 * Tracklist scanning helper functions - called by the generic metadata
 * scanning routine.
 */
static int create_songid(postid, target)
     u_int32_t postid;
     unsigned char **target;
{
  njb_songid_t *song = NULL;
  /* Create a new song ID */
  song = NJB_Songid_New();
  if (song == NULL) {
    return -1;
  }
  song->trid = postid;
  *target = (unsigned char *) song;
  return 0;
}

static int add_to_songid(frameid, framelen, data, target)
     u_int16_t frameid;
     u_int16_t framelen;
     unsigned char *data;
     unsigned char **target;
{
  njb_songid_t *song = (njb_songid_t *) *target;
  njb_songid_frame_t *frame;
  char *tmp;

  /* 0x01 0x04 = Track name, create a frame with the track name */
  if (frameid == NJB3_TITLE_FRAME_ID) {
    tmp = ucs2tostr(data); // Check for out of mem
    frame = NJB_Songid_Frame_New_Title(tmp);
    free(tmp);
    NJB_Songid_Addframe(song, frame);
  }
  /* Create a frame with the artist name */
  else if (frameid == NJB3_ARTIST_FRAME_ID) {
    tmp = ucs2tostr(data); // Check for out of mem
    frame = NJB_Songid_Frame_New_Artist(tmp);
    free(tmp);
    NJB_Songid_Addframe(song, frame);
  }
  /* Create a frame with the genre name */
  else if (frameid == NJB3_GENRE_FRAME_ID) {
    tmp = ucs2tostr(data); // Check for out of mem
    frame = NJB_Songid_Frame_New_Genre(tmp);
    free(tmp);
    NJB_Songid_Addframe(song, frame);
  }
  /* Create a frame with the album name */
  else if (frameid ==  NJB3_ALBUM_FRAME_ID) {
    tmp = ucs2tostr(data); // Check for out of mem
    frame = NJB_Songid_Frame_New_Album(tmp);
    free(tmp);
    NJB_Songid_Addframe(song, frame);
  }
  /* Create a frame with the track size */
  else if (frameid ==  NJB3_FILESIZE_FRAME_ID) {
    u_int32_t track_size = njb3_bytes_to_32bit(data);
    frame = NJB_Songid_Frame_New_Filesize(track_size);
    NJB_Songid_Addframe(song, frame);
  }
  else if (frameid ==  NJB3_LOCKED_FRAME_ID) {
    /*
     * It seems the value 0x0100 means "locked", 0x0000 means "open"
     * printf("Locked Track? Value %02x%02x\n", data[i+4], data[i+5]);
     *
     */
    if (data[0] == 0x01 && data[1] == 0x00) {
      frame = NJB_Songid_Frame_New_Protected(0x0001U);
      NJB_Songid_Addframe(song, frame);
    }
  }
  /* Create a frame with codec type */
  else if (frameid ==  NJB3_CODEC_FRAME_ID) {
    u_int16_t codec;
    
    codec = njb3_bytes_to_16bit(data);
    if (codec == NJB3_CODEC_MP3_ID || codec == NJB3_CODEC_MP3_ID_OLD) {
      frame = NJB_Songid_Frame_New_Codec(NJB_CODEC_MP3);
      NJB_Songid_Addframe(song, frame);
    } else if (codec == NJB3_CODEC_WAV_ID) {
      frame = NJB_Songid_Frame_New_Codec(NJB_CODEC_WAV);
      NJB_Songid_Addframe(song, frame);
    } else if (codec == NJB3_CODEC_WMA_ID || codec == NJB3_CODEC_PROTECTED_WMA_ID) {
      frame = NJB_Songid_Frame_New_Codec(NJB_CODEC_WMA);
      NJB_Songid_Addframe(song, frame);
    } else if (codec == NJB3_CODEC_AA_ID) {
      frame = NJB_Songid_Frame_New_Codec(NJB_CODEC_AA);
      NJB_Songid_Addframe(song, frame);
    } else {
      /* Ogg Vorbis? :-) */
      printf("LIBNJB panic: unknown codec ID %04x\n", codec);
    }
  }
  /* Create a frame with the track year */
  else if (frameid == NJB3_YEAR_FRAME_ID) {
    u_int16_t track_year = njb3_bytes_to_16bit(data);
    frame = NJB_Songid_Frame_New_Year(track_year);
    NJB_Songid_Addframe(song, frame);
  }
  /* Create a frame with the track number */
  else if (frameid == NJB3_TRACKNO_FRAME_ID) {
    u_int16_t track_number = njb3_bytes_to_16bit(data);
    frame = NJB_Songid_Frame_New_Tracknum(track_number);
    NJB_Songid_Addframe(song, frame);
  }
  /* Create a frame with the track runlengh (time) */
  else if (frameid == NJB3_LENGTH_FRAME_ID) {
    u_int16_t tracktime = njb3_bytes_to_16bit(data);
    frame = NJB_Songid_Frame_New_Length(tracktime);
    NJB_Songid_Addframe(song, frame);
  }
  /* Create a frame with the original filename */
  else if (frameid == NJB3_FNAME_FRAME_ID) {
    tmp = ucs2tostr(data); // Check for out of mem
    frame = NJB_Songid_Frame_New_Filename(tmp);
    free(tmp);
    NJB_Songid_Addframe(song, frame);
  }
  /* Ignore data about the track directory */
  else if (frameid == NJB3_DIR_FRAME_ID) {
    tmp = ucs2tostr(data); // Check for out of mem
    /* Currently we just ignore this string, could define a new FR_DIR or so */
    frame = NJB_Songid_Frame_New_Folder(tmp);
    free(tmp);
    NJB_Songid_Addframe(song, frame);
  }
  /* Unknown frame */
  else {
    /* 
       printf("Unknown frame %02x%02x, length %02x%02x bytes\n",
       data[i+2],data[i+3],data[i],data[i+1]);
    */
  }
  return 0;
}

static int terminate_songid(njb, target)
     njb_t *njb;
     unsigned char **target;
{
  njb_songid_t *song = (njb_songid_t *) *target;
  add_song_to_njb(njb, song);
  song = NULL;
  return 0;
}

/* 
 * This routine not only gets the first track,
 * but makes a list of *ALL* tracks.  next_track_tag is a dummy
 * that follows this list.
 */
int njb3_reset_get_track_tag (njb_t *njb)
{
  __dsub= "njb3_reset_get_track_tag";
  
  /*
   * This version will also retrieve filename and folder name
   * metadata for the tracks. This takes a LONG time for the 
   * firmware and should not be used.
   *
   */
  unsigned char njb3_get_track_tags[]=
    {0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x02,
     0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
     0x00,0x00,0x01,0x00,0xff,0xfe,0x00,0x14,
     0x01,0x04,0x01,0x02,0x01,0x03,0x01,0x01,
     0x00,0x0e,0x00,0x0b,0x00,0x06,0x01,0x07,
     0x01,0x06,0x01,0x05,0x00,0x00,0x00,0x00};
  
  unsigned char njb3_get_track_tags_extended[]=
    {0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x02,
     0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
     0x00,0x00,0x01,0x00,0xff,0xfe,0x00,0x18,
     0x01,0x04,0x01,0x02,0x01,0x03,0x01,0x01,
     0x00,0x0e,0x00,0x0b,0x00,0x06,0x01,0x07,
     0x01,0x06,0x01,0x05,0x00,0x0d,0x00,0x07,
     0x00,0x00,0x00,0x00};
  
  /* Structure: 0x30 bytes
   * 4 bytes command 0x00060001U - read from database
   * 4 bytes database ID 0x00000002U - track database
   * 8 bytes previous data, initially 0xff * 8
   * 2 bytes unknown 0x0000
   * 2 bytes number of tags to request 0x0100 = 256 (I think.)
   * 2 bytes unknown 0xfffe (buffer size? 64KB - 2 bytes for status)
   *
   * 2 bytes length of metadata wishlist (which follows 0x14 originally)
   *
   * The following is then a list of metadata to be returned
   *
   * 2 bytes 0x0104 = return Title metadata
   * 2 bytes 0x0102 = return Artist metadata
   * 2 bytes 0x0103 = return Genre metadata
   * 2 bytes 0x0101 = return Album metadata
   * 2 bytes 0x000e = return File size metadata
   * 2 bytes 0x000b = return Locked status metadata
   * 2 bytes 0x0006 = return Codec metadata
   * 2 bytes 0x0107 = return Year metadata
   * 2 bytes 0x0106 = return Track number metadata
   * 2 bytes 0x0105 = return Runlength metadata
   * OPTIONALLY:
   *   2 bytes 0x000d = return folder metadata (not requested by all apps)
   *   2 bytes 0x0007 = return Filename metadata (not requested by all apps)
   *
   * 2 bytes unknown 0x0000
   * 2 bytes unknown 0x0000
   */
  int result;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
  unsigned char *command;
  int commandlen;
  
  __enter;
  
  /* Clean from any previous scan */
  destroy_song_from_njb(njb);
  
  /* Use the generic metadata scan function parametrized
   * with three metadata processing functions 
   */
  if (state->get_extended_tag_info != 0) {
    command = njb3_get_track_tags_extended;
    commandlen = 0x34;
  } else {
    command = njb3_get_track_tags;
    commandlen = 0x30;
  }
  result = get_metadata_chunks(njb,
			       command,
			       commandlen,
			       create_songid,
			       add_to_songid,
			       terminate_songid);
  
  if (result == -1) {
    state->next_songid = NULL;
    state->first_songid = NULL;
    __leave;
    return -1;
  }
  
  /* Point to first song (also if it is NULL) */
  state->next_songid = state->first_songid;
  
  __leave;
  return 0;
}


njb_songid_t *njb3_get_next_track_tag (njb_t *njb)
{
  __dsub= "njb3_get_next_track_tag";
  /* Returns a songID from the list */
  njb_songid_t *tmp;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  __enter;
  if (state->next_songid == NULL) {
    state->first_songid = NULL;
    /* Ignore this "error" for now: NJB_ERROR(njb, EO_EOM); */
    __leave;
    return NULL;
  }
  tmp = state->next_songid;
  state->next_songid = tmp->next;
  __leave;
  return tmp;
}

/**
 * This routine is called by the playlist list retrieval
 * function to fill in each playlistlist with it's tracks.
 *
 * @param njb the NJB object to use.
 * @param pl the playlist to fill in.
 * @return 0 if filled in OK, other value means failure.
 */
static int populate_playlist(njb_t *njb, njb_playlist_t *pl)
{
  __dsub= "populate_playlist";
  unsigned char njb3_get_tracks_for_pl[]={0x01,0x08,0x00,0x01,0x00,0x00,0x00,0x00,
					  0x00,0x00,0x01,0x00,0xff,0xfe,0x00,0x02,
					  0x00,0x0c,0x00,0x00};
  /* Structure: 0x14 bytes
   * 2 bytes unknown 0x0108
   * 2 bytes unknown 0x0001
   * 4 bytes entity ID
   * 2 bytes unknown 0x0000  - termination?
   * 2 bytes unknown 0x0100  -  This could be max transfer length excl 2 status bytes
   * 2 bytes unknown 0xfffe  -  = 0100fffe + 2 = 01010000 or fffe+2 = 00010000
   * 2 bytes request code length 0x0002
   * 2 bytes metadata frame name 0x000c (POSTID)
   * 2 bytes termination 0x0000
   */
  unsigned char *data;
  int keep_reading_chunks=1;
  int next_chunk = 1;
  int read_again = 0;
  int i=0;
  int metastarted = 0;
  u_int16_t status;
  u_int32_t framelen;
  u_int32_t frameid;
  int32_t trackid = 0; /* Obviously signed here */
  ssize_t bread;
  njb_playlist_track_t *track;
  
  __enter;
  
  from_32bit_to_njb3_bytes(pl->plid, &njb3_get_tracks_for_pl[4]);
  
  /* Allocate a playlist tracklist buffer */
  data = (unsigned char *) malloc(NJB3_CHUNK_SIZE);
  if (data ==NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  
  /* Scan the playlist for tracks */
  if (send_njb3_command(njb, njb3_get_tracks_for_pl, 0x14) == -1){
    __leave;
    return -1;
  }
  if ( (bread = usb_pipe_read(njb, data, NJB3_CHUNK_SIZE)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  }
  
  /* Check the status word */
  status = njb3_bytes_to_16bit(&data[i]);
  /*
    I don't know what this word means, it's usually 0x000e
    doesn't seem to be status, not the number of tracks
    in the list either.
    Could be:
    0x000e = Playlist tracks follow, only one chunk.
    Could mean simply: "last chunk". When
    multiple chunks are read, 0000 comes in
    instead.
    0x0000 = OK chunks follow.
    0x0001 = Empty data? Error?
  */
  if (status == NJB3_STATUS_EMPTY) {
    /* No idea what this is, probably just means empty. */
    __leave;
    return 0;
  }
  
  /*
   * This makes us leave the party if there are no songs in this
   * playlist
   *
   * The following trace corrupted a lot of jukeboxen:
   *
   * send >>
   * 0000: 0108 0001 000d 65b1 0000 0100 fffe 0002   ......e.........
   * 0010: 000c 0000                                 ....
   *
   * read <<
   * 0000: 000e 0000 0001                            ......
   * 
   * Whereas a normal zero-length playlist should return
   *
   * read <<
   * 0000: 0000 0000 0001
   *
   * The (bread < 8) below will avoid trying to read in such
   * corrupted playlists. These appear when all tracks that 
   * are located in a playlist are removed so none of them
   * remain.
   */
  if (bread < 8) {
    __leave;
    /*
     * Corrupt or just empty?
     *
     * printf("LIBNJB Warning: corrupt empty playlist 
     * with ID %08x found, please delete it.\n", pl->plid); 
     */
    return 0;
  }
  
  i = 2;
  while((i < bread) && (keep_reading_chunks == 1)){
	  /* Now we recognize the frames */
    next_chunk = 0;
    while (!next_chunk) {
      /* Length of the next frame */
      framelen = njb3_bytes_to_16bit(&data[i]) + 2;
      
      if (i+framelen >= bread) {
	/* How many bytes to save (the current frame) */
	int saveme = bread-i;
	unsigned char *tmpbuffer;
	
	/* Move current frame into the first bytes of the buffer.
	 * If the buffer size is very small (if the jukebox
	 * sends small pieces of metadata), this can be an overlapping
	 * operation. Thus we copy to a temporary buffer and back. */
	tmpbuffer = (unsigned char *) malloc(saveme);
	memcpy (tmpbuffer, data+i, saveme);
	memcpy (data, tmpbuffer, saveme);
	free(tmpbuffer);
	
	/* Push new data from jukebox into the buffer right behind
	 * the current frame. */
	if ( (bread = usb_pipe_read(njb, data+saveme, NJB3_CHUNK_SIZE-saveme)) == -1 ) {
	  NJB_ERROR(njb, EO_USBBLK);
	  __leave;
	  return -1;
	}
	/* bread is set to the whole buffer size, that means it must be
	 * increased by the size of the current frame. */
	bread += saveme;
	
	/* Reset the index. */
	i = 0;
      } /* </Reread> Continue as if nothing happens. */
      
      
      /* What are the contents of the next frame */
      frameid = njb3_bytes_to_16bit(&data[i+2]);

      /* Zero length frame and framid zero means "read again" */
      if ((framelen == 2) && (frameid == 0x0000)) {
	if (metastarted == 1 && trackid > 0) {
	  track = NJB_Playlist_Track_New(trackid);
	  NJB_Playlist_Addtrack(pl, track, NJB_PL_END);
	} /* Else ignore it */
	next_chunk = 1;
	if (data[i+4] == 0x00 && data[i+5] == 0x01)
	  read_again = 0;
	else
	  read_again++;
      }
      /* A zero length terminates the tag */
      else if (framelen == 2) {
	if (metastarted == 1 && trackid > 0) {
	  track = NJB_Playlist_Track_New(trackid);
	  NJB_Playlist_Addtrack(pl, track, NJB_PL_END);
	  metastarted = 0;
	} /* Else ignore it */
      }
      /* Recognize track ID */
      else if (frameid == NJB3_POSTID_FRAME_ID) {
	/* We detected the start of a track tag */
	/* Then create a new playlist ID */
	metastarted = 1;
	/* Sometimes negative ID:s appear here. They are invalid entries. */
	trackid = (int32_t) njb3_bytes_to_32bit(&data[i+4]);
      }
      /* Unknown frame */
      else {
	printf("LIBNJB Panic: Unknown playlist frame %02x%02x, length %02x%02x bytes\n",
	       data[i+2],data[i+3],data[i],data[i+1]);
      }
      /* Increase the counter */
      i += framelen;
    }
    if (read_again > 0) {
      njb3_get_tracks_for_pl[8] = read_again & 0xFF;
      if (send_njb3_command (njb, njb3_get_tracks_for_pl, 0x14) == -1) {
	__leave;
	return -1;
      }
      if ((bread = usb_pipe_read (njb, data, NJB3_CHUNK_SIZE)) == -1) {
	NJB_ERROR(njb, EO_USBBLK);
	__leave;
	return -1;
      }
      
      /* Check the status word */
      status = njb3_bytes_to_16bit(&data[0]);
      if (status == NJB3_STATUS_EMPTY) {
	/* Probably means we should stop reading */
	keep_reading_chunks = 0;
      }
      i=2;
    }
    else {
      keep_reading_chunks = 0;
    }
  }
  free(data);
  /* The playlist should initially be unchanged. */
  pl->_state = NJB_PL_UNCHANGED;
  
  __leave;
  return 0;
}

/**
 * Destroys remaining playlist linked list.
 *
 * The structures retrieved up to njb->next_plid may
 * still be in use by calling application and thus
 * cannot be destroyed.
 */
static void destroy_pl_from_njb(njb_t *njb)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  if (state->next_plid != NULL) {
    njb_playlist_t *pl = state->next_plid;
    while (pl != NULL) {
      njb_playlist_t *destroypl = pl;
      pl = pl->nextpl;
      NJB_Playlist_Destroy(destroypl);
    }
  }
  state->first_plid = NULL;
  state->next_plid = NULL;
}

/**
 * This function adds a playlist struct to the njb struct holder
 */
static int add_pl_to_njb(njb_t *njb, njb_playlist_t *pl)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  /* Populate the playlist with tracks */
  pl->_state = NJB_PL_UNCHANGED;
  /* Insert this into the list */
  if (state->first_plid == NULL) {
    state->first_plid = pl;
    state->next_plid = pl;
  } else {
    state->next_plid->nextpl = pl;
    state->next_plid = pl;
  }
  return 0;
}

/**
 * Playlist scanning helper functions - called by the generic metadata
 * scanning routine.
 */
static int create_playlistid(postid, target)
     u_int32_t postid;
     unsigned char **target;
{
  /* Then create a new playlist ID */
  njb_playlist_t *pl = NJB_Playlist_New();

  if (pl == NULL) {
    return -1;
  }
  pl->plid = postid;
  pl->ntracks = 0;
  *target = (unsigned char *) pl;
  return 0;
}

static int add_to_playlistid(frameid, framelen, data, target)
     u_int16_t frameid;
     u_int16_t framelen;
     unsigned char *data;
     unsigned char **target;
{
  njb_playlist_t *pl = (njb_playlist_t *) *target;
  char *tmp;
  /* Create a frame with the playlist name */
  if (frameid == NJB3_PLNAME_FRAME_ID) {
    tmp = ucs2tostr(data);
    pl->name = tmp;
  }
  /* Unknown frame */
  else {
    /*
      printf("Unknown playlist frame %02x%02x, length %02x%02x bytes\n",
      data[i+2],data[i+3],data[i],data[i+1]);
    */
  }

  return 0;
}

static int terminate_playlistid(njb, target)
     njb_t *njb;
     unsigned char **target;
{
  njb_playlist_t *pl = (njb_playlist_t *) *target;
  pl->_state = NJB_PL_UNCHANGED;
  if (add_pl_to_njb(njb, pl) == -1) {
    return -1;
  }
  pl = NULL;
  return 0;
}

/**
 * This routine retrieves the list of playlists, while also filling in
 * each playlist post with it's respective track ID:s.
 */
int njb3_reset_get_playlist_tag (njb_t *njb)
{
	__dsub= "njb3_get_first_playlist_tag";
	unsigned char njb3_get_playlists[]={0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x01,
					    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
					    0x00,0x00,0x01,0x00,0xff,0xfe,0x00,0x02,
					    0x01,0x0f,0x00,0x00,0x00,0x00};
	/* Structure: 0x1E bytes
       	 * 4 bytes command 0x00060001U - read from database
       	 * 4 bytes database ID 0x00000001U - playlist database
	 * 8 bytes previous data, initially 0xff * 8
	 * 2 bytes unknown 0x0000
	 * 2 bytes maximum number of lists to request 0x0100 = 256 (I think.)
	 * 2 bytes unknown 0xfffe
	 *
	 * 2 bytes length of playlist metadata wishlist
	 *
	 * 2 bytes 0x010f = return Playlist names
	 *
	 * 2 bytes unknown 0x0000
	 * 2 bytes unknown 0x0000
       	 */
	int result;
	njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

	__enter;

	/* Clean from any previous scan */
	destroy_pl_from_njb(njb);

	/* Use the generic metadata scan function parametrized
	 * with three metadata processing functions 
	 */
	result = get_metadata_chunks(njb,
				     njb3_get_playlists,
				     0x1e,
				     create_playlistid,
				     add_to_playlistid,
				     terminate_playlistid);

	if (result == -1) {
	  state->next_plid = NULL;
	  state->first_plid = NULL;
	  __leave;
	  return result;
	}

	if (state->first_plid != NULL) {
	  /* Populate the tags */
	  njb_playlist_t *pl = state->first_plid;
	  
	  while (pl != NULL) {
	    if (populate_playlist(njb, pl) == -1) {
	      __leave;
	      return -1;
	    }
	    pl = pl->nextpl;
	  }
	  state->next_plid = state->first_plid->nextpl;
	}

	/* Point to first playlist (also if NULL) */
	state->next_plid = state->first_plid;
	
	__leave;

	return 0;
}

njb_playlist_t *njb3_get_next_playlist_tag (njb_t *njb)
{
  /* Returns a playlistID from the list */
  njb_playlist_t *tmp;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  if (state->next_plid == NULL) {
    state->first_plid = NULL;
    return NULL;
  }
  tmp = state->next_plid;
  state->next_plid = tmp->nextpl;
  return tmp;
}

/**
 * Frees remaining datafile linked list.
 *
 * The structures retrieved up to njb->next_dfid may
 * still be in use by calling application and thus
 * cannot be destroyed.
 */
static void destroy_df_from_njb(njb_t *njb)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  if (state->next_dfid != NULL) {
    njb_datafile_t *df = state->next_dfid;
    while (df != NULL) {
      njb_datafile_t *destroydf = df;
      df = df->nextdf;
      NJB_Datafile_Destroy(destroydf);
    }
  }
  state->first_dfid = NULL;
  state->next_dfid = NULL;
}

/**
 * This function adds a datafile struct to the njb struct holder
 */
static int add_df_to_njb(njb_t *njb, njb_datafile_t *df)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  /* Insert this into the list */
  if (state->first_dfid == NULL) {
    state->first_dfid = df;
    state->next_dfid = df;
  } else {
    state->next_dfid->nextdf = df;
    state->next_dfid = df;
  }
  return 0;
}

/**
 * Datafile scanning helper functions - called by the generic metadata
 * scanning routine.
 */
static int create_datafile(postid, target)
     u_int32_t postid;
     unsigned char **target;
{
  /* Then create a new datafile ID */
  njb_datafile_t *df = datafile_new();
  if (df == NULL) {
    return -1;
  }
  df->dfid = postid;
  *target = (unsigned char *) df;
  return 0;
}


static int add_to_datafile(frameid, framelen, data, target)
     u_int16_t frameid;
     u_int16_t framelen;
     unsigned char *data;
     unsigned char **target;
{
  njb_datafile_t *df = (njb_datafile_t *) *target;
  char *tmp;

  if (frameid == NJB3_FNAME_FRAME_ID) {
    tmp = ucs2tostr(data);
    df->filename = tmp;
  }
  else if(frameid == NJB3_FILESIZE_FRAME_ID) {
    u_int32_t tmpsize = njb3_bytes_to_32bit(data);
    df->filesize = (u_int64_t) tmpsize;
  }
  /* Ignore data about the track directory */
  else if (frameid == NJB3_DIR_FRAME_ID) {
    tmp = ucs2tostr(data);
    df->folder = tmp;
  }
  else if (frameid ==  NJB3_LOCKED_FRAME_ID) {
    /*
     * It seems the value 0x0100 means "locked", 0x0000 means "open"
     * printf("Locked File? Value %02x%02x\n", data[i+4], data[i+5]);
     * don't know how to handle protected files yet, do they actually
     * occur? Seems only NJB3 and NJB Zen (first version) i.e. the
     * USB 1.1 devices would ever add this flag to a file.
     */
  }
  else if (frameid == NJB3_FILEFLAGS_FRAME_ID) {
    u_int32_t tmpflags = njb3_bytes_to_32bit(data);
    df->flags = tmpflags;
  }
  /* Unknown or ignored frame */
  else {
    /* 
       printf("Unknown datafile frame %02x%02x, length %02x%02x bytes\n",
       data[i+2],data[i+3],data[i],data[i+1]);
    */
  }
  return 0;
}

static int terminate_datafile(njb, target)
     njb_t *njb;
     unsigned char **target;
{
  njb_datafile_t *df = (njb_datafile_t *) *target;
  if (add_df_to_njb(njb, df) == -1) {
    return -1;
  }
  df = NULL;
  return 0;
}

/**
 * This routine retrieves the list of datafiles, and returns
 * the first item.
 */
int njb3_reset_get_datafile_tag (njb_t *njb) {
	__dsub= "njb3_get_first_datafile_tag";
	/* Extended to retrieve file flags metadata 2004-10-14 */
	unsigned char njb3_get_datafiles[]={0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x00,
					    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
					    0x00,0x00,0x02,0x00,0xff,0xfe,0x00,0x0c,
					    0x00,0x07,0x00,0x0e,0x00,0x0d,0x00,0x16,
					    0x00,0x18,0x00,0x06,0x00,0x00,0x00,0x00};
	/* Structure: 0x26 bytes
       	 * 4 bytes command 0x00060001U - read from database
       	 * 4 bytes database Ox00000000U - file database
	 * 8 bytes previous data, initially 0xff * 8
	 * 2 bytes terminator 0x0000
	 * 2 bytes maximum number of files to request 0x0200 = 512 (I think.)
	 * 2 bytes unknown 0xfffe
	 *
	 * 2 bytes length of datafile metadata wishlist
	 *
	 * 2 bytes 0x0007 = return file names
	 * 2 bytes 0x000e = return file size
	 * 2 bytes 0x000d = return directory name
	 * 2 bytes 0x0016 = return ... something (unknown, permissions?)
	 * 2 bytes 0x0018 = return file flags
	 * 2 bytes 0x0006 = return locked status
	 *
	 * 2 bytes terminate list 0x0000
	 * 2 bytes terminate command 0x0000
       	 */
	int result;
	njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

	__enter;

	/* Clean from any previous scan */
	destroy_df_from_njb(njb);

	result = get_metadata_chunks(njb,
				     njb3_get_datafiles,
				     sizeof(njb3_get_datafiles),
				     create_datafile,
				     add_to_datafile,
				     terminate_datafile);

	if (result == -1) {
	  state->first_dfid = NULL;
	  state->next_dfid = NULL;
	  __leave;
	  return -1;
	}

	/* Point to first file (also if NULL) */
	state->next_dfid = state->first_dfid;

	__leave;

	return 0;
}

njb_datafile_t *njb3_get_next_datafile_tag (njb_t *njb) {
  /* Returns a datafile tag from the list */
  njb_datafile_t *tmp;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  if (state->next_dfid == NULL) {
    state->first_dfid = NULL;
    return NULL;
  }
  tmp = state->next_dfid;
  state->next_dfid = tmp->nextdf;
  return tmp;

}


/**
 * These routines retrieves a mysterious combination of key/value pairs
 * that is retrieved by the software at startup and before sending
 * tracks or files. The "AR00" key is used for a confirmation command
 * after each track/file, for what we don't know. (See the "HACKING"
 * file in the libnjb dir.)
 * 
 * The structure is retrieved and parsed like any other large metadata 
 * block.
 *
 * These keys are currently read in, but not used for anything.
 */

static int create_key(postid, target)
     u_int32_t postid;
     unsigned char **target;
{
  /* Then create a new key */
  njb_keyval_t *keyval;
  keyval = (njb_keyval_t *) malloc(sizeof(njb_keyval_t));
  keyval->value1 = 0;
  keyval->value2 = 0;
  keyval->next = NULL;
  *target = (unsigned char *) keyval;
  return 0;
}

static int add_to_key(frameid, framelen, data, target)
     u_int16_t frameid;
     u_int16_t framelen;
     unsigned char *data;
     unsigned char **target;
{
  njb_keyval_t *keyval = (njb_keyval_t *) *target;

  if (frameid == NJB3_KEY_FRAME_ID) {
    memcpy(keyval->key, data, 4);
    keyval->key[4] = '\0';
  } else if (frameid == NJB3_VALUE_FRAME_ID) {
    keyval->value1 = njb3_bytes_to_32bit(&data[0]);
    keyval->value2 = njb3_bytes_to_32bit(&data[4]);
  } else if (frameid == NJB3_JUKEBOXID_FRAME_ID) {
    memcpy(keyval->deviceid, &data[0], 16);
  }
  return 0;
}

static int terminate_key(njb, target)
     njb_t *njb;
     unsigned char **target;
{
  njb_keyval_t *key = (njb_keyval_t *) *target;
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
  /* Add key to state */
  if (state->first_key == NULL) {
    state->first_key = key;
    state->next_key = key;
  } else {
    state->next_key->next = key;
    state->next_key = key;
  }
  return 0;
}

/* Returns keys to outer call function */
njb_keyval_t *njb3_get_keys(njb_t *njb)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
  return state->first_key;
}

int njb3_read_keys(njb_t *njb)
{
  __dsub= "njb3_read_keys";
  unsigned char njb3_read_keys[]={0x00,0x0c,0x00,0x01,0x00,0x00,0x00,0x0a,
				  0x14,0x00,0x00,0x06,0x00,0x0a,0x00,0x14,
				  0x00,0x15,0x00,0x00};
  /* Structure: 20 bytes
   * 2 bytes command 0x000c - read keys
   * 2 bytes command 0x0001
   * 2 bytes unknown 0x0000
   * 2 bytes length of request 0x000a
   * 2 bytes 0x1400 subrequest
   * 2 bytes 0x0006 length
   * 2 bytes 0x000a request key name
   * 2 bytes 0x0014 request key value
   * 2 bytes 0x0015 request jukebox ID for each key
   * 2 bytes 0x0000 terminator
   */
  unsigned char *data;
  u_int32_t bread;
  u_int16_t status;
  int result;
  
  __enter;
  
  if((data = (unsigned char *) malloc(NJB3_CHUNK_SIZE)) == NULL){
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  
  if (send_njb3_command(njb, njb3_read_keys, 20) == -1){
    free(data);
    __leave;
    return -1;
  }
  if ( (bread = usb_pipe_read(njb, data, NJB3_CHUNK_SIZE)) == -1 ) {
    free(data);
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  }
  
  status = njb3_bytes_to_16bit(&data[0]);
  if (status != NJB3_STATUS_OK) {
    free(data);
    printf("LIBNJB Panic: njb3_read_keys returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  
  /* Call generic metadata parser, index to first frame */
  result = parse_metadata_block(njb,
				&data[2],
				bread,
				create_key,
				add_to_key,
				terminate_key);
  
  free(data);
  
  __leave;
  return 0;
}


/**
 * This function requests a chunk from a certain file. The offset may
 * index into the file. The chunk transfer size is 1MB by default (as
 * used by creative) but may actually exceed that.
 *
 * Returns actual chunk size or -1 if failed.
 */
int njb3_request_file_chunk(njb_t *njb, u_int32_t fileid, u_int32_t offset)
{
  __dsub= "njb3_request_file_chunk";
  unsigned char njb3_request_file[]={0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
				     0x00,0x00,0x00,0x10,0x00,0x00};
  unsigned char status_data[]={0x00,0x00,0x00,0x00,0x00,0x00};  
  u_int32_t chunk_size;
  u_int16_t status;
  int bread;
  /* Structure: 16 bytes
   * 2 bytes command 0x0002 - read file or track
   * 2 bytes command 0x0001
   * 4 bytes item ID
   * 4 bytes offset 0x0000 0x0000  -> start at 0x0000 0x0000 of the file
   * 4 bytes transfer chunk maxlength
   *
   * Originally maxlength was set to 0x0010 0x0000 but setting it to
   * big values swallows a lot more files :-)
   */

  __enter;

  /* File ID */
  from_32bit_to_njb3_bytes(fileid, &njb3_request_file[4]);
  /* Offset into file */
  from_32bit_to_njb3_bytes(offset, &njb3_request_file[8]);
  /* Chunk length */
  from_32bit_to_njb3_bytes(NJB3_CHUNK_SIZE, &njb3_request_file[12]);
  
  
  if (send_njb3_command(njb, njb3_request_file, 0x10) == -1){
    __leave;
    return -1;
  }

  bread= usb_pipe_read(njb, status_data, 6);

  if (bread == -1) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;    
  }
  /* 
   * The smallest that is returned is a
   * 16-bit status code 
   */
  else if ( bread < 2 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }

  status = njb3_bytes_to_16bit(&status_data[0]);
  /*
   * ... more statuses probably exist, we just need to investigate them ...
   */
  if (status != NJB3_STATUS_OK){
    if (status == NJB3_STATUS_TRANSFER_ERROR) {
      printf("LIBNJB Panic: error during transfer!\n");
      NJB_ERROR(njb, NJB_ERR_TRACK_NOT_FOUND);
      __leave;
      return -1;
    }
    else if (status == NJB3_STATUS_NOTEXIST) {
      printf("LIBNJB Panic: track does not exist!\n");
      NJB_ERROR(njb, NJB_ERR_TRACK_NOT_FOUND);
      __leave;
      return -1;
    }
    else if (status == NJB3_STATUS_PROTECTED) {
      printf("LIBNJB Panic: tried to access protected track!\n");
      NJB_ERROR(njb, NJB_ERR_UPLOAD_DENIED);
      __leave;
      return -1;
    } 
    /* End-of-file */
    else if (status == NJB3_STATUS_EMPTY_CHUNK) {
      __leave;
      return 0;
    }
    /* Default error */
    printf("LIBNJB Panic: unknown status code in njb3_request_file_chunk(): %04x\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  chunk_size = njb3_bytes_to_32bit(&status_data[2]);

  /* If everything was OK we will start retrieveing this chunk... */
  __leave;
  return chunk_size;
}

/**
 * This function retrieves a part of the requested chunk. 
 * Short reads are allowed, so the caller must make sure that it 
 * is called as many times as is needed to retrieve the entire
 * file chunk.
 *
 * @param data    an allocated byte array to store the retrieved
 *                chunk in.
 * @param maxsize the maximum number of bytes to retrieve from
 *                this chunk at a time.
 */
int njb3_get_file_block(njb_t *njb, unsigned char *data, u_int32_t maxsize)
{
	__dsub= "njb3_get_file_block";
	int bread;

	__enter;

	/* Read the block from USB */
	bread = usb_pipe_read(njb, data, maxsize);
       	if ( bread == -1 ) {
       		NJB_ERROR(njb, EO_USBBLK);
       		__leave;
       		return -1;
       	}

	__leave;
	return bread;
}

u_int32_t njb3_create_file(njb_t *njb, unsigned char *ptag, u_int32_t tagsize, u_int16_t database)
{
	__dsub= "njb3_create_file";
       	unsigned char njb3_create_file[]={0x00,0x04,0x00,0x01,0x00,0x00,0x00,0x00};
	u_int32_t bread;
	u_int16_t status;
	u_int32_t fileid;
	unsigned char *data;
	unsigned char status_data[]={0x00,0x00,0x00,0x00,0x00,0x00};
	/* Structure: 8 bytes + n tags + 0x0000 terminator
       	 * 2 bytes command 0x0004 - create file or track
       	 * 2 bytes command 0x0001
       	 * 2 bytes unknown 0x0000
       	 * 2 bytes database 0x0002 for tracks 0x0000 for datafiles (0x0001 = playlist)
	 * ...after this the tags follow...
	 * 2 bytes terminator 0x0000
       	 */
	__enter;

	/* Command size plus tag size, plus terminator two bytes */
	data = malloc(8+tagsize+2);
	if (data == NULL) {
		NJB_ERROR(njb, EO_NOMEM);
		__leave;
		return 0;
	}
	memcpy(&data[0], njb3_create_file, 8);
	/* Set file type */
	from_16bit_to_njb3_bytes(database, &data[6]);
	/* Copy tag to metadata chunk */
	memcpy(&data[8], ptag, tagsize);
	from_16bit_to_njb3_bytes(0x0000U, &data[8+tagsize]);
       	if (send_njb3_command(njb, data, 8+tagsize+2) == -1){
		free(data);
       		__leave;
       		return 0;
       	}
	free(data);
       	if ( (bread= usb_pipe_read(njb, status_data, 6)) == -1 ) {
       		NJB_ERROR(njb, EO_USBBLK);
       		__leave;
       		return 0;
       	} else if ( bread < 2 ) {
       		NJB_ERROR(njb, EO_RDSHORT);
       		__leave;
       		return 0;
       	}
	/* The meanings of different values for this status byte is currently unknown */
	status = njb3_bytes_to_16bit(&status_data[0]);
	if (status == 0) {
	  fileid = njb3_bytes_to_32bit(&status_data[2]);
	} else if (status == NJB3_STATUS_BAD_FILESIZE) {
	  NJB_ERROR(njb, EO_BADDATA);
	  fileid = 0;
	} else {
	  /* FIXME: handle more status codes */
	  printf("LIBNJB Panic: njb3_create_file returned status code %04x!\n", status);
	  NJB_ERROR(njb, EO_BADSTATUS);
	  fileid = 0;
	}

	__leave;
	return fileid;
}

u_int32_t njb3_send_file_chunk(njb_t *njb, unsigned char *chunk, u_int32_t chunksize, u_int32_t fileid)
{
  __dsub= "njb3_send_file_chunk";
  unsigned char njb3_send_file_chunk[]={0x00,0x03,0x00,0x01,0x00,0x00,0x00,0x00,
					0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  u_int32_t bread;
  u_int16_t status;
  u_int32_t storedsize;
  unsigned char status_data[]={0x00,0x00,0x00,0x00,0x00,0x00};
  /* Structure: 8 bytes
   * 2 bytes unknown 0x0003
   * 2 bytes unknown 0x0001
   * 4 bytes file ID
   * 2 bytes unknown 0x0000
   * 2 bytes unknown 0x0000
   * 4 bytes file chunk size
   */
  
  __enter;
  
  /* Prepare the command */
  from_32bit_to_njb3_bytes(fileid, &njb3_send_file_chunk[4]);
  from_32bit_to_njb3_bytes(chunksize, &njb3_send_file_chunk[12]);
  
  if (send_njb3_command(njb, njb3_send_file_chunk, 16) == -1){
    __leave;
    return -1;
  }
  if (send_njb3_command(njb, chunk, chunksize) == -1){
    __leave;
    return -1;
  }
  if ( (bread = usb_pipe_read(njb, status_data, 6)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 2 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }
  status = njb3_bytes_to_16bit(&status_data[0]);
  storedsize = njb3_bytes_to_32bit(&status_data[2]);
  if (status != NJB3_STATUS_OK){
    printf("LIBNJB Panic: njb3_send_file_chunk() returned status code %04x! (short write?)\n", status);
    printf("Chunk size: %04x, Written size: %04x\n", chunksize, storedsize);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  
  /* Allow short writes */
  __leave;
  return storedsize;
}

int njb3_send_file_complete(njb_t *njb, u_int32_t fileid)
{
  __dsub= "njb3_send_file_complete";
  /*
    unsigned char njb3_verify_transfer[]={0x00,0x10,0x00,0x01,0x00,0x00,0x00,0x00,
    0x00,0x0a,0x00,0x14,0x00,0x00,0x00,0x00,
    0x00,0x05,0xe8,0x62,0x00,0x00};
  */
  /* Structure: 22 bytes
   * 2 bytes unknown 0x0010
   * 2 bytes unknown 0x0001
   * 18 bytes unknown stuff
   * I suspect this is a CRC sum check for the transfer. We
   * don't use it right now, but could be made to work. Reads
   * back 2 bytes status code, which is OK when its 0x0000 I think.
   */
  unsigned char njb3_send_file_complete[]={0x00,0x09,0x00,0x01,0x00,0x00,0x00,0x00};
  /* Structure: 8 bytes
   * 2 bytes unknown 0x0009
   * 2 bytes unknown 0x0001
   * 4 bytes track ID
   */
  u_int16_t status;
  
  __enter;
  
  /*
   * Here, the verification command above should be sent. We
   * yet don't know how to implement the verification however, someone
   * has to take a look at the transfers and try to reverse engineer the
   * algorithm for calculation of the verification command (possibly
   * a CRC or simple checksum).
   *
   * After the verification command, 2 bytes are read in from the bus. 
   * These should probably be 0x0000 if the checksum/CRC was correct.
   */
  
  /* Prepare the command */
  from_32bit_to_njb3_bytes(fileid, &njb3_send_file_complete[4]);
  
  if (send_njb3_command(njb, njb3_send_file_complete, 8) == -1){
    __leave;
    return -1;
  }
  
  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }
  
  if (status != NJB3_STATUS_OK){
    printf("LIBNJB Panic: njb3_send_file_complete() returned status code %04x! (short write?)\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  __leave;
  return 0;
}


int njb3_create_folder(njb_t *njb, const char *name, u_int32_t *folderid)
{
  __dsub = "njb3_create_folder";
  u_int32_t bread;
  u_int16_t status;
  u_int32_t dirid;
  u_int32_t tagsize;
  unsigned char *ptag;
  unsigned char *data;
  unsigned char status_data[]={0x00,0x00,0x00,0x00,0x00,0x00};
  unsigned char create_folder_cmd[]={0x00,0x0a,0x00,0x01,0x00,0x00,0x00,0x00};
  /* Structure: 8 bytes
   * 2 bytes command 0x000a - create empty item (same used for creating playlists)
   * 2 bytes command 0x0001
   * 2 bytes 0x0000
   * 2 bytes database 0x0000 (datafile)
   * After this the tags follow
   */
  __enter;

  ptag = new_folder_pack3(njb, name, &tagsize);
  if (ptag == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return 0;
  }

  /* Command size plus tag size */
  data = malloc(8+tagsize);
  if (data == NULL) {
    free(ptag);
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return 0;
  }
  memcpy(&data[0], create_folder_cmd, 8);
  /* Copy tag to metadata chunk */
  memcpy(&data[8], ptag, tagsize);
  /* Free tag */
  free(ptag);
  if (send_njb3_command(njb, data, 8+tagsize) == -1){
    free(data);
    __leave;
    return 0;
  }
  free(data);
  if ( (bread= usb_pipe_read(njb, status_data, 6)) == -1 ) {
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return 0;
  } else if ( bread < 2 ) {
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return 0;
  }
  /* The meanings of different values for this status byte is currently unknown */
  status = njb3_bytes_to_16bit(&status_data[0]);
  if (status == 0) {
    dirid = njb3_bytes_to_32bit(&status_data[2]);
  } else if (status == NJB3_STATUS_BAD_FILESIZE) {
    NJB_ERROR(njb, EO_BADDATA);
    dirid = 0;
  } else {
    /* FIXME: handle more status codes */
    printf("LIBNJB Panic: njb3_create_folder returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    dirid = 0;
  }
  *folderid = dirid;
  __leave;
  if (dirid == 0) {
    return -1;
  } else {
    return 0;
  }
}

int njb3_delete_item (njb_t *njb, u_int32_t itemid)
{
  __dsub = "njb3_delete_item";
  unsigned char njb3_delitem[]={0x00,0x05,0x00,0x01,0x00,0x00,0x00,0x00};
  /* Structure: 8 bytes
   * 2 bytes unknown 0x0005
   * 2 bytes unknown 0x0001
   * 4 bytes item ID (could be track, playlist, datafile ...)
   */
  u_int16_t status;
  
  __enter;
  
  from_32bit_to_njb3_bytes(itemid, &njb3_delitem[4]);
  
  if (send_njb3_command(njb, njb3_delitem, 0x08) == -1){
    __leave;
    return -1;
  }
  
  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }

  /* 
   * The Zen Touch obviously return some weirdo stuff sometimes, like 
   * NJB3_STATUS_EMPTY_CHUNK after deleting a playlist item...
   */
  if (status != NJB3_STATUS_OK &&
      status != NJB3_STATUS_EMPTY_CHUNK) {
    printf("LIBNJB Panic: njb3_delete_item() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  
  __leave;
  return 0;
}

/**
 * This function will update a single 16bit metadata frame associated with
 * a certain item (track, playlist or datafile). Only call this function to
 * modify 16-bit values!
 *
 * @param itemid   the item ID (track, playlist or datafile) to modify.
 * @param frameid  the frame ID of the frame to modyfy. Must be a 16-bit frame.
 * @param valud    the new 16-bit value.
 */
int njb3_update_16bit_frame(njb_t *njb, u_int32_t itemid, u_int16_t frameid, u_int16_t value)
{
  __dsub= "njb3_update_16bit_frame";
  unsigned char njb3_update16[]={0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,
				 0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00};
  /* Structure: 8 bytes
   * 2 bytes unknown 0x0001 - command update
   * 2 bytes unknown 0x0001
   * 4 bytes track ID
   * 2 bytes tag frame length (in this case 4)
   * 2 bytes tag frame ID
   * 2 bytes tag frame content
   * 2 bytes termination 0x0000
   */
  u_int16_t status;
  
  __enter;
  
  from_32bit_to_njb3_bytes(itemid, &njb3_update16[4]);
  from_16bit_to_njb3_bytes(frameid, &njb3_update16[10]);
  from_16bit_to_njb3_bytes(value, &njb3_update16[12]);
  
  if (send_njb3_command(njb, njb3_update16, 0x10) == -1){
    __leave;
    return -1;
  }
  
  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }
  
  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_update_16bit_frame() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  
  __leave;
  return 0;
}


/**
 * This function updates a single string of metadata associated with
 * a certain item (track, playlist or datafile). Only call this routine
 * to modify string frames!
 *
 * @param itemid   the item ID (track, playlist or datafile) to be modified.
 * @param frameid  the frame ID of the frame to be updated.
 * @param str      the new string value.
 */
int njb3_update_string_frame(njb_t *njb, u_int32_t itemid, u_int16_t frameid, unsigned char *str)
{
  __dsub= "njb3_update_string_frame";
  unsigned char njb3_update_frame[]={0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00};
  /* Structure: 8 bytes
   * 2 bytes unknown 0x0001
   * 2 bytes unknown 0x0001
   * 4 bytes track ID
   * 2 bytes tag frame length
   * 2 bytes tag frame ID
   * 2 bytes tag frame content
   * 2 bytes termination 0x0000
   */
  unsigned char *data;
  u_int16_t status;
  u_int16_t strsize;
  u_int16_t framesize;
  u_int32_t cmdsize;
  
  __enter;
  
  strsize = ucs2strlen(str)*2 + 2;
  framesize = strsize + 2;
  cmdsize =  12 + framesize;
  
  data = malloc(cmdsize);
  if (data == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  memset(data, 0, cmdsize);
  memcpy(&data[0], njb3_update_frame, 8);
  
  from_32bit_to_njb3_bytes(itemid, &data[4]);
  from_16bit_to_njb3_bytes(framesize, &data[8]);
  from_16bit_to_njb3_bytes(frameid, &data[10]);
  memcpy(&data[12], str, strsize);
  
  if (send_njb3_command(njb, data, cmdsize) == -1){
    free(data);
    __leave;
    return -1;
  }
  
  if (njb3_get_status(njb, &status) == -1) {
    free(data);
    __leave;
    return -1;
  }

  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_update_string_frame() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    free(data);
    __leave;
    return -1;
  }
  
  free(data);
  __leave;
  return 0;
}

/**
 * A function to update a block of metadata on the series 3 devices.
 * @param itemid the track (or similar) whose metadata is to be updated.
 * @param ptag a packed metadata structure for series 3 devices.
 * @return 0 on success, -1 on failure.
 */
int njb3_update_tag(njb_t *njb, u_int32_t itemid, unsigned char *ptag, u_int32_t ptagsize)
{
  __dsub= "njb3_update_tag";
  unsigned char njb3_update_frame[]={0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00};
  /* Structure: 8 bytes
   * 2 bytes command 0x0001 - means update metadata database for item
   * 2 bytes command 0x0001
   * 4 bytes track ID
   * Then repeat the following for each tag to be updated:
   *   2 bytes tag frame length
   *   2 bytes tag frame ID
   *   n bytes tag frame content
   * End with:
   * 2 bytes termination 0x0000
   */
  unsigned char *data;
  u_int16_t status;
  u_int32_t totsize = 8+ptagsize+2;

  __enter;

  data = malloc(totsize);
  if (data == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  memset(data, 0, totsize);
  memcpy(&data[0], njb3_update_frame, 8);
  memcpy(&data[8], ptag, ptagsize);
  from_32bit_to_njb3_bytes(itemid, &data[4]);

  /* The two extra bytes are for the two terminator bytes */
  if (send_njb3_command(njb, data, totsize) == -1){
    free(data);
    __leave;
    return -1;
  }

  if (njb3_get_status(njb, &status) == -1) {
    free(data);
    __leave;
    return -1;
  }

  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_update_tag returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    free(data);
    __leave;
    return -1;
  }
  
  free(data);

  __leave;
  return 0;
}

/**
 * This function creates a new playlist on the device.
 *
 * @param name  the name of the playlist to create, as a string.
 * @param plid  a pointer to a 32-bit numer that will contain the
 *              new playlist ID after this routine has been called.
 * @return 0 on success, -1 on failure.
 */
int njb3_create_playlist (njb_t *njb, char *name, u_int32_t *plid)
{
  __dsub= "njb3_create_playlist";
  unsigned char njb3_create_pl[]={0x00,0x0a,0x00,0x01,0x00,0x00,0x00,0x01};
  /* Structure: 8 bytes
   * 2 bytes command 0x000a
   * 2 bytes command 0x0001
   * 2 bytes unknown 0x0000
   * 2 bytes database target (0x0001 = playlist database)
   */
  unsigned char status_data[]={0x00,0x00,0x00,0x00,0x00,0x00};
  unsigned char *data;
  ssize_t bread;
  u_int16_t status;
  u_int16_t strsize;
  u_int16_t framesize;
  u_int32_t cmdsize;
  
  __enter;
  
  strsize = ucs2strlen((unsigned char *) name)*2 + 2;
  framesize = strsize + 2;
  cmdsize =  16 + framesize;
  
  data = malloc(cmdsize);
  if (data == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  memset(data, 0, cmdsize);
  memcpy(&data[0], njb3_create_pl, 8);
  
  from_16bit_to_njb3_bytes(framesize, &data[8]);
  from_16bit_to_njb3_bytes(NJB3_PLNAME_FRAME_ID, &data[10]);
  memcpy(&data[12], name, strsize);
  
  if (send_njb3_command(njb, data, cmdsize) == -1){
    free(data);
    __leave;
    return -1;
  }
  if ( (bread= usb_pipe_read(njb, status_data, 6)) == -1 ) {
    free(data);
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 2 ) {
    free(data);
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }
  status = njb3_bytes_to_16bit(&status_data[0]);
  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_create_playlist returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    free(data);
    __leave;
    return -1;
  }
  /* Return the new playlist ID */
  *plid = njb3_bytes_to_32bit(&status_data[2]);
  
  free(data);
  __leave;
  return 0;
}

/**
 * This takes an array of 32-bit track ID:s and adds it to a certain playlist.
 *
 * @param plid     a pointer to the ID of the playlist to add tracks to. The ID
 *                 will change during this operation, so it is important to pass
 *                 in a pointer.
 * @param trids    an array of tracks to add
 * @param ntracks  absolute number of tracks in the array.
 */
int njb3_add_multiple_tracks_to_playlist (njb_t *njb, u_int32_t *plid, u_int32_t *trids, u_int16_t ntracks)
{
  __dsub= "njb3_add_multiple_tracks_to_playlist";
  unsigned char njb3_addtracks[]={0x01,0x07,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x1c};
  /* Structure: 0x0e + n*2 bytes
   * 2 bytes unknown 0x0107
   * 2 bytes unknown 0x0001
   * 4 bytes playlist ID
   * 2 bytes ntracks = following tracklist length
   * 2 bytes 0x011c NJB3_PLTRACKS_FRAME_ID
   * ntracks * 4 bytes track IDs of tracks to add.
   * 2 bytes unknown 0x0000
   */
  unsigned char status_data[]={0x00,0x00,0x00,0x00,0x00,0x00};
  unsigned char *data;
  u_int16_t trackcmdsize;
  u_int32_t cmdsize;
  u_int32_t bp;
  ssize_t bread;
  u_int16_t status;
  int i;
  
  __enter;
  trackcmdsize = ntracks*4 + 2;
  cmdsize = 0x0c + trackcmdsize;
  
  
  data = malloc(cmdsize);
  if (data == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return -1;
  }
  memset(data, 0, cmdsize);
  memcpy(&data[0], njb3_addtracks, 12);
  /* add playlist ID */
  from_32bit_to_njb3_bytes(*plid, &data[4]);
  /* Add number of tracks */
  from_16bit_to_njb3_bytes(trackcmdsize, &data[8]);
  /* Add the track IDs */
  bp = 0;
  for (i= 0; i< ntracks; i++) {
    from_32bit_to_njb3_bytes(trids[i], &data[12+bp]);
    bp+= 4;
  }
  /* Send the command */
  if (send_njb3_command(njb, data, cmdsize) == -1){
    free(data);
    __leave;
    return -1;
  }
  if ( (bread= usb_pipe_read(njb, status_data, 6)) == -1 ) {
    free(data);
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return -1;
  } else if ( bread < 2 ) {
    free(data);
    NJB_ERROR(njb, EO_RDSHORT);
    __leave;
    return -1;
  }
  
  status = njb3_bytes_to_16bit(&status_data[0]);
  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_add_multiple_tracks_to_playlist returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    free(data);
    __leave;
    return -1;
  }
  /* Retrieve the new playlist ID */
  *plid = njb3_bytes_to_32bit(&status_data[2]);
  
  __leave;
  return 0;
}


/**
 * This command turns the EAX DSP processor on or off. You will
 * also have to adjust the currently used EAX effect with
 * njb3_adjust_eax() below.
 */
int njb3_control_eax_processor (njb_t * njb, u_int16_t state)
{
  __dsub= "njb3_control_eax_processor";

  u_int16_t status;
  unsigned char njb3_ctrl_eax[] = {
    0x00, 0x07, 0x00, 0x01, 0x00, 0x04, 0x02, 0x0a,
    0x00, 0x00, 0x00, 0x00
  };
  /* Structure: 12 bytes
   * 4 bytes command 0x00070001U - write register?
   * 2 bytes length of following record 0x0004
   * 2 bytes metadata frame name 0x020a
   * 2 bytes off/on 0x0000 = off 0x0001 = on
   * 2 bytes termination 0x0000
   */

  __enter;

  from_16bit_to_njb3_bytes(state, &njb3_ctrl_eax[8]);

  if (send_njb3_command (njb, njb3_ctrl_eax, 0x0c) == -1) {
    __leave;
    return -1;
  }

  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }

  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_control_eax_processor() returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }

  __leave;
  return 0;
}

int njb3_adjust_eax(njb_t *njb, 
		    u_int16_t eaxid, 
		    u_int16_t patchindex, 
		    u_int16_t active, 
		    u_int16_t scalevalue)
{
    __dsub= "njb3_adjust_eax";
  /* 02 01 00 01 02 03 00 00 00 04 02 02 00 01 00 04 02 03 00 49 00 00 */
  /*
   * 02 01 00 01 02 04 00 05 00 04 02 02 00 01 00 00 = select environment
   * 02 01 00 01 02 05 00 01 00 04 02 02 00 01 00 00 = select EQ preset
   * 02 01 00 01 02 05 00 08 00 04 02 02 00 01 00 00 = select EQ custom (just a certain preset)
   * 02 01 00 01 02 06 00 01 00 04 02 02 00 01 00 00 = select a spatialization
   * 02 01 00 01 02 07 00 02 00 04 02 02 00 01 00 00 = select a timescaling
   * 02 01 00 01 02 08 00 01 00 04 02 02 00 01 00 00 = select a smartvolume
   */
  unsigned char njb3_adjust_eax[]=
    {0x02, 0x01, 0x00, 0x01, 0x02, 0x04, 0x00, 0x00,
     0x00, 0x04, 0x02, 0x02, 0x00, 0x01, 0x00, 0x04,
     0x02, 0x03, 0x00, 0x00, 0x00, 0x00};

  /* Structure: 0x10 bytes
   * 2 bytes unknown 0x0201
   * 2 bytes unknown 0x0001
   * 2 bytes parameter frame ID (e.g. 0x0204 = Advanced EQalizer)
   * 2 bytes effect index
   * 2 bytes length 0x0004
   * 2 bytes EAX ACTIVE frame ID 0x0202 (Set to active)
   * 2 bytes EAX ACTIVE frame value 0x0001 (ON)
   *    2 bytes termination 0x0000
   * OR
   *    2 bytes length 0x0004
   *    2 bytes parameter frame ID (again)
   *    2 bytes scale value
   *    2 bytes termination 0x0000
   */
  u_int16_t status;

  __enter;

  /* Effect to be adjusted */
  /*
    printf("Sending EAX adjustment for effect %04X\n  active %04X\n  patch %04X\n  value %04X\n",
    eaxid, active, patchindex, scalevalue);
  */
  from_16bit_to_njb3_bytes(eaxid, &njb3_adjust_eax[0x04]);
  from_16bit_to_njb3_bytes(active, &njb3_adjust_eax[0x0c]);
  from_16bit_to_njb3_bytes(patchindex, &njb3_adjust_eax[0x06]);
  if (scalevalue == 0x0000) {
    /* If no scalevalue, terminate command */
    from_16bit_to_njb3_bytes(0x0000, &njb3_adjust_eax[0x0e]);
    if (send_njb3_command(njb, njb3_adjust_eax, 0x10) == -1){
      __leave;
      return -1;
    }
  } else {
    /* Else also transmit the scalevalue */
    from_16bit_to_njb3_bytes(eaxid, &njb3_adjust_eax[0x10]);
    from_16bit_to_njb3_bytes(scalevalue, &njb3_adjust_eax[0x12]);
    if (send_njb3_command(njb, njb3_adjust_eax, 0x16) == -1){
      __leave;
      return -1;
    }
  }

  /* Read back status (command result) */
  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }

  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_adjust_eax returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }

  __leave;
  
  return 0;
}


/* Destroys any leftover EAX posts */
static void destroy_eax_from_njb(njb_t *njb)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;

  if (state->next_eax != NULL) {
    njb_eax_t *eax = state->next_eax;
    while (eax != NULL) {
      njb_eax_t *destroyeax = eax;
      eax = eax->next;
      destroy_eax_type(destroyeax);
    }
  }
  state->first_eax = NULL;
  state->next_eax = NULL;
}

static int parse_eax_block(unsigned char *data,
			   u_int16_t effect_number,
			   char *effect_name,
			   u_int8_t effect_group,
			   u_int8_t exclusive,
			   njb3_state_t *state)
{
  unsigned char *dp;
  u_int16_t eaxlen = 0x0000;
  u_int16_t eaxtype;
  u_int8_t eax_num = 0;
  char *eaxnames[128];
  u_int8_t strarrsize;
  njb_eax_t *eax;

  eax = new_eax_type();
  if (eax == NULL)
    return -1;

  /* Set number */
  eax->number = effect_number;
  /* Set name */
  eax->name = effect_name;
  /* Set group */
  eax->group = effect_group;
  /* Default to fixed option */
  eax->type = NJB_EAX_FIXED_OPTION_CONTROL;
  /* All EAX effects are exclusive in the newer boxes */
  eax->exclusive = exclusive;

  /* Get the strings and stuff */
  dp = data;
  eaxlen = njb3_bytes_to_16bit(dp);
  while (eaxlen != 0x0001U) {
    eaxtype = njb3_bytes_to_16bit(&dp[2]);
    if (eaxtype == NJB3_EAX_TYPENAME) {
      char *string = ucs2tostr(&dp[4]);

      if (eax_num == 0) {
	/* First add a default "off" patch */
	eaxnames[eax_num] = strdup("(Off)");
	eax_num ++;
      }

      eaxnames[eax_num] = string;
      eax_num ++;
      /* Each string is terminated by two consecutive 0x0000 */
    }
    else if (eaxtype == NJB3_MINMAX_ID) {
      /* If we have minmax values, we have a slider */
      eax->type = NJB_EAX_SLIDER_CONTROL;
      eax->max_value = njb3_bytes_to_16bit(&dp[6]);
      eax->min_value = njb3_bytes_to_16bit(&dp[8]);
    }
    else if (eaxtype == NJB3_EAX_ACTIVE_ID) {
      /* u_int16_t active = njb3_bytes_to_16bit(&dp[4]); */
      /* Just throw this away as of now... */
    }
    else if (eaxtype == NJB3_EAX_INDEX_ID) {
      eax->current_value = njb3_bytes_to_16bit(&dp[4]);
    }
    else if (eaxtype == effect_number) {
      eax->current_value = njb3_bytes_to_16bit(&dp[4]);
    }
    dp += eaxlen;
    dp += 2; // The length frame too.
    while ((eaxlen = njb3_bytes_to_16bit(dp)) == 0x0000U) {
      dp +=2;
    }
  }

  /* Decipher volume frame, typical example:
   *
   * 0000 000a 0201 0000 0064 0001 0000 0004
   *                      max  min
   *
   * 0202 0001 0004 0203 002f 0000 0000 0001
   *    active          value            end
   */

  /*
   * For fixed options, add the names and values.
   */
  if (eax->type == NJB_EAX_FIXED_OPTION_CONTROL) {
    eax->min_value = 0;
    if (eax_num > 0) {
      eax->max_value = eax_num-1;
      /* Allocate and copy name pointers */
      strarrsize = eax_num * sizeof(char *);
      eax->option_names = (char **) malloc(strarrsize);
      memcpy(eax->option_names, eaxnames, strarrsize);
    } else {
      eax->max_value = 0;
    }
    /* Default to (Off) */
    eax->current_value = 0;
  }

  /* Add the new EAX to our state */
  if (state->first_eax == NULL) {
    state->first_eax = eax;
    state->next_eax = NULL;
  } else if (state->next_eax == NULL) {
    state->first_eax->next = eax;
    state->next_eax = eax;
  } else {
    state->next_eax->next = eax;
    state->next_eax = eax;
  }

  return 0;
}

void njb3_read_eaxtypes(njb_t *njb)
{
  __dsub= "njb3_get_eax";
  unsigned char njb3_get_eaxnames[]={0x02, 0x00, 0x00, 0x01, 0x02, 0x05, 0x00,
				     0x00, 0x00, 0x1e, 0x3c, 0x00, 0x00, 0x08,
				     0x02, 0x01, 0x02, 0x02, 0x01, 0x0e, 0x02,
				     0x05, 0x00, 0x00};
  /* Structure: 0x18 bytes
   * 2 bytes unknown 0x0200
   * 2 bytes unknown 0x0001
   * 2 bytes EAX frame request (0x0205 = Equalizer)
   * 2 bytes termination 0x0000
   * 4 bytes unknown 0x001e3c00 (memory address???)
   * 2 bytes request length 0x0008
   * 2 bytes request frame 0x0201 (Min/Max values, if available)
   * 2 bytes request frame 0x0202 (Effect currently active or not)
   * 2 bytes request frame 0x010e (String with EAX type name)
   * 2 bytes request frame (0x0205 = Equalizer setting, 0x020c = Selected index etc.)
   * 2 bytes termination 0x0000
   */
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
  unsigned char *data = NULL;
  unsigned char *data2 = NULL;
  u_int32_t bread;
  u_int16_t status;
  u_int16_t dp, eaxlen, eaxtype;
  u_int8_t group = 0;
  u_int16_t eaxid = 0x0000;

  __enter;
  
  /* Clean from any previous scan */
  destroy_eax_from_njb(njb);

  /* A scanning buffer */
  if((data = (unsigned char *) malloc(NJB3_SHORTREAD_BUFSIZE)) == NULL){
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return;
  }
  if((data2 = (unsigned char *) malloc(NJB3_SHORTREAD_BUFSIZE)) == NULL){
    free(data2);
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return;
  }


  /*
   * Retrieve the current volume setting separately
   */
  from_16bit_to_njb3_bytes(NJB3_VOLUME_FRAME_ID, &njb3_get_eaxnames[0x04]);
  from_16bit_to_njb3_bytes(NJB3_VOLUME_FRAME_ID, &njb3_get_eaxnames[0x14]);

  if (send_njb3_command(njb, njb3_get_eaxnames, 0x18) == -1){
    free(data);
    free(data2);
    __leave;
    return;
  }
  if ( (bread = usb_pipe_read(njb, data, NJB3_SHORTREAD_BUFSIZE)) == -1 ) {
    free(data);
    free(data2);
    NJB_ERROR(njb, EO_USBBLK);
    __leave;
    return;
  }

  status = njb3_bytes_to_16bit(&data[0]);

  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_get_eaxnames (VOLUME VALUE) returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    free(data);
    free(data2);
    __leave;
    return;
  }
  if (parse_eax_block(&data[2], 
		      NJB3_VOLUME_FRAME_ID,
		      strdup("Volume"),
		      group,
		      0x00,
		      state) == -1) {
    free(data);
    __leave;
    return;
  }
  group ++;

  /* Get all ID:s, then parse each one individually */
  from_16bit_to_njb3_bytes(NJB3_EAXID_FRAME_ID, &njb3_get_eaxnames[0x04]);
  from_16bit_to_njb3_bytes(NJB3_EAX_INDEX_ID, &njb3_get_eaxnames[0x14]);
 
  if (send_njb3_command(njb, njb3_get_eaxnames, 0x18) == -1){
    free(data);
    free(data2);
    /* Fix up state before premature return */
    state->next_eax = state->first_eax;
    __leave;
    return;
  }
  if ( (bread = usb_pipe_read(njb, data, NJB3_SHORTREAD_BUFSIZE)) == -1 ) {
    free(data);
    free(data2);
    NJB_ERROR(njb, EO_USBBLK);
    /* Fix up state before premature return */
    state->next_eax = state->first_eax;
    __leave;
    return;
  }

  status = njb3_bytes_to_16bit(&data[0]);
  /* 
   * Some jukeboxes (like the Zen Micro) does not implement
   * any EAX features at all (or they are not controllable
   * from the host).
   */
  if (status == NJB3_STATUS_NOTIMPLEMENTED) {
    free(data);
    free(data2);
    /* Fix up state before premature return */
    state->next_eax = state->first_eax;
    __leave;
    return;
  }
  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_get_eaxnames returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    free(data);
    free(data2);
    /* Fix up state before premature return */
    state->next_eax = state->first_eax;
    __leave;
    return;
  }

  dp = 2;
  eaxlen = njb3_bytes_to_16bit(&data[dp]);
  while (eaxlen != 0x0001) {
    eaxtype = njb3_bytes_to_16bit(&data[dp+2]);

    if (eaxtype == NJB3_EAXID_FRAME_ID) {
      eaxid = njb3_bytes_to_16bit(&data[dp+4]);
    } else if (eaxtype == NJB3_EAX_TYPENAME) {
      char *eaxname = ucs2tostr(&data[dp+4]);

      /* Get it and parse it */
      from_16bit_to_njb3_bytes(eaxid, &njb3_get_eaxnames[0x04]);
      from_16bit_to_njb3_bytes(eaxid, &njb3_get_eaxnames[0x14]);

      if (send_njb3_command(njb, njb3_get_eaxnames, 0x18) == -1){
	free(data);
	free(data2);
	/* Fix up state before premature return */
	state->next_eax = state->first_eax;
	__leave;
	return;
      }
      if ( (bread = usb_pipe_read(njb, data2, NJB3_SHORTREAD_BUFSIZE)) == -1 ) {
	free(data);
	free(data2);
	/* Fix up state before premature return */
	state->next_eax = state->first_eax;
	NJB_ERROR(njb, EO_USBBLK);
	__leave;
	return;
      }

      status = njb3_bytes_to_16bit(&data2[0]);
      if (status != NJB3_STATUS_OK) {
	printf("LIBNJB Panic: njb3_get_eaxnames for effect %04X returned status code %04x!\n", 
	       eaxid,
	       status);
	NJB_ERROR(njb, EO_BADSTATUS);
	free(data);
	free(data2);
	/* Fix up state before premature return */
	state->next_eax = state->first_eax;
	__leave;
	return;
      }
      if (parse_eax_block(&data2[2],
			  eaxid,
			  eaxname,
			  group,
			  0x01,
			  state) == -1) {
	free(data);
	free(data2);
	/* Fix up state before premature return */
	state->next_eax = state->first_eax;
	__leave;
	return;
      }
    }
    dp += eaxlen;
    dp += 2; // The length frame too.
    while ((eaxlen = njb3_bytes_to_16bit(&data[dp])) == 0x0000) {
      dp +=2;
    }
  }

  free(data);
  free(data2);

  /* Fix up state */
  state->next_eax = state->first_eax;

  __leave;
}

njb_eax_t *njb3_get_nexteax(njb_t *njb)
{
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;
  njb_eax_t *ret;

  ret = state->next_eax;
  if (ret != NULL) {
    state->next_eax = ret->next;
  }  
  return ret;
}


/**
 * This announces a firmware image which will then be sent
 * in several chunks.
 *
 * @param njb a pointer to the njb device object to use
 * @param size the total size of the firmware image
 * @return 0 on success, -1 on failure
 */
int njb3_announce_firmware(njb_t *njb, u_int32_t size) {
  __dsub= "njb3_get_eax";
  unsigned char njb3_announce_firmware[]=
    {0x00, 0x0b, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

  __enter;
  from_32bit_to_njb3_bytes(size, &njb3_announce_firmware[0x06]);
  if (send_njb3_command(njb, njb3_announce_firmware, 0x0a) == -1){
    __leave;
    return -1;
  }
  __leave;
  return 0;
}

/**
 * This sends a chunk of firmware. Typically the chunks are
 * <code>NJB3_FIRMWARE_CHUNK_SIZE</code> each, except for the
 * last chunk.
 *
 * @param njb a pointer to the njb device object to use
 * @param chunksize the size of this chunk
 * @param chunk a pointer to the raw bytes representing this
 *              firmware chunk
 * @return 0 on success, -1 on failure
 */
u_int32_t njb3_send_firmware_chunk(njb_t *njb, u_int32_t chunksize, unsigned char *chunk) {
  __dsub = "njb3_send_firmware_chunk";
  __enter;
  /*
   * TODO: I have indications that on the NJB3, this send
   * command is divided into two bulk transfers if the chunksize
   * is > 0x20000, e.g. a full chunk of 0x40000 is sent in two bulk
   * transfers. On the Zen Xtra we know for sure that the firmware is
   * sent in 0x40000 continous chunks.
   */
  if (send_njb3_command(njb, chunk, chunksize) == -1){
    __leave;
    /* Announce as if we sent no bytes */
    return 0;
  }
  __leave;
  return chunksize;
}


/**
 * This simply reads back the device status after a firmware
 * upgrade.
 */
int njb3_get_firmware_confirmation(njb_t *njb) {
  __dsub = "njb3_get_firmware_confirmation";
  u_int16_t status;
  __enter;

  if (njb3_get_status(njb, &status) == -1) {
    __leave;
    return -1;
  }
  if (status != NJB3_STATUS_OK) {
    printf("LIBNJB Panic: njb3_get_firmware_confirmation returned status code %04x!\n", status);
    NJB_ERROR(njb, EO_BADSTATUS);
    __leave;
    return -1;
  }
  __leave;
  return 0;
}


/**
 * Cleans up any dangling lists in the njb_t state holder
 * struct, and other stuff related to the state.
 */
void njb3_destroy_state(njb_t *njb) {
  njb3_state_t *state = (njb3_state_t *) njb->protocol_state;  
  njb_keyval_t *key = state->first_key;

  /* Free the keys */
  while ( key != NULL) {
    njb_keyval_t *tmp = key->next;
    free(key);
    key = tmp;
  }
  destroy_song_from_njb(njb);
  destroy_pl_from_njb(njb);
  destroy_df_from_njb(njb);
  destroy_eax_from_njb(njb);
  /* Free product name string */
  if (state->product_name != NULL) {
    free(state->product_name);
  }
  /* Finally destroy the state */
  free(state);
  njb->protocol_state = NULL;
}

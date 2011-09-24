/**
 * \file usb_io.c
 *
 * This file contain some USB-specific code that is used by all
 * devices.
 */

#include <string.h>
#include <usb.h>
#include "base.h"
#include "libnjb.h"
#include "ioutil.h"
#include "usb_io.h"
#include "njb_error.h"

extern int __sub_depth;

/**
 * This function writes a number of bytes from a buffer 
 * to a devices OUT endpoint.
 *
 * @param njb the jukebox object to use
 * @param buf the buffer to send bytes from
 * @param nbytes the number of bytes to write
 * @return the number of bytes actually written
 */
ssize_t usb_pipe_write(njb_t *njb, void *buf, size_t nbytes)
{
  ssize_t bwritten = -1;

  /* Used for all libnjb enabled platforms. 
   * This section might be the source of timeout problems so
   * it is currently being tested. Also see pipe_read below. */
  
  int usb_timeout = 10 * nbytes; /* that's 5ms / byte */
  int retransmit = 10; /* Try ten times! (That should do it...) */
  
  /* Set a timeout floor */
  if (usb_timeout < USBTIMEOUT)
    usb_timeout = USBTIMEOUT;
  
  while (retransmit > 0) {
    bwritten = usb_bulk_write(njb->dev, njb->usb_bulk_out_ep, buf, nbytes, 
			     usb_timeout);
    if ( bwritten < 0 )
      retransmit--;
    else
      break;
  }
  if (retransmit == 0) {
    njb_error_add_string (njb, "usb_bulk_write", usb_strerror());
    return -1;
  }
  
  if ( njb_debug(DD_USBBLK|DD_USBBLKLIM) ) {
    size_t bytes = ( njb_debug(DD_USBBLK) ) ? nbytes : 16;
    
    fprintf(stderr, "Bulk >>\n");
    data_dump_ascii(stderr, buf, bytes, 0);
    fprintf(stderr, "\n");
  }
  
  return bwritten;
}


/**
 * This function reads a chunk of bytes to a buffer from
 * a device's IN endpoint.
 *
 * @param njb the jukebox object to use
 * @param buf the buffer to store the bytes in
 * @param nbytes the number of bytes to read in
 * @return the number of bytes actually read
 */
ssize_t usb_pipe_read (njb_t *njb, void *buf, size_t nbytes)
{
  ssize_t bread = 0;
  
  /* Used for all libnjb enabled platforms. 
   * This section might be the source of timeout problems so
   * it is currently being tested. Also see pipe_write above. */
  int usb_timeout = 10 * nbytes; /* that's 10ms / byte */
  int retransmit = 10; /* Try ten times! (That should do it...) */
  
  /* Set a timeout floor */
  if (usb_timeout < USBTIMEOUT)
    usb_timeout = USBTIMEOUT;
  
  while (retransmit > 0) {
    bread = usb_bulk_read(njb->dev, njb->usb_bulk_in_ep, buf, nbytes, 
			 usb_timeout);
    /* This should be changed to (bread < nbytes) asap, but needs
     * an NJB3 to test it, it cancels out short reads if I set
     * it to that, so these must first be avoided in all NJB3
     * code by properly checking length etc. */
    if ( bread < 0 )
      retransmit--;
    else
      break;
  }
  if ( retransmit == 0 ) {
    njb_error_add_string (njb, "usb_bulk_read", usb_strerror());
    return -1;
  }
  
  if ( njb_debug(DD_USBBLK|DD_USBBLKLIM) ) {
    size_t bytes = ( njb_debug(DD_USBBLK) ) ? bread : 16;
    
    fprintf(stderr, "Bulk <<\n");
    data_dump_ascii(stderr, buf, bytes, 0);
    fprintf(stderr, "\n");
  }
  
  return bread;
}


/**
 * This function sends a USB SETUP 8-byte command
 * across to endpoint 0 on the device.
 */
int usb_setup (njb_t *njb, int type, int request, int value,
	int index, int length, void *data)
{
  u_int8_t setup[8];
  usb_dev_handle *dev = njb->dev;
  
  if ( njb_debug(DD_USBCTL) ) {
    memset(setup, 0, 8);
    setup[0]= type;
    setup[1]= request;
    if ( value ) {
      setup[2] = value & 255;
      setup[3] = (value >> 8) & 255;
    }
    if ( index ) {
      setup[4] = index & 255;
      setup[5] = (index >> 8) & 255;
    }
    if ( length ) {
      setup[6] = length & 255;
      setup[7] = (length >> 8) & 255;
    }
  }
  
  if ( njb_debug(DD_USBCTL) ) {
    fprintf(stderr, "%*sSetup: ", 3*__sub_depth, " ");
    data_dump(stderr, setup, 8);
  }
  
  if ( usb_control_msg(dev, type, request, value, index, data, length,
		       USBTIMEOUT) < 0 ) {
    njb_error_add_string (njb, "usb_control_msg", usb_strerror());
    return -1;
  }
  
  if ( njb_debug(DD_USBCTL) ) {
    if ( length ) {
      fprintf(stderr, "%s", ( (type & UT_READ) == UT_READ ) ?
	      "<<" : ">>");
      data_dump_ascii(stderr, data, length, 0);
      fprintf(stderr, "\n");
    }
  }
  
  return 0;
}


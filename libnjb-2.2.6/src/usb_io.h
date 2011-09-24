#ifndef __NJB__USB__IO__H
#define __NJB__USB__IO__H

#include "libnjb.h"

#define USBTIMEOUT 50000

#include <usb.h>

/*
 * Legacy #defines that map native *BSD USB #defines to those used by
 * libusb.  Some day we'll get rid of these.
 */

#ifndef UT_WRITE
#define UT_WRITE	USB_ENDPOINT_OUT
#endif

#ifndef UT_READ
#define UT_READ		USB_ENDPOINT_IN
#endif

#ifndef UT_CLASS
#define UT_CLASS	USB_TYPE_CLASS
#endif

#ifndef UT_STANDARD
#define UT_STANDARD	USB_TYPE_STANDARD
#endif

#ifndef UT_WRITE_VENDOR_OTHER
#define UT_WRITE_VENDOR_OTHER (UT_WRITE | USB_TYPE_VENDOR | USB_RECIP_OTHER )
#endif

#ifndef UT_READ_VENDOR_OTHER
#define UT_READ_VENDOR_OTHER (UT_READ | USB_TYPE_VENDOR | USB_RECIP_OTHER )
#endif

ssize_t usb_pipe_read (njb_t *njb, void *buf, size_t nbytes);
ssize_t usb_pipe_write (njb_t *njb, void *buf, size_t nbytes);
int usb_setup (njb_t *njb, int type, int request, int value,
	int index, int length, void *data);

#endif

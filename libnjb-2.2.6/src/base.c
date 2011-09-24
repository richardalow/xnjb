/**
 * \file base.c
 *
 * This file contains the basic stuff for opening the device
 * on the USB bus and similar things. Here are the jukebox detection
 * algorithms for example.
 */

#include "libnjb.h"
#include "njb_error.h"
#include "defs.h"
#include "base.h"
#include "protocol.h"
#include "protocol3.h"
#include "usb_io.h"

typedef struct njb_device_entry njb_device_entry_t;
struct njb_device_entry {
  char *name;
  u_int16_t vendor_id;
  u_int16_t product_id;
  u_int8_t protocol;
  u_int8_t usb20;
  int njblib_id;
};

/**
 * A table of USB identifiers and names for all the Creative
 * devices. This table MUST be kept in exactly the same order
 * as the device list in libnjb.h.in, since the njb->device_type
 * field may be used as an index into this table.
 *
 * MTP devices currently unsupported:
 * 0x041e:0x4123: Creative Portable Media Center
 * 0x041e:0x4130: Creative Zen Micro (MTP mode)
 * 0x041e:0x4131: Creative Zen Touch (MTP mode)
 * 0x041e:0x4137: Creative Zen Sleek (MTP mode)
 * 0x041e:0x413c: Creative Zen MicroPhoto
 * 0x041e:0x412f: Third generation Dell DJ
 */
static njb_device_entry_t njb_device_table[] = {
  { "NOMAD Jukebox (1)", 0x0471, 0x0222, NJB_PROTOCOL_OASIS, 0, NJB_DEVICE_NJB1 },
  { "NOMAD Jukebox 2", 0x041e, 0x4100, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_NJB2 },
  { "NOMAD Jukebox 3", 0x041e, 0x4101, NJB_PROTOCOL_PDE, 0, NJB_DEVICE_NJB3 },
  { "NOMAD Jukebox Zen", 0x041e, 0x4108, NJB_PROTOCOL_PDE, 0, NJB_DEVICE_NJBZEN },
  { "NOMAD Jukebox Zen USB 2.0", 0x041e, 0x410b, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_NJBZEN2 },
  { "NOMAD Jukebox Zen NX", 0x041e, 0x4109, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_NJBZENNX },
  { "NOMAD Jukebox Zen Xtra", 0x041e, 0x4110, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_NJBZENXTRA },
  { "Dell Digital Jukebox", 0x041e, 0x4111, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_DELLDJ },
  { "Creative Zen Touch", 0x041e, 0x411b, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_NJBZENTOUCH },
  { "Creative Zen Micro", 0x041e, 0x411e, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_NJBZENMICRO },
  { "Second Generation Dell DJ", 0x041e, 0x4126, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_DELLDJ2 },
  { "Dell Pocket DJ", 0x041e, 0x4127, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_POCKETDJ },
  { "Creative Zen Sleek", 0x041e, 0x4136, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_ZENSLEEK },
  { "Creative Zen (Micro variant)", 0x041e, 0x411d, NJB_PROTOCOL_PDE, 1, NJB_DEVICE_CREATIVEZEN }
};
static const int njb_device_table_size = sizeof(njb_device_table) / sizeof(njb_device_entry_t);

/** The current debug flags for all if libnjb (global) */
int njb_debug_flags = 0;
/** The current subroutine depth for all of libnjb (global) */
int __sub_depth = 0;

#ifdef __NetBSD__
#define MAXDEVNAMES USB_MAX_DEVNAMES
#endif

/**
 * Search the USB bus for a Nomad JukeBox.  We can handle up to
 * NJB_MAX_DEVICES JukeBox's per USB simultaneously (a value arbitrarily
 * set in base.h, because it's easier to work with a fixed size array than
 * build dynamically growing code...if anyone _really_ plans on having
 * more than NJB_MAX_DEVICES jukeboxes on their system, they can recompile
 * with a bigger number).  The USB device starts at usb0 and goes to usbNNN
 * where NNN is USB_MAX_DEVICES.  Check each bus for a usb device.  Return
 * -1 if errors occurred...though this doesn't mean we failed to find
 * devices.  *count gives the number of NJB devices that were found.
 * Store the resulting NJB structues structure into the njbs[NJB_MAX_DEVICES]
 * array.
 *
 * @param njbs an array of jukeboxes to fill up. The array must be
 *             NJB_MAX_DEVICES large.
 * @param limit a variable that is supposed to limit the number of
 *              devices retrieved, currently not used.
 * @param count a pointer to a variable that will hold the number of
 *              devices found after the call to this function.
 * @return 0 on OK, -1 on failure.
 */
int njb_discover (njb_t *njbs, int limit, int *count)
{
  __dsub= "njb_discover";
  struct usb_bus *busses;
  struct usb_bus *bus;
  struct usb_device *device;
  int found;
  
  __enter;
  
  /* Number of found devices */
  found = 0;
  /* Set return value to "no devices" by default */
  *count = 0;

  /* Initialize libusb library */
  usb_init();
  
  /* Try to locate busses and devices */
  usb_find_busses();
  usb_find_devices();
  busses = usb_get_busses();
  
  bus = busses;
  while ( bus != NULL ) {
    device = bus->devices;
    while ( device != NULL ) {
      int i;
      
      for (i = 0; i < njb_device_table_size; i++) {
	njb_device_entry_t *njb_device = &njb_device_table[i];
	
	if (device->descriptor.idVendor == njb_device->vendor_id &&
	    device->descriptor.idProduct == njb_device->product_id) {
	  njbs[found].device = device;
	  njbs[found].dev = NULL;
	  njbs[found].device_type = njb_device->njblib_id;
	  found ++;
	  break;
	}
      }
      device = device->next;
    }
    bus = bus->next;
  }
  
  /* The number of devices we have found */
  *count = found;
  
  __leave;
  return 0;
}

/**
 * Close a specific njb for reading and writing.  
 *
 * @param njb the jukebox object to close
 */
void njb_close (njb_t *njb)
{
  __dsub= "njb_close";
  __enter;
  
  usb_release_interface(njb->dev, njb->usb_interface);
  
  /*
   * Resetting the USB bus is not popular amongst
   * NJB2/3/ZEN devices, and will just be made for
   * NJB1.
   */
  if (njb->device_type == NJB_DEVICE_NJB1) {
    usb_resetep(njb->dev, njb->usb_bulk_out_ep);
    usb_reset(njb->dev);
  }
  
  usb_close(njb->dev);
  
  __leave;
}

/**
 * Go through the USB descriptor block for the device and try to
 * locate endpoints and interfaces to use for communicating with
 * the device.
 *
 * @param njb the jukebox object associated with the USB device 
 *            to parse
 */
static void parse_usb_descriptor(njb_t *njb)
{
  int i, j, k, l;
  int found_interface = 0;
  int found_in_ep = 0;
  int found_out_ep = 0;
  u_int8_t config = 0x00;
  u_int8_t interface = 0x00;
  u_int8_t in_ep = 0x00;
  u_int8_t out_ep = 0x00;

  if (njb->device_type == NJB_DEVICE_NJB1) {
    njb->usb_config = 0x01; /* The others have 0x00 mostly */
    njb->usb_interface = 0x00;
    njb->usb_bulk_out_ep = 0x02;
    njb->usb_bulk_in_ep = 0x82;
  } else {
    /* Print descriptor information */
    if (njb_debug(0x07)) {
      printf("The device has %d configurations.\n", njb->device->descriptor.bNumConfigurations);
    }
    i = 0;
    while (!found_interface && i < njb->device->descriptor.bNumConfigurations) {
      struct usb_config_descriptor *conf = &njb->device->config[i];
      if (njb_debug(0x07)) {
	printf("Configuration %d, value %d, has %d interfaces.\n", i, conf->bConfigurationValue, conf->bNumInterfaces);
      }
      j = 0;
      while (!found_interface && j < conf->bNumInterfaces) {
	struct usb_interface *iface = &conf->interface[j];
	if (njb_debug(0x07)) {
	  printf("  Interface %d, has %d altsettings.\n", j, iface->num_altsetting);
	}
	k = 0;
	while (!found_interface && k < iface->num_altsetting) {
	  struct usb_interface_descriptor *ifdesc = &iface->altsetting[k];
	  if (njb_debug(0x07)) {
	    printf("    Altsetting %d, number %d, has %d endpoints.\n", k, ifdesc->bInterfaceNumber, ifdesc->bNumEndpoints);
	  }
	  found_in_ep = 0;
	  found_out_ep = 0;
	  for (l = 0; l < ifdesc->bNumEndpoints; l++) {
	    struct usb_endpoint_descriptor *ep = &ifdesc->endpoint[l];
	    if (njb_debug(0x07)) {
	      printf("    Endpoint %d, no %02xh, attributes %02xh\n", l, ep->bEndpointAddress, ep->bmAttributes);
	    }
	    if (!found_out_ep && (ep->bEndpointAddress & 0x80) == 0x00) {
	      if (njb_debug(0x07)) {
		printf("    Found WRITE (OUT) endpoint %02xh\n", ep->bEndpointAddress);
	      }
	      found_out_ep = 1;
	      out_ep = ep->bEndpointAddress;
	    }
	    if (!found_in_ep && (ep->bEndpointAddress & 0x80) != 0x00) {
	      if (njb_debug(0x07)) {
		printf("    Found READ (IN) endpoint %02xh\n", ep->bEndpointAddress);
	      }
	      found_in_ep = 1;
	      in_ep = ep->bEndpointAddress;
	    }
	  }
	  if (found_in_ep == 1 && found_out_ep == 1) {
	    found_interface = 1;
	    interface = ifdesc->bInterfaceNumber;
	    config = conf->bConfigurationValue;
	  }
	  k++;
	}
	j++;
      }
      i++;
    }
    if (found_interface) {
      if (njb_debug(0x07)) {
	printf("Found config %d, interface %d, IN EP: %02xh, OUT EP: %02xh\n", config, interface, in_ep, out_ep);
      }
      njb->usb_config = config;
      njb->usb_interface = interface;
      njb->usb_bulk_out_ep = out_ep;
      njb->usb_bulk_in_ep = in_ep;
    } else {
      /*
       * This is some code that should never need to run!
       */
      printf("LIBNJB panic: could not locate a suitable interface.\n");
      printf("LIBNJB panic: resorting to heuristic interface choice.\n");
      njb->usb_config = 0;
      njb->usb_interface = 0;
      if (njb_device_is_usb20(njb)) {
	if (njb->device_type == NJB_DEVICE_NJBZENMICRO) {
	  njb->usb_bulk_out_ep = 0x02; /* NJB Zen Micro use endpoint 2 OUT */
	}
	/* The other USB 2.0 jukeboxes use endpoint 1 OUT */
	njb->usb_bulk_out_ep = 0x01;
      } else {
	/*
	 * The original NJB1, NJB3 and the NJB Zen FW-edition,
	 * i.e. all USB 1.1 devices use endpoint 2 OUT
	 */
	njb->usb_bulk_out_ep = 0x02;
      }
      /* Default for all devices */
      njb->usb_bulk_in_ep = 0x82;
    }
  }
}

/**
 * Open a specific njb for reading and writing.  
 *
 * @param njb the jukebox object to open
 * @return 0 on success, -1 on failure
 */
int njb_open (njb_t *njb)
{
  __dsub= "njb_open";
  __enter;
  

  /* Initialize error stack so we can store error messages */
  initialize_errorstack(njb);
  
  /* Check what config, interface and endpoints to use */
  parse_usb_descriptor(njb);
  
  if ( (njb->dev = usb_open(njb->device)) == NULL ) {
    njb_error_add(njb, "usb_open", -1);
    __leave;
    return -1;
  }
  
  /*
   * The "high speed" devices (USB 2.0) have two configurations.
   * the second one may be for operating the device under "full speed"
   * instead, so that it becomes slower.
   */
  if ( usb_set_configuration(njb->dev, njb->usb_config) ) {
    njb_error_add(njb, "usb_set_configuration", -1);
    __leave;
    return -1;
  }
  
  /* With several jukeboxes connected, a call will often fail
   * here when you try to connect to the second jukebox after
   * closing the first. Why? */
  if ( usb_claim_interface(njb->dev, njb->usb_interface) ) {
    njb_error_add(njb, "usb_claim_interface", -1);
    __leave;
    return -1;
  }
  
  
  /*
   * This should not be needed. Removing, cause it 
   * caused problems on MacOS X.
   */
  /*
    if ( usb_set_altinterface(njb->dev, 0) ) {
    njb_error_add(njb, "usb_set_altinterface", -1);
    __leave;
    return -1;
    }
  */
  
  __leave;
  return 0;
}

/**
 * Set the debug flags to use.
 *
 * @param flags the flags to set
 */
void njb_set_debug (int flags)
{
  njb_debug_flags = flags;
}

/**
 * get the current debug flags
 *
 * @param flags a binary mask that remove some of the
 *              flags.
 */
int njb_debug (int flags)
{
  return njb_debug_flags & flags;
}

/**
 * Get a device name from its USB characteristics.
 */
char *njb_get_usb_device_name(njb_t *njb) {
  char *name;
  static char njbunknownstr[] = "Unknown device";
  
  if (njb->device_type < njb_device_table_size) {
    njb_device_entry_t *device = &njb_device_table[njb->device_type];
    
    name = device->name;
  } else {
    name = njbunknownstr;
  }
  return name;
}

/**
 * Tell if a device is USB 2.0 or not
 * @return 1 if the device is USB 2.0, 0 if it is USB 1.1
 */
int njb_device_is_usb20(njb_t *njb) {
  njb_device_entry_t *device = &njb_device_table[njb->device_type];

  return device->usb20;
}

/**
 * Tell which protocol version a device is using.
 */
njb_protocol_type_t njb_get_device_protocol(njb_t *njb) {
  njb_device_entry_t *device = &njb_device_table[njb->device_type];
  
  return device->protocol;
}

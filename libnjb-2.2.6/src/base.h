#ifndef __NJB__BASE__H
#define __NJB__BASE__H

#include "libnjb.h"

/**
 * Protocol type defines
 */
#define NJB_PROTOCOL_OASIS 0
#define NJB_PROTOCOL_PDE 1
typedef u_int8_t njb_protocol_type_t;

int njb_discover (njb_t *njbs, int limit, int *errorflag);
void njb_device_dump (njb_t *njb, FILE *fp);
int njb_open (njb_t *njb);
void njb_close (njb_t *njb);
void njb_set_debug (int flags);
int njb_debug (int flag);
char *njb_get_usb_device_name(njb_t *njb);
int njb_device_is_usb20(njb_t *njb);
njb_protocol_type_t njb_get_device_protocol(njb_t *njb);
#define PDE_PROTOCOL_DEVICE(t) \
   (njb_get_device_protocol(t) == NJB_PROTOCOL_PDE)

#endif /* __NJB__BASE__H */

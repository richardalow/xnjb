/* EAX functions */
#ifndef __NJB__EAX__H
#define __NJB__EAX__H

#include "libnjb.h"
#include "protocol.h"

int eax_unpack(void *data, size_t nbytes, njb_state_t *state);
njb_eax_t *new_eax_type(void);
void destroy_eax_type(njb_eax_t *eax);

#endif

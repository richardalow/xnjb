#ifndef __NJB__TIME__H
#define __NJB__TIME__H

njb_time_t *time_unpack(void *data);
njb_time_t *time_unpack3(void *data);
void *time_pack(njb_time_t *time);
void *time_pack3(njb_time_t *time);

#endif

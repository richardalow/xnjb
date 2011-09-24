#ifndef __NJB__SONGID__H
#define __NJB__SONGID__H

#define ID_DATA_ASCII	0
#define ID_DATA_BIN	1
#define ID_DATA_UNI     2

/* Eventually we move all private songid_t processing in here */

njb_songid_t *songid_unpack (void *data, size_t nbytes);
unsigned char *songid_pack (njb_songid_t *song, u_int32_t *size);
unsigned char *songid_pack3 (njb_songid_t *song, u_int32_t *size);
int songid_sanity_check(njb_t *njb, njb_songid_t *songid);

#endif

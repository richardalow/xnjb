#ifndef __NJB__PLAYLIST__H
#define __NJB__PLAYLIST__H

njb_playlist_t *playlist_unpack(void *data, size_t nbytes);
u_int32_t playlist_pack(njb_playlist_t *pl, char *data);

#endif

#ifndef __NJB__USB__BYTEORDER__H
#define __NJB__USB__BYTEORDER__H

u_int64_t njb1_bytes_to_64bit(unsigned char *dp);
void from_64bit_to_njb1_bytes(u_int64_t val, unsigned char *dp);
u_int32_t njb1_bytes_to_32bit(unsigned char *dp);
u_int32_t njb3_bytes_to_32bit(unsigned char *dp);
void from_32bit_to_njb1_bytes(u_int32_t val, unsigned char *dp);
void from_32bit_to_njb3_bytes(u_int32_t val, unsigned char *dp);
u_int16_t njb1_bytes_to_16bit(unsigned char *dp);
u_int16_t njb3_bytes_to_16bit(unsigned char *dp);
void from_16bit_to_njb1_bytes(u_int16_t val, unsigned char *dp);
void from_16bit_to_njb3_bytes(u_int16_t val, unsigned char *dp);
u_int16_t get_msw (u_int32_t word);
u_int16_t get_lsw (u_int32_t word);
u_int64_t make64 (u_int32_t msdw, u_int32_t lsdw);
void split64 (u_int64_t num, u_int32_t *msdw, u_int32_t *lsdw);

#endif

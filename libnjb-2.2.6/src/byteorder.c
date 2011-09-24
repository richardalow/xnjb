/**
 * \file byteorder.c
 *
 * We want libnjb to be "endianness agnostic" i.e. the byte-ordering
 * of the libnjb host platform shall not affect its functionality.
 * These routines are written using shifting and byte operations that
 * will produce the same result regardless of whether the host
 * platform is little-endian, big-endian or even mixed-endian.
 *
 * NJB1 and the "series 3 family" (NJB2, NJB3, NJB Zen, NJB Zen 2.0) have
 * different byte ordering. NJB1 is essentially big-endian, and the 
 * series 3 family little-endian. The terminology could be confusing, 
 * so we refer to the different endiannesses as "njb1-endian" and 
 * "njb3-endian".
 */
#include "libnjb.h"
#include "byteorder.h"

/**
 * This function will take 8 bytes from the njb1-endian byte array 
 * pointed to by <code>*dp</code> and transform it to a 
 * <code>u_int64_t</code> on the host platform.
 * 
 * @param dp a pointer to the 8 raw bytes in NJB1 endianness to convert
 * @return an unsigned 64 bit integer
 */
u_int64_t njb1_bytes_to_64bit(unsigned char *dp)
{
  u_int64_t ret;

  ret = ((u_int64_t) dp[3] << 56);
  ret = ret | ((u_int64_t) dp[2] << 48);
  ret = ret | ((u_int64_t) dp[1] << 40);
  ret = ret | ((u_int64_t) dp[0] << 32);
  ret = ret | ((u_int64_t) dp[7] << 24);
  ret = ret | ((u_int64_t) dp[6] << 16);
  ret = ret | ((u_int64_t) dp[5] << 8);
  ret = ret | (u_int64_t) dp[4];
  return ret;
}

/**
 * This function will write the <code>u_int64_t</code> of the 
 * host platform, <code>val</code> as NJB1-endian bytes 
 * beginning at the first byte in the byte array pointed to by 
 * <code>*dp</code>.
 *
 * @param val the unsigned 64 bit integer to convert to bytes
 * @param dp a pointer to the byte array (of atleast 8 bytes)
 *           that shall hold the resulting bytes
 */
void from_64bit_to_njb1_bytes(u_int64_t val, unsigned char *dp)
{
  dp[0] = (val >> 32) & 255;
  dp[1] = (val >> 40) & 255;
  dp[2] = (val >> 48) & 255;
  dp[3] = (val >> 56) & 255;
  dp[4] = val & 255;
  dp[5] = (val >> 8) & 255;
  dp[6] = (val >> 16) & 255;
  dp[7] = (val >> 24) & 255;
}

/**
 * This function will take 4 bytes from the NJB1-endian byte array 
 * pointed to by <code>*dp</code> and transform it to a 
 * <code>u_int32_t</code> on the host platform.
 *
 * @param dp a pointer to the 4 raw bytes in NJB1 endianness to convert
 * @return an unsigned 32 bit integer
 */
u_int32_t njb1_bytes_to_32bit(unsigned char *dp)
{
  u_int32_t ret;

  ret = ((u_int32_t) dp[3] << 24);
  ret = ret | ((u_int32_t) dp[2] << 16);
  ret = ret | ((u_int32_t) dp[1] << 8);
  ret = ret | (u_int32_t) dp[0];  
  return ret;
}

/**
 * This function will take 4 bytes from the series 3-endian byte array 
 * pointed to by <code>*dp</code> and transform it to a 
 * <code>u_int32_t</code> on the host platform.
 *
 * @param dp a pointer to the 4 raw bytes in series 3 endianness to convert
 * @return an unsigned 32 bit integer
 */
u_int32_t njb3_bytes_to_32bit(unsigned char *dp)
{
  u_int32_t ret;

  ret = ((u_int32_t) dp[0] << 24);
  ret = ret | ((u_int32_t) dp[1] << 16);
  ret = ret | ((u_int32_t) dp[2] << 8);
  ret = ret | (u_int32_t) dp[3];  
  return ret;
}


/**
 * This function will write the <code>u_int32_t</code> of the host 
 * platform, <code>val</code> as 4 NJB1-endian bytes beginning at 
 * the first byte in the byte array pointed to by <code>*dp</code>.
 *
 * @param val the unsigned 32 bit integer to convert to bytes
 * @param dp a pointer to the byte array (of atleast 4 bytes)
 *           that shall hold the resulting bytes
 */
void from_32bit_to_njb1_bytes(u_int32_t val, unsigned char *dp)
{
  dp[0] = val & 255;
  dp[1] = (val >> 8) & 255;
  dp[2] = (val >> 16) & 255;
  dp[3] = (val >> 24) & 255;
}

/**
 * This function will write the <code>u_int32_t</code> of the host 
 * platform, <code>val</code> as 4 series 3-endian bytes beginning at 
 * the first byte in the byte array pointed to by <code>*dp</code>.
 *
 * @param val the unsigned 32 bit integer to convert to bytes
 * @param dp a pointer to the byte array (of atleast 4 bytes)
 *           that shall hold the resulting bytes
 */
void from_32bit_to_njb3_bytes(u_int32_t val, unsigned char *dp)
{
  dp[0] = (val >> 24) & 255;
  dp[1] = (val >> 16) & 255;
  dp[2] = (val >> 8) & 255;
  dp[3] = val & 255;
}

/**
 * This function will take 2 bytes from the NJB1-endian byte array 
 * pointed to by <code>*dp</code> and transform it to 
 * <code>a u_int16_t</code> unsigned 16 bit integer on the host
 * platform.
 *
 * @param dp a pointer to the 2 raw bytes in NJB1 endianness to convert
 * @return an unsigned 16 bit integer
 */
u_int16_t njb1_bytes_to_16bit(unsigned char *dp)
{
  u_int16_t ret;

  ret = ((u_int16_t) dp[1] << 8);
  ret = ret | (u_int16_t) dp[0];
  return ret;
}

/**
 * This function will take 2 bytes from the series 3-endian byte array 
 * pointed to by <code>*dp</code> and transform it to 
 * <code>a u_int16_t</code> unsigned 16 bit integer on the host
 * platform.
 *
 * @param dp a pointer to the 2 raw bytes in series 3 endianness to convert
 * @return an unsigned 16 bit integer
 */
u_int16_t njb3_bytes_to_16bit(unsigned char *dp)
{
  u_int16_t ret;

  ret = ((u_int16_t) dp[0] << 8);
  ret = ret | (u_int16_t) dp[1];
  return ret;
}


/**
 * This function will write the <code>u_int16_t</code> of the host 
 * platform, <code>val</code> as 2 NJB1-endian bytes beginning at the 
 * first byte in the byte array pointed to by <code>*dp</code>.
 *
 * @param val the unsigned 16 bit integer to convert to bytes
 * @param dp a pointer to the byte array (of atleast 2 bytes)
 *           that shall hold the resulting bytes
 */
void from_16bit_to_njb1_bytes(u_int16_t val, unsigned char *dp)
{
  dp[0] = val & 255;
  dp[1] = (val >> 8) & 255;
}

/**
 * This function will write the <code>u_int16_t</code> of the host 
 * platform, <code>val</code> as 2 series 3-endian bytes beginning at the 
 * first byte in the byte array pointed to by <code>*dp</code>.
 *
 * @param val the unsigned 16 bit integer to convert to bytes
 * @param dp a pointer to the byte array (of atleast 2 bytes)
 *           that shall hold the resulting bytes
 */
void from_16bit_to_njb3_bytes(u_int16_t val, unsigned char *dp)
{
  dp[0] = (val >> 8) & 255;
  dp[1] = val & 255;
}

/**
 * This simply extract the most significant 16 bit parts of
 * a 32 bit word.
 *
 * @param word the 32 bit word to get the most significant 16 bits
 *             for
 * @return the most significant 16 bits as a 16 bit unsigned integer
 */
u_int16_t get_msw (u_int32_t word)
{
  return (u_int16_t) (word >> 16) & 0xffff;
}

/**
 * This simply extract the least significant 16 bit parts of
 * a 32 bit word.
 *
 * @param word the 32 bit word to get the least significant 16 bits
 *             for
 * @return the least significant 16 bits as a 16 bit unsigned integer
 */
u_int16_t get_lsw (u_int32_t word)
{
  return (u_int16_t) word & 0xffff;
}

/**
 * Create a 64 bit unsigned integer from two 32 bit integers
 * representing the most/least significant part of it.
 *
 * @param msdw the most significant 32 bits
 * @param lsdw the least significant 32 bits
 * @return a 64 bit unsigned integer made by joining the two 
 *         parts
 */
u_int64_t make64 (u_int32_t msdw, u_int32_t lsdw)
{
	u_int64_t val;

	val= msdw * 0x100000000ULL + lsdw;
	return val;
}

/**
 * Split a 64 bit unsigned integer into two unsigned 32 bit
 * integer representing the most/least significant 32 bits
 * of the incoming 64 bit integer.
 *
 * @param num the 64 bit integer to split
 * @param msdw a pointer to the 32 bit integer that shall hold
 *             the most significant 32 bits of the 64 bit integer
 * @param lsdw a pointer to the 32 bit integer that shall hold
 *             the least significant 32 bits of the 64 bit integer
 */
void split64 (u_int64_t num, u_int32_t *msdw, u_int32_t *lsdw)
{
	u_int64_t val= num/0x100000000ULL;

	*msdw= (u_int32_t) val;
	*lsdw= (u_int32_t) ( num - val*0x100000000ULL );
}

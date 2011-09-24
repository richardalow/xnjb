/**
 * \file njbtime.c
 *
 * This file deals with the time structures used by the internal clock
 * of the devices.
 */

#include <stdlib.h>
#include <string.h>
#include "libnjb.h"
#include "njbtime.h"
#include "njb_error.h"
#include "defs.h"
#include "base.h"
#include "byteorder.h"

extern int __sub_depth;

/**
 * Unpacks a raw NJB1 time structure into libnjb representation.
 * 
 * @param data raw memory representing a NJB1 timestamp
 * @return a libnjb time structure
 */
njb_time_t *time_unpack(void *data)
{
  __dsub = "time_unpack";
  njb_time_t *time;
  unsigned char *dp= (unsigned char *) data;
  
  __enter;
  
  time = malloc(sizeof(njb_time_t));
  if ( time == NULL ) {
    __leave;
    return NULL;
  }

  memset(time, 0, sizeof(njb_time_t));
  time->year = njb1_bytes_to_16bit(&dp[0]);
  time->month = njb1_bytes_to_16bit(&dp[2]);
  time->day = njb1_bytes_to_16bit(&dp[4]);
  time->weekday = njb1_bytes_to_16bit(&dp[6]);
  time->hours = njb1_bytes_to_16bit(&dp[8]);
  time->minutes = njb1_bytes_to_16bit(&dp[10]);
  time->seconds = njb1_bytes_to_16bit(&dp[12]);
  
  /* The usage of the last two bytes 
   * (which always seem to be 0x0000)
   * is unknown
   */
  __leave;
  return time;
}

/**
 * Unpacks a raw series 3 time structure into libnjb representation.
 * 
 * @param data raw memory representing a series 3 timestamp
 * @return a libnjb time structure
 */
njb_time_t *time_unpack3(void *data)
{
  __dsub = "time_unpack3";
  njb_time_t *time;
  unsigned char *dp = (unsigned char *) data;

  __enter;
  
  time= malloc(sizeof(njb_time_t));
  if ( time == NULL ) {
    __leave;
    return NULL;
  }
  
  memset(time, 0, sizeof(njb_time_t));
  time->year = 1000*(dp[9]>>4) + 100*(dp[9]&15) + 10 * (dp[10]>>4) + (dp[10]&15);
  time->month = 10*(dp[8]>>4) + (dp[8]&15);
  time->day = 10*(dp[7]>>4) + (dp[7]&15);
  time->weekday = 10*(dp[6]>>4) + (dp[6]&15);
  time->hours = 10*(dp[11]>>4) + (dp[11]&15);
  time->minutes = 10*(dp[12]>>4) + (dp[12]&15);
  time->seconds = 10*(dp[13]>>4) + (dp[13]&15);
  /* The usage of the last two bytes 
   * (which always seem to be 0x0000)
   * is unknown
   */
  __leave;
  return time;
}

/**
 * Packs a libnjb time structure to the raw format used by the NJB1
 *
 * @param time the libnjb time structure to pack
 * @return a pointer to raw bytes representing the NJB1 time structure
 *         in a newly allocated byte array. The caller shall free this
 *         memory after use.
 */
void *time_pack(njb_time_t *time)
{
  __dsub = "time_pack";
  unsigned char *data;
  
  __enter;
  
  data= (unsigned char *) malloc(16);
  if ( data == NULL ) {
    __leave;
    return NULL;
  }
  
  memset(data, 0, 16);
  from_16bit_to_njb1_bytes(time->year, &data[0]);
  from_16bit_to_njb1_bytes(time->month, &data[2]);
  from_16bit_to_njb1_bytes(time->day, &data[4]);
  from_16bit_to_njb1_bytes(time->weekday, &data[6]);
  from_16bit_to_njb1_bytes(time->hours, &data[8]);
  from_16bit_to_njb1_bytes(time->minutes, &data[10]);
  from_16bit_to_njb1_bytes(time->seconds, &data[12]);
  
  __leave;
  return data;
}

/**
 * Packs a libnjb time structure to the raw format used by the series 3
 * devices.
 *
 * @param time the libnjb time structure to pack
 * @return a pointer to raw bytes representing the series 3 time structure
 *         in a newly allocated byte array. The caller shall free this
 *         memory after use.
 */
void *time_pack3(njb_time_t *time)
{
  __dsub= "time_pack3";
  unsigned char *data;
  //       	unsigned char njb3_set_time[]={0x00,0x07,0x00,0x01,0x00,0x0a,0x01,0x10,0x02,0x31,0x12,0x20,0x02,0x10,0x12,0x00,0x00,0x00};
  unsigned char njb3_set_time_command[]={0x00,0x07,0x00,0x01,0x00,0x0a,0x01,0x10,
					 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  
  __enter;
  
  data= (unsigned char *) malloc(18);
  if ( data == NULL ) {
    __leave;
    return NULL;
  }
  
  memcpy(data, njb3_set_time_command, 18);
  
  /* BCD encoded time */
  data[8]= time->weekday;
  data[9]= time->day % 10 + 16*(time->day / 10);
  data[10]= time->month % 10 + 16*(time->month / 10);
  data[11]= (time->year / 100) % 10 + 16*( (time->year / 100) / 10);
  data[12]= (time->year % 100) % 10 + 16*( (time->year % 100) / 10);
  data[13]= time->hours % 10 + 16*(time->hours / 10);
  data[14]= time->minutes % 10 + 16*(time->minutes / 10);
  data[15]= time->seconds % 10 + 16*(time->seconds / 10);
  
  __leave;
  return data;
}

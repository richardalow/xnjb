#ifndef __NJB__ERROR__H
#define __NJB__ERROR__H

#include <stdio.h>
#include "libnjb.h"

/** 
 * @defgroup errors Error codes
 * @{
 */
#define EO_USBCTL       1	/**< I/O failure on USB control pipe	*/
#define EO_USBBLK       2	/**< I/O failure on USB data pipe	*/
#define EO_RDSHORT      3	/**< short read on USB data pipe	*/
#define EO_NOMEM        4	/**< out of memory			*/
#define EO_BADDATA      5	/**< invalid data			*/
#define EO_EOM          6	/**< end of data			*/
#define EO_BADSTATUS    7	/**< bad status from Jukebox		*/
#define EO_BADNJBID     8	/**< Jukebox ID has changed		*/
#define EO_BADCOUNT     9	/**< library count mismatch		*/
#define EO_WRSHORT      10	/**< short write on USB data pipe	*/
#define EO_NULLTMP      11	/**< temporary transfer dir not defined	*/
#define EO_TOOBIG       12	/**< block size too big			*/
#define EO_CANTMOVE     13	/**< can't move file to destination	*/
#define EO_TIMEOUT      14	/**< operation timed out		*/
#define EO_ABORTED      15	/**< operation aborted			*/
#define EO_EOF          16	/**< received EOF			*/
#define EO_DEVICE       17	/**< can't open device for read/write	*/
#define EO_INIT         18	/**< can't initialize device		*/
#define EO_TMPFILE      19	/**< can't create temporary file	*/
#define EO_XFERDENIED   20	/**< transfer request denied		*/
#define EO_WRFILE       21	/**< error writing output file		*/
#define EO_XFERERROR    22	/**< bad transfer completion status	*/
#define EO_SRCFILE      23	/**< can't read source file		*/
#define EO_INVALID	24	/**< invalid arguments			*/
#define EO_AGAIN	25	/**< resource temporarily unavailable	*/
#define EO_BAD_NJB1_REPLACE 26  /**< too little info to replace tag on NJB1 */
/** @} */

#define MAX_ERRLEN	128
#define MAX_ERRORS	16

typedef struct njb_error_stack_struct njb_error_stack_t; /**< See struct definition */
/**
 * This struct holds an error stack for each NJB object.
 */
struct njb_error_stack_struct {
  int idx; /**< Current index into the error stack */
  int count; /**< Number of errors currently on the stack */
  char **msg; /**< An array of error message strings */
};

void initialize_errorstack(njb_t *njb);
void destroy_errorstack(njb_t *njb);
void njb_error_add (njb_t *njb, const char *sub, int err);
void njb_error_add2 (njb_t *njb, const char *sub, const char *prefix, int err);
void njb_error_add3 (njb_t *njb, const char *sub, const char *prefix, const 
	char *suffix, int err);
void njb_error_add_string (njb_t *njb, const char *sub, const char* error);
void njb_error_clear (njb_t *njb);

#define NJB_ERROR(a,b) njb_error_add(a,subroutinename,b)
#define NJB_ERROR2(a,b,c) njb_error_add2(a,subroutinename,b,c)
#define NJB_ERROR3(a,b,c,d) njb_error_add3(a,subroutinename,b,c,d)

#endif


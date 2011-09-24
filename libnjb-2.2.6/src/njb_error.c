/**
 * file njb_error.c
 *
 * Functions dealing with generic error handling.
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "libnjb.h"
#include "base.h"
#include "defs.h"
#include "protocol.h"
#include "njb_error.h"

extern int __sub_depth;
const char *njb_status_string (unsigned char code);

/**
 * This function returns an error code as a string.
 * 
 * @param code the error code as a number
 * @return a string representing the error code
 */
static const char *njb_error_string (int code)
{
  switch (code)
    {
    case -1:
      return strerror(errno);
    case 0:
      return "";
    case EO_USBCTL:
      return "I/O failure on USB control pipe";
    case EO_USBBLK:
      return "I/O failure on USB data pipe";
    case EO_RDSHORT:
      return "short read on USB data pipe";
    case EO_NOMEM:
      return "out of memory";
    case EO_BADDATA:
      return "invalid data";
    case EO_EOM:
      return "end of data";
    case EO_BADSTATUS:
      return "bad status from Jukebox";
    case EO_BADNJBID:
      return "Jukebox ID has changed";
    case EO_BADCOUNT:
      return "library count mismatch";
    case EO_WRSHORT:
      return "short write on USB data pipe";
    case EO_NULLTMP:
      return "temporary transfer dir not defined";
    case EO_TOOBIG:
      return "block size too big";
    case EO_CANTMOVE:
      return "can't move file to destination";
    case EO_TIMEOUT:
      return "operation timed out";
    case EO_ABORTED:
      return "operation aborted";
    case EO_EOF:
      return "received EOF";
    case EO_DEVICE:
      return "can't open device for read/write";
    case EO_INIT:
      return "can't initialize device";
    case EO_TMPFILE:
      return "can't create temporary file";
    case EO_XFERDENIED:
      return "transfer request denied";
    case EO_WRFILE:	
      return "error writing output file";
    case EO_XFERERROR:
      return "bad transfer completion status";
    case EO_SRCFILE:
      return "can't read source file";
    case EO_INVALID:
      return "invalid arguments";
    case EO_AGAIN:
      return "resource temporarily unavailable";
    case EO_BAD_NJB1_REPLACE:
      return "the NJB1 needs complete tag info when replacing tags";
    default:
      return "(undefined error)";
    }
}

/**
 * This allocates memory for the error stack
 */
void initialize_errorstack(njb_t *njb) {
  __dsub= "initialize_errorstack";
  njb_error_stack_t *estack;
  
  __enter;
  if (njb == NULL) {
    __leave;
    return;
  } else {
    njb->error_stack = (void *) malloc(sizeof(njb_error_stack_t));
    estack = (njb_error_stack_t *) njb->error_stack;
    estack->msg = (char **) malloc(MAX_ERRORS * sizeof(char *));
    estack->count = 0;
    estack->idx = 0;
    njb_error_clear(njb);
  }
  __leave;
}

/**
 * This destroys the error structure in memory
 */
void destroy_errorstack(njb_t *njb) {
  __dsub = "destroy_errorstack";
  njb_error_stack_t *estack;

  __enter;
  if (njb == NULL) {
    __leave;
    return;
  } else {
    estack = (njb_error_stack_t *) njb->error_stack;
    /* First make sure there are no dangling error strings */
    njb_error_clear(njb);
    free(estack->msg);
  }
  __leave;
}

/**
 * This indicates if the error stack has been overflowed.
 *
 * @param njb a pointer to the device object to check
 */
static int error_overflow (njb_t *njb)
{
  __dsub = "error_overflow";
  njb_error_stack_t *estack;
  __enter;

  if (njb == NULL) {
    __leave;
    return 1;
  } else {
    estack = (njb_error_stack_t *) njb->error_stack;
    if ( estack->count >= MAX_ERRORS ) {
      strcpy(estack->msg[MAX_ERRORS], "Error stack overflow");
      estack->count = MAX_ERRORS+1;
      __leave;
      return 1;
    }
  }
  __leave;
  return 0;
}

void njb_error_add3 (njb_t *njb, const char *sub, const char *prefix, const char
		     *suffix, int code)
{
  __dsub= "njb_error_add3";
  njb_error_stack_t *estack;
  char *ep;

  __enter;

  ep = (char *) malloc(MAX_ERRLEN);  
  
  if ( error_overflow(njb) ) {
    __leave;
    return;
  }
  
  snprintf(ep, MAX_ERRLEN, "%s: %s: %s %s", sub, prefix,
	   njb_error_string(code), suffix);

  /* Add to error stack */
  estack = (njb_error_stack_t *) njb->error_stack;
  estack->msg[estack->count] = ep;

  estack->count++;
  __leave;
}

void njb_error_add2 (njb_t *njb, const char *sub, const char *prefix, int code)
{
  __dsub= "njb_error_add2";
  njb_error_stack_t *estack;
  char *ep;

  __enter;

  ep = (char *) malloc(MAX_ERRLEN);

  if ( error_overflow(njb) ) {
    __leave;
    return;
  }
  
  snprintf(ep, MAX_ERRLEN, "%s: %s: %s", sub, prefix,
	   njb_error_string(code));

  /* Add to error stack */
  estack = (njb_error_stack_t *) njb->error_stack;
  estack->msg[estack->count] = ep;
  
  estack->count++;
  __leave;
}

void njb_error_add (njb_t *njb, const char *sub, int code)
{
  __dsub = "njb_error_add";
  njb_error_stack_t *estack;
  char *ep;
  
  __enter;
  ep = (char *) malloc(MAX_ERRLEN);

  if ( error_overflow(njb) ) {
    __leave;
    return;
  }
  
  snprintf(ep, MAX_ERRLEN, "%s: %s", sub, njb_error_string(code));
 
  /* Add to error stack */
  estack = (njb_error_stack_t *) njb->error_stack;
  estack->msg[estack->count] = ep;
 
  estack->count++;
  __leave;
}

void njb_error_add_string (njb_t *njb, const char *sub, const char *error)
{
  __dsub = "njb_error_add_string";
  njb_error_stack_t *estack;
  char *ep;

  __enter;
  ep = (char *) malloc(MAX_ERRLEN);

  if ( error_overflow(njb) ) {
    __leave;
    return;
  }
  
  snprintf(ep, MAX_ERRLEN, "%s: %s", sub, error);

  /* Add to error stack */
  estack = (njb_error_stack_t *) njb->error_stack;
  estack->msg[estack->count] = ep;
  
  estack->count++;
  __leave;
}


/**
 * This clears the internal memory stack for a device.
 *
 * @param njb a pointer to the device object to clear off errors
 */
void njb_error_clear (njb_t *njb)
{
  __dsub = "njb_error_clear";

  __enter;
  if (njb == NULL) {
    __leave;
    return;
  } else {
    int i;
    njb_error_stack_t *estack;

    estack = (njb_error_stack_t *) njb->error_stack;
    if (estack != NULL) {
      /* Free memory used by error strings */
      for (i = 0; i < estack->count; i++) {
	free(estack->msg[i]);
      }
      estack->count = 0;
      estack->idx = 0;
    }
  }
  __leave;
}

/**
 * This function tells wheter an error message is queued and pending
 * for the current device. If so, the error should be retrieved
 * using <code>NJB_Error_Geterror()</code> or dumped using
 * <code>NJB_Error_Dumperror()</code>.
 *
 * @param njb a pointer to the NJB object to use
 * @return 0 if there are no errors pending, 1 if there are errors pending
 * @see NJB_Error_Reset_Geterror()
 * @see NJB_Error_Geterror()
 * @see NJB_Error_Dump()
 */
int NJB_Error_Pending(njb_t *njb)
{
  __dsub = "NJB_Error_Pending";
  njb_error_stack_t *estack = (njb_error_stack_t *) njb->error_stack;

  __enter;
  if (estack->count > 0) {
    __leave;
    return 1;
  }
  __leave;
  return 0;
}

/**
 * This function resets the internal error stack, so that 
 * old errors do not remain on following calls to retrieve
 * the error.
 * 
 * Retrieve the errors if the function 
 * <code>NJB_Error_Pending()</code> indicate that there are
 * errors pending. Typical usage:
 *
 * <pre>
 * njb_t *njb;
 * char *errstr;
 *
 * if (NJB_Error_Pending(njb) {
 *    NJB_Error_Reset_Geterror(njb);
 *    while ( (errstr = NJB_Error_Geterror(njb)) != NULL )  {
 *          printf("%s\n", errstr);
 *    }
 * }
 * </pre>
 *
 * @param njb a pointer to the NJB object to use
 * @see NJB_Error_Pending()
 * @see NJB_Error_Geterror()
 */
void NJB_Error_Reset_Geterror(njb_t *njb)
{
  __dsub = "NJB_Error_Reset_Geterror";
  njb_error_stack_t *estack;

  __enter;
  if (njb != NULL) {
    estack = (njb_error_stack_t *) njb->error_stack;
    estack->idx = 0;
  }
  __leave;
}

/**
 * This function returns the topmost error on the error 
 * stack as a string. The result is statically allocated and MUST
 * NOT be freed by the calling application. This function should
 * be repeatedly called until no errors remain and the function
 * returns NULL. After retrieveing all error strings like this,
 * the error stack is cleared and none of the errors will be
 * returned again.
 *
 * @param njb a pointer to the NJB object to use
 * @return a string representing one error on the error stack
 *         or NULL.
 */
const char *NJB_Error_Geterror(njb_t *njb)
{
  __dsub = "NJB_Error_Geterror";
  njb_error_stack_t *estack;
  const char *cp;

  __enter;

  if (njb == NULL) {
    __leave;
    return NULL;
  } else {
    estack = (njb_error_stack_t *) njb->error_stack;
    if ( estack->idx == estack->count ) {
      njb_error_clear(njb);
      __leave;
      return NULL;
    }
    cp = estack->msg[estack->idx];
    estack->idx++;
  }
  __leave;
  return cp;
}


/**
 * This function dumps the current libnjb error stack to a file,
 * such as stderr. All errors currently on the stack are dumped.
 * After dumping, the error stack is cleared and errors are not
 * returned again. The function may be called on an empty error
 * stack without effect, but you may just as well check the stack
 * yourself using <code>NJB_Error_Pending()</code> as in the 
 * example below.
 *
 * Typical usage:
 *
 * <pre>
 * njb_t *njb;
 *
 * if (NJB_Error_Pending(njb)) {
 *     NJB_Error_Dump(njb, stderr);
 * }
 * </pre>
 *
 * @param njb a pointer to the NJB object to use
 * @param fp a file pointer to dump the textual representation of
 *        the error stack to.
 * @see NJB_Error_Pending()
 */
void NJB_Error_Dump(njb_t *njb, FILE *fp)
{
  __dsub = "NJB_Error_Dump";
  const char *sp;

  __enter;
  NJB_Error_Reset_Geterror(njb);
  while ( (sp = NJB_Error_Geterror(njb)) != NULL ) {
    fprintf(fp, "%s\n", sp);
  }
  njb_error_clear(njb);
  __leave;
}

/**
 * \file datafile.c
 *
 * This file contains functions that deal with putting regular
 * data files onto the devices. It is not quite a "file system" but
 * metadata elements in a database that have all the attributes of
 * regular files.
 */

#include <string.h>
#include "libnjb.h"
#include "njb_error.h"
#include "defs.h"
#include "base.h"
#include "unicode.h"
#include "protocol3.h"
#include "byteorder.h"
#include "datafile.h"

extern int __sub_depth;
extern int njb_unicode_flag;

/* TODO: get rid of 64 bit packing */

/**
 * Creates a new datafile struct.
 * @return a nullified and freshly allocated njb_datafile_t struct
 */
njb_datafile_t *datafile_new (void)
{
  __dsub= "datafile_new";
  njb_datafile_t *df;
  
  __enter;
  
  df = (njb_datafile_t *) malloc(sizeof(njb_datafile_t));
  if ( df == NULL ) {
    __leave;
    return NULL;
  }
  
  memset(df, 0, sizeof(njb_datafile_t));
  
  __leave;
  return df;
}

/**
 * Destroys a datafile struct.
 * @param df the datafile struct to destroy
 */
void NJB_Datafile_Destroy(njb_datafile_t *df)
{
  __dsub= "NJB_Datafile_Destroy";

  __enter;
  if ( df->filename != NULL ) 
    free(df->filename);
  if ( df->folder != NULL ) 
    free(df->folder);
  free(df);
  __leave;
}

/**
 * Helper function that returns the size of the datafile
 * as a 64bit unsigned integer
 * @param df the datafile whose size we are interested in.
 * @return the size.
 */
u_int64_t datafile_size (njb_datafile_t *df)
{
  __dsub= "datafile_size";
  u_int64_t val;
  
  __enter;
  
  val = df->filesize;
  
  __leave;
  return val;
}

/**
 * Helper function that sets the size of the datafile.
 * @param df the datafile to be altered.
 * @param size the size of the file as a 64bit unsigned integer.
 */
void datafile_set_size (njb_datafile_t *df, u_int64_t size)
{
  __dsub= "datafile_set_size";
  
  __enter;
  df->filesize = size;
  __leave;
}

/**
 * Helper function that sets the timestamp for a datafile.
 * @param df the datafile to be altered.
 * @param ts the new timestamp for this datafile.
 */
void datafile_set_time (njb_datafile_t *df, time_t ts)
{
  __dsub= "datafile_set_time";
  
  __enter;
  df->timestamp = (u_int32_t) ts;
  __leave;
}

/**
 * Helper function that sets the name of a datafile.
 * @param df the datafile to be altered.
 * @param filename the new name.
 * @return 0 on success, -1 on failure.
 */
int datafile_set_name (njb_datafile_t *df, const char *filename)
{
  __dsub= "datafile_set_name";
  
  __enter;
  
  df->filename= strdup(filename);
  if ( df->filename == NULL ) {
    __leave;
    return -1;
  }
  
  __leave;
  return 0;
}

/**
 * Helper function that sets the foldername of a datafile.
 * @param df the datafile to be altered.
 * @param folder the new folder name. Folder names always begin
 *        and end with a backslash (\) with a backslash separator
 *        between levels, like this: "\foo\bar\fnord\".
 * @return 0 on success, -1 on failure.
 */
int datafile_set_folder (njb_datafile_t *df, const char *folder)
{
  __dsub= "datafile_set_folder";
  
  __enter;
  
  df->folder= strdup(folder);
  if ( df->folder == NULL ) {
    __leave;
    return -1;
  }
  
  __leave;
  return 0;
}

/**
 * Unpacks a datafile struct from NJB1.
 * @param data the raw binary data to unpack
 * @param nbytes the size of the raw binary data in bytes
 * @return a newly allocated and filled in njb_datafile_t struct.
 */
njb_datafile_t *datafile_unpack (unsigned char *data, size_t nbytes)
{
  __dsub = "datafile_unpack";
  u_int16_t lname;
  unsigned char *dp = (unsigned char *) data;
  njb_datafile_t *df;
  
  __enter;
  
  df = datafile_new();
  if ( df == NULL ) {
    __leave;
    return NULL;
  }
  
  /* Add metadata in correct byte order */
  
  df->filesize = njb1_bytes_to_64bit(&data[0]);
  /*
    FIXME: works correctly???
    df->msdw = njb1_bytes_to_32bit(&data[0]);
    df->lsdw = njb1_bytes_to_32bit(&data[4]);
  */
  lname = njb1_bytes_to_16bit(&data[8]);
  
  if ( (lname + 10) > nbytes ) {
    NJB_Datafile_Destroy(df);
    __leave;
    return NULL;
  }
  
  /* Handle unicode conversion of filename, or
   * default to ISO 8859-1 */
  df->filename = (char *) malloc(lname+1);
  memcpy(df->filename, &dp[10], lname);
  df->filename[lname] = '\0';
  if ( df->filename == NULL ) {
    NJB_Datafile_Destroy(df);
    __leave;
    return NULL;
  }
  
  if (njb_unicode_flag == NJB_UC_UTF8) {
    char *utf8str = NULL;
    
    utf8str = strtoutf8((unsigned char *) df->filename);
    if (utf8str == NULL) {
      NJB_Datafile_Destroy(df);
      __leave;
      return NULL;
    }
    free(df->filename);
    df->filename = utf8str;
  }
  
  df->flags = NJB_FILEFLAGS_REGULAR_FILE;

  /* NJB1 devices don't have folders */
  df->folder = NULL;
  
  __leave;
  return df;
}

/**
 * A function that packs a datafile tag into the format used
 * by the NJB1.
 * @param df the datafile tag to pack.
 * @param size a pointer to a variable that shall hold the
 *        size of the packed object after this function has
 *        been called.
 * @return a pointer to the packed struct, which must be freed
 *         by the caller after use.
 */
unsigned char *datafile_pack (njb_datafile_t *df, u_int32_t *size)
{
  __dsub= "datafile_pack";
  unsigned char *ptag;
  char *filename = NULL;
  u_int16_t len;
  
  __enter;
  
  
  /* Convert filename to ISO 8859-1 as is used on NJB 1 */
  if (njb_unicode_flag == NJB_UC_UTF8) {
    filename = utf8tostr((unsigned char *) df->filename);
  } else {
    filename = strdup(df->filename);
  }
  if (filename == NULL) {
    __leave;
    return NULL;
  }
  
  len = (u_int16_t) strlen(filename) + 1;
  *size = len + 10;
  
  ptag= (unsigned char *) malloc(*size);
  if ( ptag == NULL ) {
    free(filename);
    __leave;
    return NULL;
  }
  
  /* Pack tag with correct byte order */
  /* TODO: works correctly ???
     from_32bit_to_njb1_bytes(df->msdw, &ptag[0]);
     from_32bit_to_njb1_bytes(df->lsdw, &ptag[4]);
  */
  from_64bit_to_njb1_bytes(df->filesize, &ptag[0]);
  from_16bit_to_njb1_bytes(len, &ptag[8]);
  memcpy(&ptag[10], filename, len);
  
  free(filename);
  
  __leave;
  return ptag;
}

/**
 * Helper function that adds a UCS2 unicode string to a memory position
 *
 * @param data a pointer to the raw memory byte array that shall hold the UCS2 string
 * @param datap a index pointer into the same array that points to the current
 *              location (where the string shall be copied) and which will be
 *              modified to point to the next free byte after this function has
 *              been called
 * @param tagtype an identifier for the type of tag, prepended to the string as
 *                16 bits
 * @param unistr the string to add
 */
static void add_bin_unistr(unsigned char *data, u_int32_t *datap, u_int32_t tagtype, unsigned char *unistr)
{
  u_int32_t binlen;
  
  binlen = ucs2strlen(unistr) * 2 + 2;
  from_16bit_to_njb3_bytes(binlen+2, &data[*datap]);
  *datap += 2;
  from_16bit_to_njb3_bytes(tagtype, &data[*datap]);
  *datap += 2;
  memcpy(data+(*datap), unistr, binlen);
  *datap += binlen;
}

/**
 * This function packs (serialize, marshall) a high-level representation 
 * of a datafile metadata structure into a simple byte-array as used by 
 * the series 3 devices.
 *
 * @param njb the NJB object to use
 * @param df the datafile in libnjb structure format
 * @param size a pointer to an integer that will hold the resulting size of the 
 *             packed structure
 * @return the packed structure as a newly allocated byte array. The caller
 *         shall free this memory after use. Returns NULL on failure.
 */
unsigned char *datafile_pack3 (njb_t *njb, njb_datafile_t *df, u_int32_t *size)
{
  __dsub= "datafile_pack3";
  unsigned char *filename = NULL;
  unsigned char *foldername = NULL;
  unsigned char ptag[1024];
  unsigned char *retag;
  u_int32_t p = 0;
  
  __enter;
  
  /* Create a filename */
  filename = strtoucs2((unsigned char *) df->filename);
  if (filename == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return NULL;
  }

  /* Create a folder name too */
  if (df->folder == NULL) {
    foldername = strtoucs2((unsigned char *) "\\");
  } else {
    foldername = strtoucs2((unsigned char *) df->folder);
  }
  if (foldername == NULL) {
    free(filename);
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return NULL;
  }
 
  /* Add filename tag */
  add_bin_unistr(ptag, &p, NJB3_FNAME_FRAME_ID, filename);
  free(filename);
  /* Add folder tag */
  add_bin_unistr(ptag, &p, NJB3_DIR_FRAME_ID, foldername);
  free(foldername);

  /* Add filesize in 32 bits */
  from_16bit_to_njb3_bytes(0x0006, &ptag[p]);
  p += 2;
  from_16bit_to_njb3_bytes(NJB3_FILESIZE_FRAME_ID, &ptag[p]);
  p += 2;
  /*
   * NJB3 doesn't seem to support
   * filesizes greater than 32 bits (?) 
   * so df->msdw is thrown away! 
   */
  from_32bit_to_njb3_bytes((u_int32_t) df->filesize, &ptag[p]);
  p += 4;
  /* Add timestamp. */
  from_16bit_to_njb3_bytes(0x0006, &ptag[p]);
  p += 2;
  from_16bit_to_njb3_bytes(NJB3_FILETIME_FRAME_ID, &ptag[p]);
  p += 2;
  from_32bit_to_njb3_bytes(df->timestamp, &ptag[p]);
  p += 4;
  if ( njb->device_type == NJB_DEVICE_NJB3 
       || njb->device_type == NJB_DEVICE_NJBZEN 
       ) {
    /* This is the locked status used in older firmware. The file is not locked. */
    from_16bit_to_njb3_bytes(0x0004U, &ptag[p]);
    p += 2;
    from_16bit_to_njb3_bytes(NJB3_LOCKED_FRAME_ID, &ptag[p]);
    p += 2;
    from_16bit_to_njb3_bytes(0x0000U, &ptag[p]); /* = not locked */
    p += 2;
  } else {
    /* End with flags instead - used in USB 2.0 devices
     * Probably locking files was deemed unnecessary,
     * so that was removed. Instead this strange thing was introduced. */
    from_16bit_to_njb3_bytes(0x0006U, &ptag[p]);
    p += 2;
    from_16bit_to_njb3_bytes(NJB3_FILEFLAGS_FRAME_ID, &ptag[p]); /* Could be readable/writeable etc... */
    p += 2;
    from_32bit_to_njb3_bytes(0x20000000U, &ptag[p]); /* Frame contents */
    p += 4;
  }

  *size = p;
  retag = (unsigned char *) malloc(*size);
  if ( retag == NULL ) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return NULL;
  }
  memcpy(retag, ptag, *size);
  
  __leave;
  return retag;
}

/**
 * This creates a folder metadata entry for a new folder on the
 * series 3 devices. (Not applicable for NJB1.)
 *
 * @param njb the NJB object to use
 * @param name the name of the new folder, must have a reasonable format!
 * @param size a pointer to an integer that will hold the resulting size
 *             of the packed folder structure
 * @return the packed folder structure as a newly allocated byte array.
 *         the caller shall free this memory after use. Returns NULL on
 *         failure.
 */
unsigned char *new_folder_pack3 (njb_t *njb, const char *name, u_int32_t *size)
{
  __dsub= "new_folder_pack3";
  unsigned char ptag[1024];
  unsigned char *retag;
  unsigned char *dirname = NULL;
  u_int32_t p = 0;
  
  __enter;
  
  /* Create a filename */
  dirname = strtoucs2((unsigned char *) name);
  if (dirname == NULL) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return NULL;
  }
  
  /* Add "." period filename tag */
  from_16bit_to_njb3_bytes(0x0006, &ptag[p]);
  p += 2;
  from_16bit_to_njb3_bytes(NJB3_FNAME_FRAME_ID, &ptag[p]);
  p += 2;
  from_16bit_to_njb3_bytes(0x002e, &ptag[p]); /* Period "." */
  p += 2;
  from_16bit_to_njb3_bytes(0x0000, &ptag[p]); /* Terminator */
  p += 2;
  add_bin_unistr(ptag, &p, NJB3_DIR_FRAME_ID, dirname);
  free(dirname);
  /* Add filesize in 32 bits */
  from_16bit_to_njb3_bytes(0x0006, &ptag[p]);
  p += 2;
  from_16bit_to_njb3_bytes(NJB3_FILESIZE_FRAME_ID, &ptag[p]);
  p += 2;
  /* Directories are obviously 0 bytes long */
  from_32bit_to_njb3_bytes(0x00000000U, &ptag[p]);
  p += 4;
  /* Add timestamp. */
  from_16bit_to_njb3_bytes(0x0006, &ptag[p]);
  p += 2;
  from_16bit_to_njb3_bytes(NJB3_FILETIME_FRAME_ID, &ptag[p]);
  p += 2;
  /* Put in current datetime? */
  from_32bit_to_njb3_bytes(0x00000000U, &ptag[p]);
  p += 4;
  if ( njb->device_type == NJB_DEVICE_NJB3 
       || njb->device_type == NJB_DEVICE_NJBZEN 
       ) {
    /* This is the locked status used in older firmware. The file is not locked. */
    from_16bit_to_njb3_bytes(0x0004, &ptag[p]);
    p += 2;
    from_16bit_to_njb3_bytes(NJB3_LOCKED_FRAME_ID, &ptag[p]);
    p += 2;
    from_16bit_to_njb3_bytes(0x0000, &ptag[p]); /* = not locked */
    p += 2;
  } else {
    /* End with flags instead - used in USB 2.0 devices
     * Probably locking files was deemed unnecessary,
     * so that was removed. Instead this strange thing was introduced. */
    from_16bit_to_njb3_bytes(0x0006, &ptag[p]);
    p += 2;
    from_16bit_to_njb3_bytes(NJB3_FILEFLAGS_FRAME_ID, &ptag[p]); /* Could be readable/writeable etc... */
    p += 2;
    from_32bit_to_njb3_bytes(0x80000000U, &ptag[p]); /* Frame contents */
    p += 4;
  }
  /* Three 0x0000 after the stuff, why, I don't know. */
  from_16bit_to_njb3_bytes(0x0000, &ptag[p]);
  p += 2;
  from_16bit_to_njb3_bytes(0x0000, &ptag[p]);
  p += 2;
  from_16bit_to_njb3_bytes(0x0000, &ptag[p]);
  p += 2;

  *size = p;
  retag = (unsigned char *) malloc(*size);
  if ( retag == NULL ) {
    NJB_ERROR(njb, EO_NOMEM);
    __leave;
    return NULL;
  }
  memcpy(retag, ptag, *size);
  
  __leave;
  return retag;
}

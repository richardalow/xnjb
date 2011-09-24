#include "common.h"
#include <string.h>

static void datafile_dump (njb_datafile_t *df, FILE *fp)
{
  u_int64_t size = df->filesize;
  
  fprintf(fp, "File ID  : %u\n", df->dfid);
  fprintf(fp, "Filename : %s\n", df->filename);
  if (df->folder != NULL) {
    fprintf(fp, "Folder   : %s\n", df->folder);
  }
  fprintf(fp, "Fileflags: %08X\n", df->flags);
#ifdef __WIN32__
  fprintf(fp, "Size :     %I64u bytes\n", size);
#else
  fprintf(fp, "Size :     %llu bytes\n", size);
#endif
}

int main (int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  extern char *optarg;
  int opt;
  int n, debug;
  njb_datafile_t *filetag;
  char *lang;
  
  debug= 0;
  while ( (opt= getopt(argc, argv, "D:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug= atoi(optarg);
      break;
    default:
      fprintf(stderr, "usage: tracks [ -D debuglvl ]\n");
      return 1;
    }
  }
  
  if ( debug ) NJB_Set_Debug(debug);
  
  /*
   * Check environment variables $LANG and $LC_CTYPE
   * to see if we want to support UTF-8 unicode
   * $LANG = "xx_XX.UTF-8" or $LC_CTYPE = "?"
   * trigger unicode support.
   */
  lang = getenv("LANG");
  if (lang != NULL) {
    if (strlen(lang) > 5) {
      if (!strcmp(&lang[strlen(lang)-5], "UTF-8")) {
	NJB_Set_Unicode(NJB_UC_UTF8);
      }
    }
  }

  if (NJB_Discover(njbs, 0, &n) == -1) {
    fprintf(stderr, "could not locate any jukeboxes\n");
    return 1;
  }
  
  if ( n == 0 ) {
    fprintf(stderr, "no NJB devices found\n");
    return 0;
  } 
  
  njb = njbs;
  
  if ( NJB_Open(njb) == -1 ) {
    NJB_Error_Dump(njb, stderr);
    return 1;
  }
  
  if ( NJB_Capture(njb) == -1 ) {
    NJB_Error_Dump(njb, stderr);
    return 1;
  }
  
  NJB_Reset_Get_Datafile_Tag(njb);
  while ( (filetag = NJB_Get_Datafile_Tag (njb)) ) {
    datafile_dump(filetag, stdout);
    printf("----------------------------------\n");
    NJB_Datafile_Destroy(filetag);
  }

  /* Dump any pending errors */
  if (NJB_Error_Pending(njb)) {
    NJB_Error_Dump(njb, stderr);
  }
  
  NJB_Release(njb);
  
  NJB_Close(njb);
  return 0;
}


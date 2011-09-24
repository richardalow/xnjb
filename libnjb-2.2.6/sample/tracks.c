#include "common.h"
#include <stdio.h>
#include <string.h>

static void songid_frame_dump (njb_songid_frame_t *frame, FILE *fp)
{
  fprintf(fp, "%s: ", frame->label);
	
  if ( frame->type == NJB_TYPE_STRING ) {
    fprintf(fp, "%s\n", frame->data.strval);
  } else if (frame->type == NJB_TYPE_UINT16) {
    fprintf(fp, "%d\n", frame->data.u_int16_val);
  } else if (frame->type == NJB_TYPE_UINT32) {
    fprintf(fp, "%u\n", frame->data.u_int32_val);
  } else {
    fprintf(fp, "(weird data word size, cannot display!)\n");
  }
}

static void songid_dump (njb_songid_t *song, FILE *fp)
{
  njb_songid_frame_t *frame;
  
  NJB_Songid_Reset_Getframe(song);
  fprintf(fp, "ID: %u\n", song->trid);
  while( (frame = NJB_Songid_Getframe(song)) ) {
    songid_frame_dump(frame, fp);
  }
}

int main (int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  extern char *optarg;
  int opt;
  int n, debug, rc = 1;
  int extended = 0; /* Whether to get extended track info */
  njb_songid_t *songtag;
  char *lang;
  
  debug= 0;
  while ( (opt= getopt(argc, argv, "D:E")) != -1 ) {
    switch (opt) {
    case 'D':
      debug = atoi(optarg);
      break;
    case 'E':
      extended = 1;
      break;
    default:
      fprintf(stderr, "usage: tracks [ -D debuglvl ] -E\n");
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
    NJB_Error_Dump(njb,stderr);
    return 1;
  }
  
  if ( NJB_Capture(njb) == -1 ) {
    NJB_Error_Dump(njb,stderr);
    goto err1;
  }

  if (extended != 0) {
    NJB_Get_Extended_Tags(njb, 1);
  }
  
  n = 0;
  NJB_Reset_Get_Track_Tag(njb);
  while ( (songtag = NJB_Get_Track_Tag(njb)) ) {
    songid_dump(songtag, stdout);
    NJB_Songid_Destroy(songtag);
    printf("----------------------------------\n");
    n ++;
  }

  /* Dump any pending errors */
  if (NJB_Error_Pending(njb)) {
    NJB_Error_Dump(njb,stderr);
  }

  printf("In total: %u tracks.\n", n);
  
  NJB_Release(njb);
  rc = 0;
  
err1:
  NJB_Close(njb);
  return rc;
}


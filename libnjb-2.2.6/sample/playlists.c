#include "common.h"
#include <string.h>

static void playlist_dump (njb_playlist_t *pl, FILE *fp)
{
  njb_playlist_track_t *track;
  unsigned int i = 1;
  
  fprintf(fp, "Playlist: %s\n", pl->name);
  fprintf(fp, "ID:       %u\n", pl->plid);
  fprintf(fp, "Tracks:   %u\n", pl->ntracks);
  fprintf(fp, "State:    %d\n", pl->_state);
  
  NJB_Playlist_Reset_Gettrack(pl);
  while ( (track = NJB_Playlist_Gettrack(pl)) ) {
    fprintf(fp, "%5u - track ID %u\n", i, track->trackid);
    i++;
  }
}

int main (int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  int n, debug;
  njb_playlist_t *playlist;
  extern char *optarg;
  int opt;
  char *lang;
  
  debug = 0;
  while ( (opt= getopt(argc, argv, "D:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug= atoi(optarg);
      break;
    default:
      fprintf(stderr, "usage: playlists [ -D debuglvl ]\n");
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
    return 1;
  }
  
  NJB_Reset_Get_Playlist(njb);
  n = 0;
  while ( (playlist = NJB_Get_Playlist(njb)) ) {
    playlist_dump(playlist, stdout);
    printf("----------------------------------\n");
    n ++;
  }
  
  /* Dump any pending errors */
  if (NJB_Error_Pending(njb)) {
    NJB_Error_Dump(njb,stderr);
  }
  
  printf("Total: %d playlists\n", n);
  
  NJB_Release(njb);
  
  NJB_Close(njb);
  return 0;
}


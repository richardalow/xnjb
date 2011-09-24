#include "common.h"
#include <string.h>

#define PL_CREATE	1
#define PL_DELETE	2
#define PL_RENAME	3

static void usage(void)
{
  fprintf(stderr, "usage: pl [ -D debuglvl ] -d name\n" \
	  "pl [ -D debuglvl ] -r oldname newname\n" \
	  "pl [ -D debuglvl ] -c name trackid < trackid2 trackid3 ... >\n");
}

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
  njb_t njbs[NJB_MAX_DEVICES];
  njb_t *njb;
  njb_playlist_t *playlist;
  njb_playlist_track_t *pl_track;
  int i, n, opt, mode, debug;
  char *name, *newname;
  extern int optind;
  extern char *optarg;
  char *endptr;
  char *lang;
  
  debug= 0;
  mode= 0;
  name= NULL;
  newname = NULL;
  while ( (opt= getopt(argc, argv, "d:r:c:")) != -1 ) {
    switch (opt) {
    case 'c':
      mode= PL_CREATE;
      break;
    case 'd':
      mode= PL_DELETE;
      break;
    case 'r':
      mode= PL_RENAME;
      break;
    case 'D':
      debug= atoi(optarg);
      break;
    case '?':
    default:
      usage();
      return 1;
    }
    name= optarg;
  }
  argc-= optind;
  argv+= optind;
  
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
  
  if ( ! mode ) {
    usage();
    return 1;
  }
  
  playlist = NULL;
  if ( mode == PL_CREATE ) {
    playlist = NJB_Playlist_New();
    if ( playlist == NULL ) {
      perror("NJB_Playlist_New");
      return 1;
    }
    
    if ( NJB_Playlist_Set_Name(playlist, name) == -1 ) {
      perror("NJB_Playlist_Set_Name");
    } else {
      for (i= 0; i< argc; i++) {
	u_int32_t trid= strtoul(argv[i], &endptr, 10);
	if ( *endptr != '\0' ) {
	  fprintf(stderr, "invalid track id %s",
		  argv[i]);
	  return 1;
	}
	
	pl_track= NJB_Playlist_Track_New(trid);
	if ( pl_track == NULL ) {
	  perror("NJB_Playlist_Track_New");
	  return 1;
	}
	
	NJB_Playlist_Addtrack(playlist, pl_track,
			      NJB_PL_END);
      }
      playlist_dump(playlist, stdout);
    }
  } else if ( mode == PL_RENAME ) {
    newname= argv[0];
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
  
  if ( mode == PL_CREATE ) {
    if ( NJB_Update_Playlist(njb, playlist) == -1 )
      NJB_Error_Dump(njb,stderr);
  } else {
    int status = 0;
    int repeat= 1;
    
    NJB_Reset_Get_Playlist(njb);
    while ( repeat ) {
      playlist = NJB_Get_Playlist(njb);
      if ( playlist == NULL ) { 
	repeat= 0;
      } else {
	repeat = strcmp(name, playlist->name);
	if ( repeat )
	  NJB_Playlist_Destroy(playlist);
      }
    }
    
    if ( playlist == NULL ) {
      NJB_Error_Dump(njb,stderr);
    } else {
      if ( mode == PL_DELETE ) {
	status= NJB_Delete_Playlist(njb,
				    playlist->plid);
      } else {
	if ( NJB_Playlist_Set_Name(playlist, newname) == -1 ) {
	  status = -1;
	} else {
	  status= NJB_Update_Playlist(njb,
				      playlist);
	}
      }
    }
    
    if ( status == -1 ) NJB_Error_Dump(njb,stderr);
  }
  
  NJB_Release(njb);
  
  NJB_Close(njb);
  
  return 0;
}


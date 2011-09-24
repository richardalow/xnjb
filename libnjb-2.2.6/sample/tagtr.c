#include "common.h"
#include <string.h>

static char *prompt (const char *prompt, char *buffer, size_t bufsz, int required)
{
  char *cp, *bp;
  
  while (1) {
    fprintf(stdout, "%s> ", prompt);
    if ( fgets(buffer, bufsz, stdin) == NULL ) {
      if (ferror(stdin)) {
	perror("fgets");
      } else {
	fprintf(stderr, "EOF on stdin\n");
      }
      return NULL;
    }
    
    cp= strrchr(buffer, '\n');
    if ( cp != NULL ) *cp= '\0';
    
    bp= buffer;
    while ( bp != cp ) {
      if ( *bp != ' ' && *bp != '\t' ) return bp;
      bp++;
    }
    
    if (! required) return bp;
  }
}

static void usage (void)
{
  fprintf(stderr, "usage: tagtr [ -D debuglvl ] <trackid>\n");
  exit(1);
}

int main(int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  int n, opt, debug, lookup;
  extern char *optarg;
  extern int optind;
  char filename[80], artist[80], title[80], genre[80], codec[80], album[80];
  char num[80];
  char *pfilename, *partist, *ptitle, *pgenre, *pcodec, *palbum, *pnum;
  char *endptr;
  u_int16_t tracknum, length, year;
  u_int32_t filesize;
  int trackid;
  char *lang;
  njb_songid_t *songid;
  njb_songid_frame_t *frame;
  
  debug = 0;
  lookup = 0;
  
  while ( (opt= getopt(argc, argv, "D:l")) != -1 ) {
    switch (opt) {
    case 'l':
      lookup= 1;
    case 'D':
      debug= atoi(optarg);
      break;
    default:
      usage();
    }
  }
  argc-= optind;
  argv+= optind;
  
  if ( argc != 1 ) usage();
  
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
  
  trackid= strtoul(argv[0], &endptr, 10);
  if ( endptr[0] != '\0' ) {
    fprintf(stderr, "invalid id %s\n", argv[1]);
    return 1;
  }
  
  if ( (pcodec= prompt("CODEC", codec, 80, 0)) == NULL ) return 1;
  if ( ! strlen(pcodec) ) pcodec= NULL;
  
  if ( (ptitle= prompt("Title", title, 80, 0)) == NULL ) return 1;
  if ( ! strlen(ptitle) ) ptitle= NULL;
  
  if ( (palbum= prompt("Album", album, 80, 0)) == NULL ) return 1;
  if ( ! strlen(palbum) ) palbum= NULL;
  
  if ( (partist= prompt("Artist", artist, 80, 0)) == NULL ) return 1;
  if ( ! strlen(partist) ) partist= NULL;
  
  if ( (pgenre= prompt("Genre", genre, 80, 0)) == NULL ) return 1;
  if ( ! strlen(pgenre) ) pgenre= NULL;
  
  if ( (pfilename= prompt("File path", filename, 80, 0)) == NULL ) return 1;
  if ( ! strlen(pfilename) ) pfilename= NULL;
  
  if ( (pnum= prompt("Track number", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    tracknum = strtoul(pnum, 0, 10);
  } else {
    tracknum = 0;
  }
  
  if ( (pnum= prompt("Length", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    length= strtoul(pnum, 0, 10);
  } else {
    length= 0;
  }
  
  if ( (pnum= prompt("Year", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    year = strtoul(pnum, 0, 10);
  } else {
    year = 0;
  }
  
  filesize= 0;
  if ( ! lookup ) {
    if ( (pnum= prompt("File size", num, 80, 0)) == NULL ) 
      return 1;
    if ( strlen(pnum) ) {
      filesize = strtoul(pnum, 0, 10);
    } else {
      filesize = 0;
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
  
  njb= njbs;
  
  if ( NJB_Open(njb) == -1 ) {
    NJB_Error_Dump(njb,stderr);
    return 1;
  }
  
  NJB_Capture(njb);
  
  songid = NJB_Songid_New();
  frame = NJB_Songid_Frame_New_Codec(pcodec);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Filesize(filesize);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Title(ptitle);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Album(palbum);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Artist(partist);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Genre(pgenre);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Year(year);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Tracknum(tracknum);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Length(length);
  NJB_Songid_Addframe(songid, frame);
  frame = NJB_Songid_Frame_New_Filename(pfilename);
  NJB_Songid_Addframe(songid, frame);
  
  if ( NJB_Replace_Track_Tag(njb, trackid, songid) == -1 ) {
    NJB_Error_Dump(njb,stderr);
  } else {
    printf("NJB track ID:    %u\n", trackid);
  }
  printf("\n");
  
  NJB_Songid_Destroy(songid);
  
  NJB_Release(njb);
  
  return 0;
}

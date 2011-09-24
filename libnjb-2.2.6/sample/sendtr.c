#include "common.h"
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h> /* basename() */
#endif

/*
 * This is an improved commandline track transfer program
 * based on Enrique Jorreto Ledesma's work on the original program by 
 * Shaun Jackman and Linus Walleij.
 */

/* Function that compensate for missing libgen.h on Windows */
#ifndef HAVE_LIBGEN_H
static char *basename(char *in) {
  char *p;
  if (in == NULL)
    return NULL;
  p = in + strlen(in) - 1;
  while (*p != '\\' && *p != '/' && *p != ':')
    { p--; }
  return ++p;
}
#endif

static int progress (u_int64_t sent, u_int64_t total, const char* buf, unsigned len, void *data)
{
  int percent = (sent*100)/total;
#ifdef __WIN32__
  printf("Progress: %I64u of %I64u (%d%%)\r", sent, total, percent);
#else
  printf("Progress: %llu of %llu (%d%%)\r", sent, total, percent);
#endif
  fflush(stdout);
  return 0;
}

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
    
    cp = strrchr(buffer, '\n');
    if ( cp != NULL ) *cp = '\0';
    
    bp = buffer;
    while ( bp != cp ) {
      if ( *bp != ' ' && *bp != '\t' ) return bp;
      bp++;
    }
    
    if (! required) return bp;
  }
}

static void usage(void)
{
  fprintf(stderr, "usage: sendtr [ -D debuglvl ] [ -q ] -t <title> -a <artist> -l <album> -c <codec>\n");
  fprintf(stderr, "       -g <genre> -n <track number> -d <duration in seconds> <path>\n");
  fprintf(stderr, "(-q means the program will not ask for missing information.)\n");
  exit(1);
}

int main(int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  int n, opt, debug;
  extern int optind;
  extern char *optarg;
  char *path, *filename;
  char artist[80], title[80], genre[80], codec[80], album[80];
  char num[80];
  char *partist = NULL;
  char *ptitle = NULL;
  char *pgenre = NULL;
  char *pcodec = NULL;
  char *palbum = NULL;
  u_int16_t tracknum = 0;
  u_int16_t length = 0;
  u_int16_t year = 0;
  u_int16_t quiet = 0;
  u_int32_t filesize;
  struct stat sb;
  u_int32_t trackid;
  char *lang;
  njb_songid_t *songid;
  njb_songid_frame_t *frame;
  
  debug = 0;

  /* Arguments parsing from njbputtrack (njbtools) */
  while ( (opt = getopt(argc, argv, "qD:t:a:l:c:g:n:d:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug = atoi(optarg);
      break;
    case 't':
      ptitle = strdup(optarg);
      break;
    case 'a':
      partist = strdup(optarg);
      break;
    case 'l':
      palbum = strdup(optarg);
      break;
    case 'c':
      pcodec = strdup(optarg); // XXX DSM check for MP3, WAV or WMA
      break;
    case 'g':
      pgenre = strdup(optarg);
      break;
    case 'n':
      tracknum = atoi(optarg);
      break;
    case 'd':
      length = atoi(optarg);
      break;
    case 'q':
      quiet = 1;
      break;
    default:
      usage();
    }
  }
  argc-= optind;
  argv+= optind;
  
  if ( argc != 1 ) {
    printf("You need to pass a filename.\n");
    usage();
  }
  if ( debug ) {
    NJB_Set_Debug(debug);
  }
  
  
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
  
  path = argv[0];

  filename = basename(path);
  if ( stat(path, &sb) == -1 ) {
    fprintf(stderr, "%s: ", path);
    perror("stat");
    return 1;
  }
  filesize = (u_int32_t) sb.st_size;

  /* Ask for missing parameters if not quiet */
  if (!quiet) {
    if (pcodec == NULL) {
      if ( (pcodec = prompt("Codec", codec, 80, 1)) == NULL ) {
	printf("A codec name is required.\n");
	usage();
      }
    }
    if (!strlen(pcodec)) {
      printf("A codec name is required.\n");
      usage();
    }
    
    if (ptitle == NULL) {
      if ( (ptitle = prompt("Title", title, 80, 0)) == NULL )
	usage();
    }
    if (!strlen(ptitle))
      ptitle = NULL;
    
    
    if (palbum == NULL) {
      if ( (palbum = prompt("Album", album, 80, 0)) == NULL )
	usage();
    }
    if (!strlen(palbum))
      palbum = NULL;
    
    if (partist == NULL) {
    if ( (partist = prompt("Artist", artist, 80, 0)) == NULL )
      usage();
    }
    if (!strlen(partist))
      partist = NULL;
    
    if (pgenre == NULL) {
      if ( (pgenre = prompt("Genre", genre, 80, 0)) == NULL )
	usage();
    }
    if (!strlen(pgenre))
      pgenre = NULL;
    
    if (tracknum == 0) {
      char *pnum;
      if ( (pnum = prompt("Track number", num, 80, 0)) == NULL )
      tracknum = 0;
      if ( strlen(pnum) ) {
	tracknum = strtoul(pnum, 0, 10);
      } else {
	tracknum = 0;
      }
    }
    
    if (year == 0) {
      char *pnum;
      if ( (pnum = prompt("Year", num, 80, 0)) == NULL )
	year = 0;
      if ( strlen(pnum) ) {
	year = strtoul(pnum, 0, 10);
      } else {
	year = 0;
      }
    }
    
    if (length == 0) {
      char *pnum;
      if ( (pnum = prompt("Length", num, 80, 0)) == NULL )
	length = 0;
      if ( strlen(pnum) ) {
	length = strtoul(pnum, 0, 10);
      } else {
	length = 0;
      }
    }
  }
    
  printf("Sending track:\n");
  printf("Codec:     %s\n", pcodec);
  if (ptitle) printf("Title:     %s\n", ptitle);
  if (palbum) printf("Album:     %s\n", palbum);
  if (partist) printf("Artist:    %s\n", partist);
  if (pgenre) printf("Genre:     %s\n", pgenre);
  if (year > 0) printf("Year:      %d\n", year);
  if (tracknum > 0) printf("Track no:  %d\n", tracknum);
  if (length > 0) printf("Length:    %d\n", length);
  
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
  frame = NJB_Songid_Frame_New_Filename(filename);
  NJB_Songid_Addframe(songid, frame);
  
  if ( NJB_Send_Track(njb, path, songid, progress, NULL, &trackid) == -1 ) {
    printf("\n");
    NJB_Error_Dump(njb,stderr);
  } else {
    printf("\nNJB track ID:    %u\n", trackid);
  }
  printf("\n");
  
  NJB_Songid_Destroy(songid);
  
  NJB_Release(njb);
  NJB_Close(njb);
  
  return 0;
}

#include "common.h"
#include <sys/stat.h>
#include <string.h>

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

static void usage (void)
{
  fprintf(stderr, "usage: sendfile [ -D<debuglvl> ] [ -F<foldername> ] <path>\n");
  exit(1);
}


int main(int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  int n, opt, debug;
  extern int optind;
  extern char *optarg;
  char *path;
  char *foldername = NULL;
  u_int32_t fileid;
  char *lang;

  debug= 0;
  while ( (opt = getopt(argc, argv, "D:F::")) != -1 ) {
    switch (opt) {
    case 'D':
      debug = atoi(optarg);
      break;
    case 'F':
      foldername = optarg;
      break;
    default:
      usage();
      break;
    }
  }
  argc-= optind;
  argv+= optind;
  
  if ( argc != 1 ) usage();
  
  if (foldername != NULL) {
    if (foldername[0] != '\\' || foldername[strlen(foldername)-1] != '\\') {
      fprintf(stderr, "Illegal folder parameter: \"%s\"\n", foldername);
      fprintf(stderr, "Folder names must begin and end with a backslash, and be doubly escaped e.g.:\n");
      fprintf(stderr, "sendfile -F\"\\\\foo\\\\bar\\\\fnord\\\\\" foo.txt\n");
      exit(1);
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
  
  path = argv[0];
  
  printf("Sending file:\n");
  printf("Filename:     %s\n", path);

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
  
  if ( NJB_Send_File(njb, path, NULL, foldername, progress, NULL, &fileid) == -1 ) {
    NJB_Error_Dump(njb,stderr);
  } else {
    printf("\nNJB ID:    %u\n", fileid);
  }
  printf("\n");
  
  NJB_Release(njb);
  
  NJB_Close(njb);
  
  return 0;
}

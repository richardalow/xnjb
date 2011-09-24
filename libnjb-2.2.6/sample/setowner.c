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
    
    cp = strrchr(buffer, '\n');
    if ( cp != NULL ) *cp= '\0';
    
    bp = buffer;
    while ( bp != cp ) {
      if ( *bp != ' ' ) return bp;
      bp++;
    }
    
    if (! required) return bp;
  }
}

int main (int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  int n, opt, debug;
  char newowner[80];
  char *pown;
  char *ownerstring;
  extern char *optarg;
  char *lang;
  
  debug = 0;
  while ( (opt = getopt(argc, argv, "D:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug= atoi(optarg);
      break;
    default:
      fprintf(stderr, "usage: %s [ -D debuglvl ]\n", argv[0]);
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
    fprintf(stderr, "No NJB devices found\n");
    return 1;
  }

  njb = njbs;
  if ( NJB_Open(njb) == -1 ) {
    NJB_Error_Dump(njb,stderr);
    return 1;
  }

  if ( (pown = prompt("New ownerstring", newowner, 80, 1)) == NULL ) return 1;
  if ( NJB_Set_Owner_String (njb, newowner) == -1 ) {
    NJB_Error_Dump(njb,stderr);
    return 1;
  }
  
  ownerstring = NJB_Get_Owner_String(njb);
  printf("New owner string: %s\n", ownerstring);
  free(ownerstring);
  
  NJB_Close(njb);
  return 0;
}

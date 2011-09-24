#include "common.h"

int main (int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES];
  njb_t *njb;
  int n, opt, debug;
  extern char *optarg;
  u_int64_t total;
  u_int64_t free;
  
  debug = 0;
  while ( (opt = getopt(argc, argv, "D:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug = atoi(optarg);
      break;
    default:
      fprintf(stderr, "usage: handshake [ -D debuglvl ]\n");
      return 1;
    }
  }
  
  if ( debug ) NJB_Set_Debug(debug);

  if (NJB_Discover(njbs, 0, &n) == -1) {
    fprintf(stderr, "could not locate any jukeboxes\n");
    return 1;
  }

  if ( n == 0 ) {
    fprintf(stderr, "no NJB devices found\n");
    return 0;
  }

  njb = &njbs[0];

  if ( NJB_Open(njb) == -1 ) {
    NJB_Error_Dump(njb,stderr);
    return 1;
  }
  
  if (NJB_Get_Disk_Usage(njb, &total, &free) == -1) {
    NJB_Error_Dump(njb,stderr);
    return 1;
  }
  
#ifdef __WIN32__
  printf("Total bytes on jukebox: %I64u (%I64u MB)\n",
	 total, total/(1024*1024));
  printf("Free bytes on jukebox: %I64u (%I64u MB)\n",
	 free, free/(1024*1024));
#else
  printf("Total bytes on jukebox: %llu (%llu MB)\n",
	 total, total/(1024*1024));
  printf("Free bytes on jukebox: %llu (%llu MB)\n",
	 free, free/(1024*1024));
#endif
  
  NJB_Close(njb);
  
  return 0;
}


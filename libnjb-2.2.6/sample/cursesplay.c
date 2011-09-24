#include "common.h"
#include <curses.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* Global repeation variable */
int repeat = 1;

static void stopplay (int signo)
{
  repeat = 0;
}

static void hhmmss (u_int16_t seconds, u_int16_t *hh, u_int16_t *mm, u_int16_t *ss)
{
  *hh = *mm = 0;
  if ( seconds >= 3600 ) *hh= seconds/3600;
  seconds -= 3600*(*hh);
  if ( seconds >= 60 ) *mm= seconds/60;
  *ss= seconds-(60*(*mm));
}

static void usage (void) {
  fprintf(stderr, "usage: play [ -D debuglvl ] <trackid> ...\n");
  exit(1);
}

int main (int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  int i, n, status, change, opt, debug;
  u_int16_t sec, hh, mm, ss;
  u_int32_t trackid;
  extern char *optarg;
  extern int optind;
  WINDOW * root;
  
  debug = 0;
  
  while ( (opt = getopt(argc, argv, "D:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug = atoi(optarg);
      break;
    default:
      break;
    }
  }
  argc -= optind;
  argv += optind;
  
  if ( ! argc ) usage();
  
  if ( debug ) NJB_Set_Debug(debug);
  
  signal(SIGINT, stopplay);
  
  if ( NJB_Discover(njbs, 0, &n) == -1 ) {
    perror("could not probe for NJB devices\n");
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
  
  for (i= 0; i< argc; i++) {
    trackid= strtoul(argv[i], NULL, 10);
    
    if ( i == 0 ) {
      status= NJB_Play_Track(njb, trackid);
    } else {
      status= NJB_Queue_Track(njb, trackid);
    }
    
    if ( status == -1 ) NJB_Error_Dump(njb, stdout);
  }
  
  root = initscr();
  cbreak();
  noecho();
  halfdelay (1);
  
  printw("+---------------+\n"
	 "| x  : exit     |\n"
	 "| p  : pause    |\n"
	 "| r  : resume   |\n"
	 "| <> : seek 10s |\n"
	 "+---------------+\n");
  
  change = 0;
  i = 0;
  mvprintw(6, 0, "Track ID %10.10s: 00:00:00", argv[i]);
  fflush(stdout);
  while ( repeat ) {
    int c;
    
    if ((c = getch ()) != ERR) {
      switch (c) {
      case 'p':
	NJB_Pause_Play (njb);
	break;
      case 'r':
	NJB_Resume_Play (njb);
	break;
      case 'x':
	repeat = 0;
	break;
      case '>':
	NJB_Seek_Track (njb, sec*1000+10000);
	break;
      case '<':
	NJB_Seek_Track (njb, sec > 10 ? sec*1000-10000 : 0);
	break;
      }
    }
    if ( change ) {
      i++;
      if ( i == argc ) {
	repeat= 0;
      } else {
	mvprintw(6, 0, "Track ID %10.10s: 00:00:00", argv[i]);
	refresh ();
      }
      change= 0;
    } else
      NJB_Elapsed_Time(njb, &sec, &change);
    
    hhmmss(sec, &hh, &mm, &ss);
    mvprintw (6, 21, "%02u:%02u:%02u", hh, mm, ss);
    refresh ();
  }
  echo ();
  endwin ();
  
  NJB_Stop_Play(njb);
  
  NJB_Release(njb);
  
  NJB_Close(njb);
  return 0;
}

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

static void dumptime(njb_time_t *time)
{
  if (time != NULL) {
    switch(time->weekday) {
    case 0:
      printf("Sunday ");
      break;
    case 1:
      printf("Monday ");
      break;
    case 2:
      printf("Tuesday ");
      break;
    case 3:
      printf("Wednesday ");
      break;
    case 4:
      printf("Thursday ");
      break;
    case 5:
      printf("Friday ");
      break;
    case 6:
      printf("Saturday ");
    }
    /* OK this is the way we write dates
     * in Sweden, you may change it if you
     * don't like it!
     */
    printf("%u-%.2u-%.2u ", time->year, 
	   time->month, time->day);
    printf("%.2u:%.2u:%.2u\n", time->hours, 
	   time->minutes, time->seconds);
  }
}


int main(int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  extern char *optarg;
  int opt;
  int n, debug;
  njb_time_t *time;
  char *pnum;
  char num[80];
  
  debug = 0;
  while ((opt = getopt(argc, argv, "D:")) != -1) {
    switch (opt) {
    case 'D':
      debug = atoi(optarg);
      break;
    default:
      fprintf(stderr, "usage: settime [ -D debuglvl ]\n");
      return 1;
    }
  }
  
  if (debug)
    NJB_Set_Debug(debug);

  if (NJB_Discover(njbs, 0, &n) == -1) {
    fprintf(stderr, "could not locate any jukeboxes\n");
    return 1;
  } 
  
  if (n == 0) {
    fprintf(stderr, "no NJB devices found\n");
    return 0;
  }
  
  njb = njbs;
  
  if (NJB_Open(njb) == -1) {
    NJB_Error_Dump(njb,stderr);
    return 1;
  }

  if (NJB_Capture(njb) == -1) {
    NJB_Error_Dump(njb,stderr);
    return 1;
  }
  
  time = NJB_Get_Time(njb);
  
  printf("The time on the jukebox is:\n");
  dumptime(time);
  
  printf("\nNew time (old values preserved if left blank):\n");
  
  if ( (pnum= prompt("Year:", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    time->year= (u_int16_t) strtoul(pnum, 0, 10);
  }
  
  if ( (pnum= prompt("Month (1-12):", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    time->month= (u_int16_t) strtoul(pnum, 0, 10);
  }
  
  if ( (pnum= prompt("Day:", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    time->day= (u_int16_t) strtoul(pnum, 0, 10);
  }
  
  if ( (pnum= prompt("Weekday (0-6):", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    time->weekday= (u_int16_t) strtoul(pnum, 0, 10);
  }
  
  if ( (pnum= prompt("Hours (0-23):", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    time->hours= (u_int16_t) strtoul(pnum, 0, 10);
  }
  
  if ( (pnum= prompt("Minutes (0-59):", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    time->minutes= (u_int16_t) strtoul(pnum, 0, 10);
  }
  
  if ( (pnum= prompt("Seconds (0-59):", num, 80, 0)) == NULL ) return 1;
  if ( strlen(pnum) ) {
    time->seconds= (u_int16_t) strtoul(pnum, 0, 10);
  }
  
  printf("The time on the jukebox is being set to:\n");
  dumptime(time);
  
  if (NJB_Set_Time(njb, time) == -1) {
    NJB_Error_Dump(njb,stderr);
  }
  
  NJB_Destroy_Time(time);
  
  NJB_Release(njb);
  
  NJB_Close(njb);
  return 0;
}

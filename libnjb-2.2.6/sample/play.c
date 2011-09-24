#include "common.h"
#include <signal.h>

int repeat= 1;

void stopplay (int signo);
void hhmmss (u_int16_t seconds, u_int16_t *hh, u_int16_t *mm, u_int16_t *ss);
void usage (void);

int main (int argc, char **argv)
{
	njb_t njbs[NJB_MAX_DEVICES], *njb;
	int i, n, status, change, opt, debug;
	u_int16_t sec, hh, mm, ss;
	u_int32_t trackid;
	extern char *optarg;
	extern int optind;

	debug= 0;

	while ( (opt= getopt(argc, argv, "D:")) != -1 ) {
		switch (opt) {
		case 'D':
			debug= atoi(optarg);
			break;
		default:
			break;
		}
	}
	argc-= optind;
	argv+= optind;

	if ( ! argc ) usage();

	if ( debug ) NJB_Set_Debug(debug);

	signal(SIGINT, stopplay);

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

	if ( NJB_Capture(njb) == -1 ) {
		NJB_Error_Dump(njb,stderr);
		return 1;
	}

	for (i= 0; i< argc; i++) {
		trackid= strtoul(argv[i], NULL, 10);

		if ( i == 0 ) {
			status= NJB_Play_Track(njb, trackid);
		} else {
			status= NJB_Queue_Track(njb, trackid);
		}

		if ( status == -1 ) NJB_Error_Dump(njb,stderr);
	}

	printf("---> Hit ^C to exit <---\n");

	change= 0;
	i= 0;
	printf("Track ID %10.10s: 00:00:00\b\b\b\b\b\b\b", argv[i]);
	fflush(stdout);
	while ( repeat ) {
		if ( change ) {
			i++;
			if ( i == argc ) {
				repeat= 0;
			} else {
				printf("\rTrack ID %10.10s: 00:00:00\b\b\b\b\b\b\b",
					argv[i]);
				fflush(stdout);
			}
			change= 0;
		} else
			NJB_Elapsed_Time(njb, &sec, &change);
		hhmmss(sec, &hh, &mm, &ss);
		printf("%02u:%02u:%02u\b\b\b\b\b\b\b\b", hh, mm, ss);
		fflush(stdout);

		sleep(1);
	}
	printf("\n");

	NJB_Stop_Play(njb);

	NJB_Release(njb);

	NJB_Close(njb);
	return 0;
}

void stopplay (int signo)
{
	repeat= 0;
}

void hhmmss (u_int16_t seconds, u_int16_t *hh, u_int16_t *mm, u_int16_t *ss)
{
	if ( seconds >= 3600 ) *hh= seconds/3600;
	seconds-= 3600*(*hh);
	if ( seconds >= 60 ) *mm= seconds/60;
	*ss= seconds-(60*(*mm));
}

void usage (void) {
	fprintf(stderr, "usage: play [ -D debuglvl ] <trackid> ...\n");
	exit(1);
}

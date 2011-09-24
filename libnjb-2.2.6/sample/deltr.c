#include "common.h"

void usage (void);

int main (int argc, char **argv)
{
	njb_t njbs[NJB_MAX_DEVICES], *njb;
	u_int32_t id;
	int n, debug, syntax, opt;
	extern int optind;
	extern char *optarg;
	char *endptr;

	debug= syntax= 0;

	while ( (opt= getopt(argc, argv, "D:")) != -1 ) {
		switch (opt) {
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

	id= strtoul(argv[0], &endptr, 10);
	if ( endptr[0] != '\0' ) {
		fprintf(stderr, "invalid track id %s\n", argv[0]);
		return 1;
	}

	if ( debug ) NJB_Set_Debug(debug);

	if ( NJB_Discover(njbs, 0, &n) == -1 ) {
	        fprintf(stderr, "could not locate any jukeboxes\n");
		return 1;
	}

	if ( n == 0 ) {
		fprintf(stderr, "no NJB devices found\n");
		return 0;
	}

	njb= njbs;

	if ( NJB_Open(njb) == -1 ) {
		NJB_Error_Dump(njb, stderr);
		return 1;
	}

	if ( NJB_Capture(njb) == -1 ) {
		NJB_Error_Dump(njb, stderr);
		return 1;
	}

	if ( NJB_Delete_Track(njb, id) == -1 ) {
		NJB_Error_Dump(njb, stderr);
	}

	NJB_Release(njb);

	NJB_Close(njb);

	return 0;
}

void usage (void)
{
	fprintf(stderr, "usage: deltr [ -D debuglvl ] <trackid>\n");
	exit(1);
}

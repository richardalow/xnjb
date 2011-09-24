#include "common.h"

int progress(u_int64_t sent, u_int64_t total, const char* buf, unsigned len, void *data );
void usage(void);

int main (int argc, char **argv)
{
	njb_t njbs[NJB_MAX_DEVICES], *njb;
	int n, opt, debug;
	u_int32_t id;
	u_int64_t size;
	extern int optind;
	extern char *optarg;
	char *endptr;
	char *file;

	debug= 0;
	size= 0;
	while ( (opt= getopt(argc, argv, "D:s:")) != -1 ) {
		switch (opt) {
		case 'D':
			debug= atoi(optarg);
			break;
		case 's':
			size= strtoul(optarg, &endptr, 10);
			if ( *endptr != '\0' ) {
				fprintf(stderr, "illegal size value %s\n",
					optarg);
				return 1;
			}
			break;
		default:
			usage();
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if ( argc != 2 ) {
		usage();
		return 1;
	}

	id = strtoul(argv[0], &endptr, 10);
	if ( *endptr != 0 ) {
		fprintf(stderr, "illegal value %s\n", optarg);
		return 1;
	} else if ( ! id ) {
		fprintf(stderr, "bad file id %u\n", id);
		return 1;
	}

	file = argv[1];

	if ( debug ) NJB_Set_Debug(debug);


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
		NJB_Error_Dump(njb, stderr);
		return 1;
	}

	NJB_Capture(njb);

	if ( ! size ) {
		njb_datafile_t *df;

		printf("Locating file %u\n", id);
		NJB_Reset_Get_Datafile_Tag(njb);
		while ( (df = NJB_Get_Datafile_Tag(njb)) ) {
			if ( df->dfid == id ) {
				size = df->filesize;
			}
			NJB_Datafile_Destroy(df);
		}

		/* Dump any pending errors */
		NJB_Error_Dump(njb, stderr);

		if ( size ) {
			printf("%Lu bytes\n", size);
		} else {
			fprintf(stderr, "File %u not found\n", id);
		}
	}

	if ( size ) {
		if ( NJB_Get_File(njb, id, (u_int32_t) size, file, progress, NULL ) == -1 ) {
			NJB_Error_Dump(njb, stderr);
		}
		printf("\n");
	}

	NJB_Release(njb);

	NJB_Close(njb);

	return 0;
}

int progress (u_int64_t sent, u_int64_t total, const char* buf, unsigned len, void *data)
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

void usage (void)
{
	fprintf(stderr, "getfile [ -s size ] <fileid> <filename>\n");
}

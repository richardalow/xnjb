#include "common.h"

int main(int argc, char **argv)
{
	njb_t njbs[NJB_MAX_DEVICES], *njb;
	extern char *optarg;
	int opt;
	int n, debug;
	njb_time_t *time;

	debug = 0;
	while ((opt = getopt(argc, argv, "D:")) != -1) {
		switch (opt) {
		case 'D':
			debug = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: dumptime [ -D debuglvl ]\n");
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
		NJB_Error_Dump(njb, stderr);
		return 1;
	}

	if (NJB_Capture(njb) == -1) {
		NJB_Error_Dump(njb, stderr);
		return 1;
	}

	time = NJB_Get_Time(njb);

	if (time != NULL) {
		printf("The time on the jukebox is:\n");
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
		NJB_Destroy_Time(time);
	}

	if (NJB_Error_Pending(njb)) {
	  NJB_Error_Dump(njb, stderr);
	}

	NJB_Release(njb);

	NJB_Close(njb);
	return 0;
}

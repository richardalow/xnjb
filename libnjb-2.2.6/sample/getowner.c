#include "common.h"
#include <string.h>

int main (int argc, char **argv)
{
	njb_t njbs[NJB_MAX_DEVICES], *njb;
	int n, opt, debug;
	char *ownerstring;
	extern char *optarg;
	char *lang;

	debug= 0;
	while ( (opt= getopt(argc, argv, "D:")) != -1 ) {
		switch (opt) {
		case 'D':
			debug= atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: getowner [ -D debuglvl ]\n");
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
		NJB_Error_Dump(njb, stderr);
		return 1;
	}

	ownerstring = NJB_Get_Owner_String (njb);
	if (ownerstring != NULL) {
	  printf("Owner string: %s\n", ownerstring);
	  free(ownerstring);
	} else {
	  fprintf(stderr, "could not retrieve owner string\n");
	  NJB_Error_Dump(njb, stderr);
	}

	NJB_Close(njb);
	return 0;
}


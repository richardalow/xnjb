#include "common.h"
/* For isspace() */
#include <ctype.h>

static void usage (void)
{
  fprintf(stderr, "usage: setpbm [ -D debuglvl ] <path>\n");
  exit(1);
}

static void skip_whitespaces (FILE *f) {
  int c;

  while (!feof (f)) {
    c = fgetc (f);
    if (!isspace (c)) {
      if (c == '#') { // Skip comment
	while (!feof (f) && (c = fgetc (f)) != '\n')
	  ;
      }
      else {
	ungetc (c, f);
	break;
      }
    }
  }
}


static int verify_pbm (FILE *f, int x, int y) {
  u_int16_t magic;
  unsigned int width, height;
  int c;

  if (fread (&magic, 1, 2, f) < 2) {
    return -1;
  }

  if (magic != 0x3450) {
    return -1;
  }

  skip_whitespaces(f);
  width = 0;
  fscanf (f, "%u", &width);
  if (width != x) {
    return -1;
  }
  skip_whitespaces (f);
  fscanf (f, "%u", &height);
  if (height != y) {
    return -1;
  }
  c = fgetc (f);
  if (!isspace (c)) {
    return -1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  int n, opt, debug;
  extern int optind;
  extern char *optarg;
  char *path;
  int x, y, bytes;
  
  debug = 0;
  while ( (opt = getopt(argc, argv, "D:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug= atoi(optarg);
      break;
    default:
      usage();
    }
  }

  argc -= optind;
  argv += optind;
  
  if ( argc != 1 ) usage();
  
  if ( debug ) NJB_Set_Debug(debug);

  path= argv[0];
  
  printf("Uploading image:\n");
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

  /* Get the bitmap dimensions */
  if ( NJB_Get_Bitmap_Dimensions(njb, &x, &y, &bytes) == -1 ) {
    printf("This device does not support setting the bitmap.");
    NJB_Error_Dump(njb, stderr);
  } else {
      FILE *f;
      unsigned char data[1088];

      if ( (f = fopen(path, "rb")) == NULL) {
	printf("Could not open bitmap file.\n");
	exit(-1);
      }
      
      if (verify_pbm(f, x, y) == -1) {
	printf("Bad bitmap dimensions or bad file.\n");
	fclose (f);
	exit(-1);
      }

      if ( fread (data, 1, 1088, f) < 1088 ) {
	printf("Could not read in bitmap file.\n");
	fclose (f);
	exit(-1);
      }

      fclose (f);

      if ( NJB_Set_Bitmap(njb, data) == -1 ) {
	NJB_Error_Dump(njb, stderr);
      }
  }
  
  NJB_Release(njb);
  
  NJB_Close (njb);

  return 0;
}

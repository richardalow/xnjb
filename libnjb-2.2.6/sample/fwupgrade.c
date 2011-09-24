/**
 * \file fwupgrade.c
 *
 * This firmware extraction and flashing program was the result
 * of combining finds made by "xchip" from the Nomadness reverse-
 * engineering forum with the sniffer logs made for duplicating
 * the firmware flashing process.
 *
 * "xchip" found out how to remove the XOR scrambling and scan for
 * the zlib compressed image inside a firmware file from Creative.
 *
 */
#include "common.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
/* To handle .exe files, this must exist */
#if HAVE_ZLIB_H
#include <zlib.h>
#endif
/* You need this 10 MB buffer for this operation... */
#define BUFFERSIZE 10*1024*1024

static int progress (u_int64_t sent, u_int64_t total, const char* buf, unsigned len, void *data)
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

static void usage (void)
{
  fprintf(stderr, "usage: fwupgrade [ -D<debuglvl> ] <path>\n");
  exit(1);
}

/**
 * This function reads in the firmware to a largeish buffer.
 */
static int read_in_fw(char *path, unsigned char *buffer) {
  /* File descriptor and pointer */
  int fd;
  size_t bread;

  /* Read in the firmware file */
#ifdef __WIN32__
  if ( (fd = open(path, O_RDONLY|O_BINARY)) == -1 ) {
#else
  if ( (fd = open(path, O_RDONLY)) == -1 ) {
#endif
    printf("Could not open firmware file descriptor.\n");
    return -1;
  }
  bread = read(fd, buffer, BUFFERSIZE);
  if (bread < 0) {
    printf("Error while reading firmware file.\n");
    close(fd);
    return -1;
  }
  
  if (bread == BUFFERSIZE) {
    printf("Warning: this firmware file is very large.\n");
    printf("It probably cannot be properly decoded.\n");
  }
  close(fd);
  printf("Read in a firmware file of size 0x%x.\n", bread);
  if (bread < 1024) {
    printf("Ridiculously small firmware file. Aborting.\n");
    return -1;
  }
  return bread;
}

/**
 * This prints a UCS2 unicode string.
 */
static void ucs2_printf(unsigned char *str) {
  while (!str[0] == 0x00 || !str[1] == 0x00) {
    printf("%c", str[1]);
    str += 2;
  }
}

/**
 * This function will analyze a firmware image
 * and print out some information about it.
 */
static void analyze_firmware(unsigned char *buffer) {
  if (buffer[0] == 'C' && buffer[1] == 'I' &&
      buffer[2] == 'F' && buffer[3] == 'F') {
    u_int32_t cifflen;
    u_int32_t offset = 0;

    cifflen = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    printf("Firmware CIFF image, %08x bytes:\n", cifflen);
    
    /* Wind past header */
    offset = 8;

    printf("   Offset:   Type:  Size:\n");
    while (offset < cifflen) {
      char secname[5];
      u_int32_t seclen;

      secname[0] = buffer[offset];
      secname[1] = buffer[offset+1];
      secname[2] = buffer[offset+2];
      secname[3] = buffer[offset+3];
      secname[4] = '\0';

      seclen = (buffer[offset+4] << 24) | (buffer[offset+5] << 16) | 
	(buffer[offset+6] << 8) | buffer[offset+7];
      
      printf("   %08x  %s   %08x bytes", offset, secname, seclen);
      
      if (!strcmp(secname, "CINF") || !strcmp(secname, "DATA")) {
	printf(" \"");
	ucs2_printf(&buffer[offset+8]);
	printf("\"");
      }
      printf("\n");

      offset += 8;
      offset += seclen;
    }
  } else {
    printf("Unknown firmware image type.\n");
  }
}

#if HAVE_ZLIB_H
static void dexor_fw_image(unsigned char *buffer, 
			   size_t zimglen, 
			   unsigned char *key, 
			   u_int8_t keylen) {
  register u_int8_t j = 0;
  register u_int32_t i;
  register unsigned char c;

  printf("Dexor with key: ");
  for (i = 0; i < keylen; i++) {
    printf("%02x ", key[i]);
  }
  printf("\n");
  for (i = 0; i < zimglen; i++) {
    c = key[j] | 0x80;
    buffer[i] = buffer[i] ^ c;
    j = (i+1) % keylen;
  }
}

/**
 * Here we call zlib to decompress the firmware image.
 */
static void decompress_fw_image(unsigned char *compressed, size_t compressed_len, 
			 unsigned char **decompressed, size_t *rawlen) {
  /* Allocate another big buffer */
  *decompressed = (unsigned char *) malloc(BUFFERSIZE);
  if (*decompressed == NULL) {
    *rawlen = 0;
    return;
  }
  *rawlen = BUFFERSIZE;
  if (uncompress(*decompressed, (uLongf*) rawlen, compressed, compressed_len) != Z_OK) {
    *rawlen = 0;
    return;
  }
}

/**
 * Here we write out the uncompressed file to disk.
 */
static int write_fw_file(char *path, 
		  unsigned char *decompressed,
		  size_t rawlen) {
  int fd = -1;

#ifdef __WIN32__
   if ( (fd = open(path, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 0664)) == -1 ) {
#else
   if ( (fd = open(path, O_CREAT|O_TRUNC|O_WRONLY, 0664)) == -1 ) {
#endif
     printf("Could not open uncompressed file.\n");
     return -1;
   }
   if (write(fd, decompressed, rawlen) == -1) {
     printf("Error while writing uncompressed file.\n");
     close(fd);
     unlink(path);
     return -1;
   }
   close(fd);
   return 0;
}
#endif

/**
 * Returns 0 if OK (yes), 1 if not OK (no)
 */
static int prompt()
{
  char buff[2];
  
  while (1) {
    fprintf(stdout, "> ");
    if ( fgets(buff, sizeof(buff), stdin) == NULL ) {
      if (ferror(stdin)) {
	fprintf(stderr, "File error on stdin\n");
      } else {
	fprintf(stderr, "EOF on stdin\n");
      }
      return 1;
    }
    if (buff[0] == 'y') {
      return 0;
    } else if (buff[0] == 'n') {
      return 1;
    }
  }
}

int main(int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  int n, opt, debug;
  extern int optind;
  extern char *optarg;
  char *path;
  char *sendpath;
  char *lang;
  unsigned char *buffer;
  size_t bread;
  
  debug = 0;
  while ( (opt = getopt(argc, argv, "D:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug = atoi(optarg);
      break;
    default:
      usage();
      break;
    }
  }
  argc -= optind;
  argv += optind;
  
  if ( argc != 1 ) usage();
  
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
  
  path = argv[0];
  
  printf("Analyzing firmware file %s...\n", path);

  /* Allocate a buffer */
  buffer = (unsigned char *) malloc(BUFFERSIZE);
  if (buffer == NULL) {
    printf("Could not allocate a firmware scanning buffer.\n");
    return 1;
  }

  /* Read in to buffer */
  bread = read_in_fw(path, buffer);
  if (bread == -1) {
    return 1;
  }

  /*
   * See if this is a RAW (dexored) firmware image, all
   * firmware images starts with the string "CIFF"
   */
  if (buffer[0] == 'C' && buffer[1] == 'I' && 
      buffer[2] == 'F' && buffer[3] == 'F') {
    printf("This seems to be a raw (dexored) firmware image.\n");
    sendpath = path;
  } else {
#if HAVE_ZLIB_H
    char known_key[] = "SamBanDam";
    char uncompressed_file[] = "tempimage.bin";
    /* Length of the zlib compressed image */
    unsigned int zimglen;
    /* Pointer into file buffer */
    unsigned int offset = 0;
    unsigned char *decompressed;
    size_t rawlen;
    size_t i;

    printf("This could be a zlib compressed windows executable.\n");

    /* Scanning for zlib header */
    printf("Scanning for zlib header...\n");
    for (i = 0; i < bread-3; i++) {
      if (
	  // Old Key
	  (buffer[i] == 0xAB && buffer[i+1] == 0x3B && buffer[i+2] == 0x01) ||
	  // New Key
	  (buffer[i] == 0xCA && buffer[i+1] == 0x69 && buffer[i+2] == 0x0F)
	  )
	{
	  offset = i-4;
	  break;
	}
    }
    if (offset == 0) {
      printf("Could not locate a zlib header in this firmware file.\n");
      return 1;
    } else {
      printf("Found zlib header at file position 0x%x.\n", offset);
    }
    
    zimglen = buffer[offset+3] << 24 | buffer[offset+2] << 16 
      | buffer[offset+1] << 8 | buffer[offset];
    printf("Zlib compressed image length: 0x%x\n", zimglen);
    if (zimglen > bread-offset) {
      printf("Inconsistent length of zlib compressed image, aborting.\n");
      return 1;
    }
    
    /* Decompress zlib image using Zlib */
    printf("Calling zlib uncompress() on raw chunks...\n");
    decompress_fw_image(&buffer[offset+4], zimglen, &decompressed, &rawlen);
    if (rawlen == 0) {
      printf("Failed.\n");

      /* "Dexor" the firmware image */
      printf("\"Dexoring\" firmware zlib image with a known key...\n");
      dexor_fw_image(&buffer[offset+4], zimglen, known_key, strlen(known_key));

      /* Decompress zlib image using Zlib */
      printf("Calling zlib uncompress()...\n");
      decompress_fw_image(&buffer[offset+4], zimglen, &decompressed, &rawlen);
      if (rawlen == 0) {
	printf("Could not dexor the firmware :(\nAborting.\n");
	exit(1);
      }
    }
    printf("Decompressed image size: 0x%x\n", rawlen);
    if (decompressed[0] == 'C' && decompressed[1] == 'I' &&
	decompressed[2] == 'F' && decompressed[3] == 'F') {
      printf("The extracted image looks like a firmware file.\n");
    } else {
      printf("The extracted image does not look like a firmware file.\n");
      printf("Aborting.\n");
      return 1;
    }

    /* Write out decompressed file */
    printf("Writing out the extracted image to disk as \"%s\".\n", uncompressed_file);
    if (write_fw_file(uncompressed_file, decompressed, rawlen) == -1) {
      printf("Failed to write uncompressed file. Aborting.\n");
      return 1;
    }
    
    sendpath = uncompressed_file;

    /* Exchange buffer for the decompressed image */
    free(buffer);
    buffer = decompressed;

#else
    printf("This may be an zlib compressed .exe file firmware.\n");
    printf("You must compile the \"fwupgrade\" program on a system\n");
    printf("which has the zlib library and headers properly installed\n");
    printf("to enable the compression of zlib compressed firmware images.\n");
#endif    
  }
  
  /* Make some more information available on this firmware */
  analyze_firmware(buffer);

  /* Free firmware read buffer */
  free(buffer);
    
  printf("Sending firmware file to jukebox\n");
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

  printf("I will now send the firmware to your device.\n");
  printf("Continue? (y/n)\n");
  if (prompt() == 0) {
    if ( NJB_Send_Firmware(njb, sendpath, progress, NULL) == -1 ) {
      NJB_Error_Dump(njb,stderr);
    } else {
      printf("\nFirmware upload complete.");
    }
    printf("\n");
  } else {
    printf("Aborted.\n");
  }
  
  NJB_Release(njb);
  
  NJB_Close(njb);
  
  return 0;
}

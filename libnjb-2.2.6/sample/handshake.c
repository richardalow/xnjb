#include "common.h"

int main (int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES];
  njb_t *njb;
  int i, n, opt, debug;
  extern char *optarg;
  u_int8_t sdmiid[16];
  
  debug = 0;
  while ( (opt= getopt(argc, argv, "D:")) != -1 ) {
    switch (opt) {
    case 'D':
      debug= atoi(optarg);
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
    fprintf(stderr, "No NJB devices found\n");
    return 1;
  }
  
  printf("Found %u devices.\n", n);
  
  for (i = 0; i < n; i++) {
    njb_keyval_t *key;
    int j;
    int chargestat;
    int auxpowstat;
    const char *devname;
    const char *prodname;

    njb = &njbs[i];
    
    printf("Device %u: ", i+1);
    printf("\n    Player device type: ");

    
    njb = &njbs[i];
    
    if (NJB_Open(njb) == -1) {
      NJB_Error_Dump(njb, stderr);
      return 1;
    }

    if ((devname = NJB_Get_Device_Name(njb, 0)) != NULL) {
      printf("%s\n", devname);
    } else {
      printf("Error getting device name.\n");
      return 1;
    }

    /* Ping the device */
    NJB_Ping(njb);
    
    /* Retrieve the device ID */
    if (NJB_Get_SDMI_ID(njb, (u_int8_t *) &sdmiid) == 0) {
      printf("    Jukebox SDMI ID: ");
      for (j = 0; j < 16; j++) {
	printf("%02X", sdmiid[j]);
      }
      printf("\n");
    } else {
      printf("    Error getting SDMI ID.\n");
    }
    

    if (njb->device_type == NJB_DEVICE_NJB1) {
      u_int8_t major, minor, release;

      if (NJB_Get_Firmware_Revision(njb, &major, &minor, &release) == 0) {
	printf("    Firmware: %u.%u\n", major, minor);
      }
#ifdef __WIN32__
      printf("    Library counter: %I64u\n", NJB_Get_NJB1_Libcounter(njb));
#else
      printf("    Library counter: %llu\n", NJB_Get_NJB1_Libcounter(njb));
#endif
    }
    else {
      u_int8_t major, minor, release;
      
      if (NJB_Get_Firmware_Revision(njb, &major, &minor, &release) == 0) {
	printf("    Firmware: %u.%u.%u\n", major, minor, release);
      }
      if (NJB_Get_Hardware_Revision(njb, &major, &minor, &release) == 0) {
	printf("    Hardware: %u.%u.%u\n", major, minor, release);
      }
    }

    if ((prodname = NJB_Get_Device_Name(njb, 1)) != NULL) {
      printf("    Product name: %s\n", prodname);
    } else {
      printf("    Error getting product name.\n");
      return 1;
    }
    
    /* Auxiliary power status */
    auxpowstat = NJB_Get_Auxpower(njb);
    if (auxpowstat == -1) {
      printf("    Failure getting auxilary power status\n");
      return 1;
    } else if (auxpowstat == 1) {
      printf("    Device is on auxiliary power (AC charger or USB)\n");
    } else if (auxpowstat == 0) {
      printf("    Device is disconnected from auxiliary power\n");      
    }

    /* Battery charging status */
    chargestat = NJB_Get_Battery_Charging(njb);
    if (chargestat == -1) {
      printf("    Failure getting battery charging status\n");
      return 1;
    } else if (chargestat == 1) {
      printf("    Battery is charging\n");
    } else if (chargestat == 0) {
      printf("    Battery is not charging\n");      
    }    
    
    if (njb->device_type != NJB_DEVICE_NJB1) {
      /* This is only well implemented for the series 3 devices. */
      int battery_level = NJB_Get_Battery_Level(njb);
      if (battery_level == -1) {
	printf("    Failure getting battery level\n");
	return 1;
      } else {
	printf("    Battery level: %d%%\n", battery_level);
      }
    }
    
    key = NJB_Get_Keys(njb);
    while (key != NULL) {
      printf("    Device key: %s = %08X, %08X\n", key->key, key->value1, key->value2);
      key = key->next;
    }
    
    NJB_Close(njb);
    
  }
  
  return 0;
}


#include "common.h"

int main(int argc, char **argv)
{
  njb_t njbs[NJB_MAX_DEVICES], *njb;
  extern char *optarg;
  int opt;
  int i, n, debug;
  njb_eax_t *eax;
  
  debug = 0;
  while ((opt = getopt(argc, argv, "D:")) != -1) {
    switch (opt) {
    case 'D':
      debug = atoi(optarg);
      break;
    default:
      fprintf(stderr, "usage: dumpeax [ -D debuglvl ]\n");
      return 1;
    }
  }
  
  if (debug)
    NJB_Set_Debug(debug);
  
  if (NJB_Discover(njbs, 0, &n) == -1) {
    fprintf(stderr, "could not locate any jukeboxes\n");
    exit(1);
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
  
  NJB_Reset_Get_EAX_Type (njb);
  
  while ((eax = NJB_Get_EAX_Type (njb)) != NULL) {
    printf("------------------------------------------\n");
    printf("Effect number: %04X\n", eax->number);
    printf("Effect name: %s\n", eax->name);
    printf("Effect group %d\n", eax->group);
    if (eax->exclusive != 0x00) {
      printf("Effect is exclusive\n");
    }
    if (eax->type == NJB_EAX_FIXED_OPTION_CONTROL) {
      printf("Effect has fixed options:\n");
      printf("    Current selection: %d\n", eax->current_value);
      for(i = 0; i < eax->max_value - eax->min_value + 1; i++) {
	printf("    %d. %s\n", eax->min_value+i, eax->option_names[i]);
      }
    }
    if (eax->type == NJB_EAX_SLIDER_CONTROL) {
      printf("Effect is a slider:\n");
      printf("    Current value: %d\n", eax->current_value);
      printf("    Min value: %d\n", eax->min_value);
      printf("    Max value %d\n", eax->max_value);
    }	  
    NJB_Destroy_EAX_Type (eax);
  }
  
  /* Dump any pending errors */
  NJB_Error_Dump(njb, stderr);
  
  NJB_Release(njb);
  
  NJB_Close(njb);
  return 0;
}

/**
 * \file eax.c
 *
 * This file contains functions specific to manipulate EAX
 * API including other sound settings like volume.
 */
#include <string.h>
#include "libnjb.h"
#include "njb_error.h"
#include "defs.h"
#include "base.h"
#include "byteorder.h"
#include "eax.h"

extern int __sub_depth;

/**
 * This adds a EAX type to the current state, i.e. a linked
 * list associated with the current device and session.
 *
 * @param state the state holder to use
 * @param eax the EAX type to add to the state holder
 */
static void eax_add_to_state(njb_state_t *state, njb_eax_t *eax)
{
  /* Add the new EAX to our state */
  if (state->first_eax == NULL) {
    state->first_eax = eax;
    state->next_eax = NULL;
  } else if (state->next_eax == NULL) {
    state->first_eax->next = eax;
    state->next_eax = eax;
  } else {
    state->next_eax->next = eax;
    state->next_eax = eax;
  }
}

/**
 * Convert a 16-bit integer to a string.
 *
 * @param value the integer to convert
 * @return a string representing the integer
 */
static char *sixteen_to_string_hz(u_int16_t value)
{
  char buffer[16];
  snprintf(buffer, 16, "%d Hz", value);
  buffer[15] = '\0';
  return strdup(buffer);
}

/**
 * This function inpacks an EAX block from the NJB1.
 * The EAX effects are then added to the linked list in
 * the device state and retrieved one by one by the user
 * of the API.
 *
 * @param data raw chunk of data to be converted to EAX
 *             effects
 * @param nbytes the size of the data chunk
 * @param state the device state that holds the linked list
 *              to add the effects to
 * @return 0 on success, -1 on failure
 */
int eax_unpack(void *data, size_t nbytes, njb_state_t *state)
{
  __dsub= "eax_unpack_new_api";
  njb_eax_t *eax, *volumeeax, *mutingeax; 
  unsigned char *dp = (unsigned char *) data;
  u_int16_t frequencies;
  u_int16_t effects;
  u_int16_t effect_amount;
  u_int16_t phonemodes;
  u_int16_t rearmodes;
  int8_t tmp8;
  int i = 0;

  __enter;

  // First delete any old pending EAX types
  // and reset the pointers.
  while (state->next_eax != NULL) {
    njb_eax_t *eax;
    eax = state->next_eax;
    state->next_eax = state->next_eax->next;
    destroy_eax_type(eax);
  }
  state->first_eax = NULL;
  state->next_eax = NULL;

  // Volume EAX
  volumeeax = new_eax_type();
  volumeeax->number = NJB_SOUND_SET_VOLUME;
  volumeeax->name = strdup("Volume");
  volumeeax->group = 0x00;
  volumeeax->type = NJB_EAX_SLIDER_CONTROL;
  volumeeax->current_value = (int16_t) *dp++;
  volumeeax->min_value = 0;
  volumeeax->max_value = 100;
  // Do not add to state until after Muting's been added

  // Muting EAX
  mutingeax = new_eax_type();
  mutingeax->number = NJB_SOUND_SET_MUTING;
  mutingeax->name = strdup("Muted");
  mutingeax->group = 0x00;
  mutingeax->type = NJB_EAX_FIXED_OPTION_CONTROL;
  mutingeax->min_value = 0x0000U;
  mutingeax->max_value = 0x0001U;
  mutingeax->current_value = (int16_t) *dp++;
  mutingeax->option_names = (char **) malloc(2 * sizeof(char *));
  mutingeax->option_names[0] = strdup("Off");
  mutingeax->option_names[1] = strdup("On");

  eax_add_to_state(state, mutingeax);
  eax_add_to_state(state, volumeeax);

  // Equalizer activation EAX
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_EQSTATUS;
  eax->name = strdup("Equalizer active");
  eax->group = 0x01;
  eax->type = NJB_EAX_FIXED_OPTION_CONTROL;
  eax->min_value = 0x0000U;
  eax->max_value = 0x0001U;
  eax->current_value = (int16_t) *dp++;
  eax->option_names = (char **) malloc(2 * sizeof(char *));
  eax->option_names[0] = strdup("Off");
  eax->option_names[1] = strdup("On");
  eax_add_to_state(state, eax);
  
  // Equalizer bass EAX
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_BASS;
  eax->name = strdup("Bass");
  eax->group = 0x01;
  eax->type = NJB_EAX_SLIDER_CONTROL;
  tmp8 = (int8_t) *dp++;
  eax->current_value = (int16_t) tmp8;
  tmp8 = (int8_t) *dp++;
  eax->min_value = (int16_t) tmp8;
  tmp8 = (int8_t) *dp++;
  eax->max_value = (int16_t) tmp8;
  eax_add_to_state(state, eax);

  // Equalizer midrange EAX
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_MIDRANGE;
  eax->name = strdup("Midrange");
  eax->group = 0x01;
  eax->type = NJB_EAX_SLIDER_CONTROL;
  tmp8 = (int8_t) *dp++;
  eax->current_value = (int16_t) tmp8;
  tmp8 = (int8_t) *dp++;
  eax->min_value = (int16_t) tmp8;
  tmp8 = (int8_t) *dp++;
  eax->max_value = (int16_t) tmp8;
  eax_add_to_state(state, eax);

  // Equalizer treble EAX
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_TREBLE;
  eax->name = strdup("Treble");
  eax->group = 0x01;
  eax->type = NJB_EAX_SLIDER_CONTROL;
  tmp8 = (int8_t) *dp++;
  eax->current_value = (int16_t) tmp8;
  tmp8 = (int8_t) *dp++;
  eax->min_value = (int16_t) tmp8;
  tmp8 = (int8_t) *dp++;
  eax->max_value = (int16_t) tmp8;
  eax_add_to_state(state, eax);

  // Equalizer center frequency
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_MIDFREQ;
  eax->name = strdup("Midrange center frequency");
  eax->group = 0x01;
  eax->type = NJB_EAX_FIXED_OPTION_CONTROL;
  eax->min_value = 0x0000U;
  frequencies = (u_int16_t) *dp++;
  eax->max_value = frequencies - 1;
  eax->current_value = (int16_t) *dp++;
  // We have to conjure these strings ourselves
  eax->option_names = (char **) malloc(frequencies * sizeof(char *));
  for (i = 0; i < frequencies; i ++) {
    eax->option_names[i] = sixteen_to_string_hz(njb1_bytes_to_16bit(&dp[0]));
    dp += 2;
  }
  eax_add_to_state(state, eax);

  // EAX Effects
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_EAX;
  eax->name = strdup("EAX effect");
  eax->group = 0x02;
  eax->type = NJB_EAX_FIXED_OPTION_CONTROL;
  eax->min_value = 0x0000U;
  effects = (u_int16_t) *dp++;
  eax->max_value = effects - 1;
  eax->current_value = (int16_t) *dp++;
  // We have to conjure these strings ourselves
  eax->option_names = (char **) malloc(effects * sizeof(char *));
  effect_amount = 0;
  for (i = 0; i < effects; i ++) {
    u_int8_t efflen= (u_int8_t) *dp++;
    
    eax->option_names[i]= (char *) malloc(efflen+1);
    memcpy(eax->option_names[i], dp, efflen);
    eax->option_names[i][efflen] = '\0';
    dp += efflen;
    // This is actually the same for all effects, so we
    // only need one scale value for it (below).
    effect_amount = (int16_t) *dp++;
  }
  eax_add_to_state(state, eax);

  // Also add a scale value for the EAX level
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_EAXAMT;
  eax->name = strdup("EAX effect level");
  eax->group = 0x02;
  eax->type = NJB_EAX_SLIDER_CONTROL;
  eax->min_value = 0;
  eax->max_value = 100;
  eax->current_value = effect_amount;
  eax_add_to_state(state, eax);

  // Headphone modes
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_HEADPHONE;
  eax->name = strdup("Headphone mode");
  eax->group = 0x03;
  eax->type = NJB_EAX_FIXED_OPTION_CONTROL;
  eax->min_value = 0x0000U;
  phonemodes = (u_int16_t) *dp++;
  eax->max_value = phonemodes - 1;
  eax->current_value = (int16_t) *dp++;
  // We have to conjure these strings ourselves
  eax->option_names = (char **) malloc(phonemodes * sizeof(char *));
  for (i = 0; i < phonemodes; i ++) {
    u_int8_t len= (u_int8_t) *dp++;
    
    eax->option_names[i]= (char *) malloc(len+1);
    memcpy(eax->option_names[i], dp, len);
    eax->option_names[i][len] = '\0';
    dp += len;
  }
  eax_add_to_state(state, eax);

  // Rear speaker modes
  eax = new_eax_type();
  eax->number = NJB_SOUND_SET_REAR;
  eax->name = strdup("Rear speaker mode");
  eax->group = 0x03;
  eax->type = NJB_EAX_FIXED_OPTION_CONTROL;
  eax->min_value = 0x0000U;
  rearmodes = (u_int16_t) *dp++;
  eax->max_value = rearmodes - 1;
  eax->current_value = (int16_t) *dp++;
  // We have to conjure these strings ourselves
  eax->option_names = (char **) malloc(rearmodes * sizeof(char *));
  for (i = 0; i < rearmodes; i ++) {
    u_int8_t len= (u_int8_t) *dp++;
    
    eax->option_names[i]= (char *) malloc(len+1);
    memcpy(eax->option_names[i], dp, len);
    eax->option_names[i][len] = '\0';
    dp += len;
  }
  eax_add_to_state(state, eax);

  // At last rewind the EAX list.
  state->next_eax = state->first_eax;

  return 0;
}

/**
 * This function creates a new EAX type holder
 * (allocates memory for it) with no information.
 *
 * @return the new EAX type holder
 */
njb_eax_t *new_eax_type(void)
{
  __dsub = "new_eax_type";
  njb_eax_t *eax; 
  
  __enter;
  
  eax = malloc(sizeof(njb_eax_t));
  if ( eax == NULL ) {
    __leave;
    return NULL;
  }
  
  memset(eax, 0, sizeof(njb_eax_t));
  eax->number = 0;
  eax->name = NULL;
  eax->group = 0;
  eax->exclusive = 0;
  eax->type = NJB_EAX_NO_CONTROL;
  eax->current_value = 0;
  eax->min_value = 0;
  eax->max_value = 0;
  eax->option_names = NULL;
  eax->next = NULL;
  __leave;
  return eax;  
}
/**
 * This function destroys an EAX type holder and free
 * up the memory used by it.
 *
 * @param eax the EAX type holder to destroy
 */
void destroy_eax_type(njb_eax_t *eax)
{
  if (eax == NULL)
    return;
  /* Free the EAX effect name */
  if (eax->name != NULL) {
    free(eax->name);
  }
  /* Free the EAX selection names */
  if (eax->type == NJB_EAX_FIXED_OPTION_CONTROL) {
    u_int8_t i;
    for (i = 0; i < (eax->max_value - eax->min_value); i++) {
      if (eax->option_names[i] != NULL) {
	free(eax->option_names[i]);
      }
    }
    if (eax->option_names != NULL) {
      free(eax->option_names);
    }
  }
  free(eax);
  return;
}

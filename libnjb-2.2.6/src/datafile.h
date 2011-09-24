#ifndef __NJB__DATAFILE__H
#define __NJB__DATAFILE__H

njb_datafile_t *datafile_new (void);
u_int64_t datafile_size (njb_datafile_t *df);
void datafile_set_size (njb_datafile_t *df, u_int64_t size);
void datafile_set_time (njb_datafile_t *df, time_t ts);
int datafile_set_name (njb_datafile_t *df, const char *filename);
int datafile_set_folder (njb_datafile_t *df, const char *folder);
njb_datafile_t *datafile_unpack (unsigned char *data, size_t nbytes);
unsigned char *datafile_pack (njb_datafile_t *df, u_int32_t *size);
unsigned char *datafile_pack3 (njb_t *njb, njb_datafile_t *df, u_int32_t *size);
unsigned char *new_folder_pack3 (njb_t *njb, const char *name, u_int32_t *size);

#endif

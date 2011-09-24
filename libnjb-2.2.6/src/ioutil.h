#ifndef __NJB__IOUTIL__H
#define __NJB__IOUTIL__H

void data_dump(FILE *f, void *buf, size_t nbytes);
void data_dump_ascii (FILE *f, void *buf, size_t n, size_t dump_boundry);

#endif

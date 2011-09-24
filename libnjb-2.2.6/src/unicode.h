#ifndef __NJB__UNICODE__H
#define __NJB__UNICODE__H

void njb_set_unicode (int flag);
int ucs2strlen(const unsigned char *unicstr);
char *strtoutf8(const unsigned char *str);
char *utf8tostr(const unsigned char *str);
char *ucs2tostr(const unsigned char *unicstr);
unsigned char *strtoucs2(const unsigned char *str);

#endif /* __NJB__UNICODE__H */

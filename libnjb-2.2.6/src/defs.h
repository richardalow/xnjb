#ifndef __NJB__DEFS__H
#define __NJB__DEFS__H

/* Takes out GCC weirdness for other compilers */
#ifndef __GNUC__
#  define  __attribute__(x)  /*NOTHING*/
#endif

/* Macros for printing debug traces from subroutines */
#define __dsub static char *subroutinename __attribute__((unused))
#define __sub subroutinename
#define __enter if(njb_debug(DD_SUBTRACE))fprintf(stderr,"%*s==> %s\n",3*__sub_depth++,"",__sub)
#define __leave if(njb_debug(DD_SUBTRACE))fprintf(stderr,"%*s<== %s\n",3*(--__sub_depth),"",__sub)

#endif

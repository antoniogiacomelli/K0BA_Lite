/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 *  In this header:
 *                  o Kernel Version record definition
 *                  xx.xx.xx
 *                  major minor patch
 *
 *****************************************************************************/
#ifndef KVERSION_H
#define KVERSION_H

/*** Minimal valid version **/
/** This is to manage API retrocompatibilities */
#define K_DEF_MINIMAL_VER        0U

extern struct kversion const KVERSION;

#if (K_DEF_MINIMAL_VER==0U) /* there is no retrocompatible version */
                            /* the valid is the current            */
#define K_VALID_VERSION (unsigned)((KVERSION.major << 16 | KVERSION.minor << 8 \
            | KVERSION.patch << 0))

#else

#define K_VALID_VERSION K_DEF_MINIMAL_VER

#endif

struct kversion
{
    unsigned char major;
    unsigned char minor;
    unsigned char patch;
};

unsigned int kGetVersion(void);

#endif /* KVERSION_H */

/**
 * \file
 * \author T. Canham
 * \brief Defines ISF basic types
 *
 * \copyright
 * Copyright 2009-2016, by the California Institute of Technology.
 * ALL RIGHTS RESERVED.  United States Government Sponsorship
 * acknowledged.
 *
 */

#include <Fw/Types/BasicTypes.hpp>

#if defined __VXWORKS__ || defined __TI_ARM__ // Neither VxWorks nor TI ARM have strnlen
NATIVE_INT_TYPE strnlen(const char *s, NATIVE_INT_TYPE maxlen) {
    // walk buffer looking for NULL
    for (NATIVE_INT_TYPE index = 0; index < maxlen; index++) {
        if (0 == s[index]) {
            return index+1;
        }
    }
    return maxlen;
}
#endif


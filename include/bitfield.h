#pragma once

// Reorders bits based on endianness to maintain MSB first order
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define BITFIELD(...)  __VA_ARGS__
#else
    // For little endian, reverse the field order
    #define _BITFIELD_REV2(a,b)                b,a
    #define _BITFIELD_REV3(a,b,c)              c,b,a
    #define _BITFIELD_REV4(a,b,c,d)            d,c,b,a
    #define _BITFIELD_REV5(a,b,c,d,e)          e,d,c,b,a
    #define _BITFIELD_REV6(a,b,c,d,e,f)        f,e,d,c,b,a
    
    #define _COUNT_ARGS(...) _COUNT_ARGS_(__VA_ARGS__,6,5,4,3,2,1)
    #define _COUNT_ARGS_(_1,_2,_3,_4,_5,_6,N,...) N
    
    #define _BITFIELD_SELECT(count) _BITFIELD_REV##count
    #define _BITFIELD_REORDER(count, ...) _BITFIELD_SELECT(count)(__VA_ARGS__)
    
    #define BITFIELD(...) _BITFIELD_REORDER(_COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)
#endif

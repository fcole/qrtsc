#ifdef WIN32
	#include <io.h> /* Needed by MS non-standard _mktemp for ISO mktemp */
	#define mktemp _mktemp
#endif

#define HAVE_ZLIB 1
#define STDC_HEADERS 1

#define MATIO_PLATFORM "i686-pc-win32"
#define MATIO_MAJOR_VERSION 1
#define MATIO_MINOR_VERSION 3
#define MATIO_RELEASE_LEVEL 2

#define SIZEOF_DOUBLE 8
#define SIZEOF_FLOAT 4
#define SIZEOF_LONG 4
#define SIZEOF_INT 4
#define SIZEOF_SHORT 2
#define SIZEOF_CHAR 1
#define SIZEOF_VOID_PTR 4

#define HAVE_MAT_INT64_T 0
#define HAVE_MAT_UINT64_T 0
#define HAVE_MAT_INT32_T 1
#define HAVE_MAT_UINT32_T 1
#define HAVE_MAT_INT16_T 1
#define HAVE_MAT_UINT16_T 1
#define HAVE_MAT_INT8_T 1
#define HAVE_MAT_UINT8_T 1

#define _mat_uint8_t  unsigned char
#define _mat_uint16_t unsigned short
#define _mat_uint32_t unsigned int
#define _mat_uint64_t unsigned long long
#define _mat_int8_t   signed char
#define _mat_int16_t  signed short
#define _mat_int32_t  signed int
#define _mat_int64_t  signed long long

#define EXTENDED_SPARSE 1

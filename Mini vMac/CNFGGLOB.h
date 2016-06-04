#include <stdint.h>
#include <unistd.h>

#define BigEndianUnaligned 0
#define LittleEndianUnaligned 1
#define MayInline inline
#define MayNotInline __attribute__((noinline))
#define SmallGlobals 0
#define cIncludeUnused 0
#define UnusedParam(p) (void) p

/* --- integer types ---- */

typedef uint8_t ui3b;
#define HaveRealui3b 1

typedef int8_t si3b;
#define HaveRealsi3b 1

typedef uint16_t ui4b;
#define HaveRealui4b 1

typedef int16_t si4b;
#define HaveRealsi4b 1

typedef uint32_t ui5b;
#define HaveRealui5b 1

typedef int32_t si5b;
#define HaveRealsi5b 1

#define HaveRealui6b 0
#define HaveRealsi6b 0

/* --- integer representation types ---- */

typedef ui3b ui3r;
#define ui3beqr 1

typedef si3b si3r;
#define si3beqr 1

typedef ui4b ui4r;
#define ui4beqr 1

typedef si4b si4r;
#define si4beqr 1

typedef ui5b ui5r;
#define ui5beqr 1

typedef si5b si5r;
#define si5beqr 1

typedef int64_t si6r;
typedef int64_t si6b;
typedef uint64_t ui6r;
typedef uint64_t ui6b;
#define LIT64(a) a##ULL

/* --- shared config for all variants ---- */

#define NeedIntlChars 1
#define kStrAppName "Mini vMac"
#define kAppVariationStr "minivmac-3.4.0-ios"
#define kStrCopyrightYear "2016"
#define kMaintainerName "Jesús A. Álvarez"
#define kStrHomePage "https://namedfork.net/minivmac"
#define VarFullScreen 0
#define WantInitFullScreen 0
#define MayFullScreen 1
#define WantInitMagnify 0
#define WantInitRunInBackground 0
#define NeedRequestInsertDisk 0
#define NeedDoMoreCommandsMsg 0
#define NeedDoAboutMsg 0
#define UseControlKeys 0
#define UseActvCode 0
#define EnableDemoMsg 0

#define IncludePbufs 1
#define NumPbufs 4

#define dbglog_HAVE 0

#define EnableMouseMotion 1

#define IncludeHostTextClipExchange 1
#define IncludeSonyRawMode 1
#define IncludeSonyGetName 1
#define IncludeSonyNew 1
#define IncludeSonyNameNew 1

#define EmLocalTalk 0


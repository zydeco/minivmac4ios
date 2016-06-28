#define RomFileName "MacII.ROM"

#define WantInitSpeedValue 2
#define WantInitNotAutoSlow 0
#define EnableAutoSlow 0

#define MySoundEnabled 1
#define MySoundRecenterSilence 0
#define kLn2SoundSampSz 3

#define NumDrives 8

#define vMacScreenHeight 480
#define vMacScreenWidth 640
#define vMacScreenDepth 3

#define kROM_Size 0x00040000

#ifdef PLIST_PREPROCESSOR
#define MNVMBundleDisplayName Mac II (640×480)
#define MNVMBundleGetInfoString 8M, 640×480
#else
#include "../CNFGGLOB.h"
#endif

#define RomFileName "MacII.ROM"

#define WantInitSpeedValue 2
#define WantInitNotAutoSlow 0
#define EnableAutoSlow 0

#define MySoundEnabled 1
#define MySoundRecenterSilence 0
#define kLn2SoundSampSz 3

#define NumDrives 8

#define vMacScreenHeight 768
#define vMacScreenWidth 1024
#define vMacScreenDepth 3

#define kROM_Size 0x00040000

#ifdef PLIST_PREPROCESSOR
#define MNVMBundleDisplayName Mac II
#define MNVMBundleGetInfoString 8M, 1024×768
#else
#include "../CNFGGLOB.h"
#endif

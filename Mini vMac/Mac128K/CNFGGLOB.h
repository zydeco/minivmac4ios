#define RomFileName "Mac128K.ROM"

#define WantInitSpeedValue 2
#define WantInitNotAutoSlow 0
#define EnableAutoSlow 1

#define MySoundEnabled 1
#define MySoundRecenterSilence 0
#define kLn2SoundSampSz 3

#define NumDrives 2

#define vMacScreenHeight 342
#define vMacScreenWidth 512
#define vMacScreenDepth 0

#define kROM_Size 0x00010000

#ifdef PLIST_PREPROCESSOR
#define MNVMBundleDisplayName Mac 128K
#define MNVMBundleGetInfoString 128K, 512×342
#else
#include "../CNFGGLOB.h"
#endif

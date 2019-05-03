/*
	STRCNENG.h

	Copyright (C) 2006 Paul C. Pratt, Pierre Lemieux

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

/*
	STRing CoNstants for ENGlish

	Pierre Lemieux provided some corrections and suggestions.
*/

#define kStrAboutTitle "About"
#define kStrAboutMessage "To display information about this program, use the ;]A;} command of the ^p Control Mode. To learn about the Control Mode, see the ;[More Commands;ll;{ item in the ;[Special;{ menu."

#define kStrMoreCommandsTitle "More commands are available in the ^p Control Mode."
#define kStrMoreCommandsMessage "To enter the Control Mode, press and hold down the ;]^c;} key. You will remain in the Control Mode until you release the ;]^c;} key. Type ;]H;} in the Control Mode to list available commands."

#define kStrTooManyImagesTitle "Too many Disk Images"
#define kStrTooManyImagesMessage "I can not mount that many Disk Images. Try ejecting one."

#define kStrImageInUseTitle "Disk Image in use"
#define kStrImageInUseMessage "I can not mount the Disk Image because it is already in use by another application or already open in ^p."

#define kStrOutOfMemTitle "Not enough Memory"
#define kStrOutOfMemMessage "There is not enough memory available to launch ^p."

#define kStrNoROMTitle "Unable to locate ROM image"
#define kStrNoROMMessage "I can not find the ROM image file ;[^r;{. For more information, see: ;[^w;{."

#define kStrCorruptedROMTitle "ROM checksum failed"
#define kStrCorruptedROMMessage "The ROM image file ;[^r;{ may be corrupted."

#define kStrUnsupportedROMTitle "Unsupported ROM"
#define kStrUnsupportedROMMessage "The ROM image file ;[^r;{ loaded successfully, but I don;}t support this ROM version."

#define kStrQuitWarningTitle "Please shut down the emulated computer before quitting."
#define kStrQuitWarningMessage "To force ^p to quit, at the risk of corrupting the mounted disk image files, use the ;]Q;} command of the ^p Control Mode. To learn about the Control Mode, see the ;[More Commands;ll;{ item in the ;[Special;{ menu."

#define kStrReportAbnormalTitle "Abnormal Situation"
#define kStrReportAbnormalMessage "The emulated computer is attempting an operation that wasn;}t expected to happen in normal use."

#define kStrBadArgTitle "Unknown argument"
#define kStrBadArgMessage "I did not understand one of the command line arguments, and ignored it."

#define kStrOpenFailTitle "Open failed"
#define kStrOpenFailMessage "I could not open the disk image."

#define kStrNoReadROMTitle "Unable to read ROM image"
#define kStrNoReadROMMessage "I found the ROM image file ;[^r;{, but I can not read it."

#define kStrShortROMTitle "ROM image too short"
#define kStrShortROMMessage "The ROM image file ;[^r;{ is shorter than it should be."

/* state of a boolean option */
#define kStrOn "on"
#define kStrOff "off"

/* state of a key */
#define kStrPressed "pressed"
#define kStrReleased "released"

/* state of Stopped */
#define kStrStoppedOn kStrOn
#define kStrStoppedOff kStrOff

/* About Screen */
#define kStrProgramInfo "^v"
#define kStrSponsorIs "^v. This variation is made for:"
#define kStrWorkOfMany "Copyright ^y. ^p contains the work of many people. This version is maintained by:"
#define kStrForMoreInfo "For more information, see:"
#define kStrLicense "^p is distributed under the terms of the GNU Public License, version 2."
#define kStrDisclaimer " ^p is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;ls without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."

/* Help Screen */
#define kStrHowToLeaveControl "To leave the Control Mode, release the ;]^c;} key."
#define kStrHowToPickACommand "Otherwise, type a letter. Available commands are:"
#define kStrCmdAbout "About (version information)"
#define kStrCmdOpenDiskImage "Open disk image;ll"
#define kStrCmdQuit "Quit"
#define kStrCmdSpeedControl "Speed control;ll (^s)"
#define kStrCmdMagnifyToggle "Magnify toggle (^g)"
#define kStrCmdFullScrnToggle "Full screen toggle (^f)"
#define kStrCmdCtrlKeyToggle "emulated ;]^m;} Key toggle (^k)"
#define kStrCmdReset "Reset"
#define kStrCmdInterrupt "Interrupt"
#define kStrCmdHelp "Help (show this page)"

/* Speed Control Screen */
#define kStrCurrentSpeed "Current speed: ^s"
#define kStrSpeedAllOut "All out"
#define kStrSpeedStopped "stopped toggle (^h)"
#define kStrSpeedBackToggle "run in Background toggle (^b)"
#define kStrSpeedAutoSlowToggle "autosloW toggle (^l)"
#define kStrSpeedExit "Exit speed control"

#define kStrNewSpeed "Speed: ^s"
#define kStrSpeedValueAllOut kStrSpeedAllOut

#define kStrNewStopped "Stopped is ^h."
#define kStrNewRunInBack "Run in background is ^b."
#define kStrNewAutoSlow "AutoSlow is ^l."

#define kStrNewMagnify "Magnify is ^g."

#define kStrNewFullScreen "Full Screen is ^f."

#define kStrNewCntrlKey "Emulated ;]^m;} key ^k."

#define kStrCmdCancel "cancel"

#define kStrConfirmReset "Are you sure you want to reset the emulated computer? Unsaved changes will be lost, and there is a risk of corrupting the mounted disk image files. Type a letter:"
#define kStrResetDo "reset"
#define kStrResetNo kStrCmdCancel

#define kStrHaveReset "Have reset the emulated computer"

#define kStrCancelledReset "Reset cancelled"

#define kStrConfirmInterrupt "Are you sure you want to interrupt the emulated computer? This will invoke any installed debugger. Type a letter:"
#define kStrInterruptDo "interrupt"
#define kStrInterruptNo kStrCmdCancel

#define kStrHaveInterrupted "Have interrupted the emulated computer"

#define kStrCancelledInterrupt "Interrupt cancelled"

#define kStrConfirmQuit "Are you sure you want to quit ^p? You should shut down the emulated computer before quitting to prevent corrupting the mounted disk image files. Type a letter:"
#define kStrQuitDo "quit"
#define kStrQuitNo kStrCmdCancel

#define kStrCancelledQuit "Quit cancelled"

#define kStrModeConfirmReset "Control Mode : Confirm Reset"
#define kStrModeConfirmInterrupt "Control Mode : Confirm Interrupt"
#define kStrModeConfirmQuit "Control Mode : Confirm Quit"
#define kStrModeSpeedControl "Control Mode : Speed Control"
#define kStrModeControlBase "Control Mode (Type ;]H;} for help)"
#define kStrModeControlHelp "Control Mode"
#define kStrModeMessage "Message (Type ;]C;} to continue)"

#define kStrMenuFile "File"
#define kStrMenuSpecial "Special"
#define kStrMenuHelp "Help"

#define kStrMenuItemAbout "About ^p"
#define kStrMenuItemOpen "Open Disk Image"
#define kStrMenuItemQuit "Quit"
#define kStrMenuItemMore "More Commands"

#define kStrAppMenuItemHide "Hide ^p"
#define kStrAppMenuItemHideOthers "Hide Others"
#define kStrAppMenuItemShowAll "Show All"
#define kStrAppMenuItemQuit "Quit ^p"

#define kStrCmdCopyOptions "copy variation options"
#define kStrHaveCopiedOptions "Variation options copied"

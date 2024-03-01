# Mini vMac for iOS

## Features

* Emulates Mac Plus, Mac II or Mac 128K
* Full simulated keyboard (including all Mac keys)
* Full sound output
* Uses external keyboard and mouse/trackpad if available
* Regulable emulation speed
* Easy(ish) to import/export disk images

## Requirements

* iOS/iPadOS 13.4 or later, or visionOS
    * Previous versions support down to iOS 7
* ROM image from Mac Plus, Mac II and/or Mac 128K
* Disk images with Mac software

## Usage

### ROM and Disk Images

You can import the ROM (`vMac.ROM`) and disk images (with `.dsk` or `.img`
extension) into Mini vMac from other apps (iCloud Storage, Dropbox, etc), using
AirDrop, iTunes File Sharing or the iOS Files app.

* To insert disk images, swipe left with two fingers and the list of disks will
    appear. Icons are automatically generated based on the contents of the disk.
* Tap an hold on a file to delete, rename or share
* Tap the Edit button to show all files, and the Create Disk Image option

The disk image menu will attempt to find an icon from the following sources (in
descending order of priority):

1. Volume icon (System 7)
2. Application icon, if the disk contains only one application
3. Application matching the name of the volume (not the disk image)
4. Application with name written in the comment field of the volume

If no icon is found, it will show a standard floppy disk icon.

### Keyboard

Swipe up with two fingers to show the keyboard, and down to hide it.

The emulated keyboard features all the keys on an Apple Extended keyboard
(except the Power key). The Command, Option, Control and Shift keys are
sticky, to make keyboard shortcuts easier to type. You can change the
appearance of the emulated keyboard in the settings.

If you have an external keyboard attached, you can use it too, although some
shortcuts may interfere with iOS.

### Mouse

You can choose to use the screen as a touchscreen, where tapping on the screen
acts a mouse click, or as a trackpad. In trackpad mode, dragging is done by
tapping twice fast and holding it down. 3D touch can also be used for clicking and
dragging on supported devices.

If you use a mouse or trackpad on iPad OS 13.4 or newer, it will be used automatically.

### Settings

Swipe right with two fingers to show the settings dialog, where you can change
the following:

* **Speed:** make the emulated machine faster than a Mac Plus
* **Mouse Type:** switch between touchscreen and trackpad mode
* **Keyboard Layout:** change the layout of the emulated keyboard
* **Display Scaling:** choose how to scale the display
* **Emulated Machine:** changes won't take effect if there are disks inserted

## Installation

The easiest way is to install an IPA from [releases](https://github.com/zydeco/minivmac4ios/releases) with [AltStore](https://altstore.io).

### Building from source

To build the project, make sure you check out the repository and submodules:

```
git clone https://github.com/zydeco/minivmac4ios.git
cd minivmac4ios
git submodule update --init --recursive
```

The folders `minivmac`, `libmfs` and `libres` should not be empty. The source code zip file you can download from GitHub does not include all dependencies.

To install run the app on your device with Xcode, you'll to perform some [additoinal steps](https://developer.apple.com/documentation/xcode/enabling-developer-mode-on-a-device):

* Pair the device with Xcode
* Specify your Apple ID in the Account preferences in Xcode.
* Change the Team (to your own) and Bundle Identifier (for example, change the `net.namedfork` prefix to your own) in the “Mini vMac” target, under the “Signing & Capabilities” tab.
* Enable [Developer Mode](https://developer.apple.com/documentation/xcode/enabling-developer-mode-on-a-device) on your device (iOS 16 and later).
* On older iOS versions, trust your developer profile in the iOS Settings (under General > Device Management) after installing the app for the first time.

## Adding new emulated machines

See documentation [on the wiki](https://github.com/zydeco/minivmac4ios/wiki/Adding-a-new-emulated-machine).

## Credits

* Mini vMac for iOS by [Jesús A. Álvarez](https://github.com/zydeco)
* [Mini vMac](http://gryphel.com/c/minivmac/) ©2001-2021 Paul C. Pratt
* [vMac](http://vmac.org/) ©1996-1999 Philip Cummins & al.
* [hfsutils](http://www.mars.org/home/rob/proj/hfs/) ©1996-2000 Robert Leslie
* [libmfs](https://github.com/zydeco/libmfs) and [libres](https://github.com/zydeco/libres) by [Jesús A. Álvarez](https://github.com/zydeco)

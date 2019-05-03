# Mini vMac for iOS

## Features

* Emulates Mac Plus, Mac II or Mac 128K
* Full simulated keyboard (including all Mac keys)
* Full sound output
* Uses external keyboard if available
* Regulable emulation speed
* Easy(ish) to import/export disk images

## Requirements

* iOS 7 or later
* ROM image from Mac Plus, Mac II and/or Mac 128K
* Disk images with Mac software

## Usage

### ROM and Disk Images

You can import the ROM (`vMac.ROM`) and disk images (with `.dsk` or `.img`
extension) into Mini vMac from other apps (iCloud Storage, Dropbox, etc), using
AirDrop, or via iTunes File Sharing.

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
tapping twice fast and holding it down.

If you have a jailbroken device and [BTC Mouse & Trackpad](http://www.ringwald.ch/cydia/mouse/),
Mini vMac will use your bluetooth mouse or trackpad.

### Settings

Swipe right with two fingers to show the settings dialog, where you can change
the following:

* **Speed:** make the emulated machine faster than a Mac Plus
* **Mouse Type:** switch between touchscreen and trackpad mode
* **Keyboard Layout:** change the layout of the emulated keyboard
* **Emulated Machine:** for changes to take effect, close and relaunch Mini vMac

### Credits

* Mini vMac for iOS by [Jesús A. Álvarez](https://github.com/zydeco)
* [Mini vMac](http://gryphel.com/c/minivmac/) ©2001-2019 Paul C. Pratt
* [vMac](http://vmac.org/) ©1996-1999 Philip Cummins & al.
* [hfsutils](http://www.mars.org/home/rob/proj/hfs/) ©1996-2000 Robert Leslie
* [libmfs](https://github.com/zydeco/libmfs) and [libres](https://github.com/zydeco/libres) by [Jesús A. Álvarez](https://github.com/zydeco)

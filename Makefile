XCODEBUILD=xcodebuild
PROJECT="Mini vMac.xcodeproj"
SCHEME="Mini vMac"
CONFIGURATION=Release
APP="build/Build/Products/$(CONFIGURATION)-iphoneos/Mini vMac.app"
VERSION=`xpath 2>/dev/null Mini\ vMac/Info.plist "/plist/dict/key[.='CFBundleShortVersionString']/following-sibling::*[1]/text()"`
LDID=ldid

deb: $(APP)
	rm -rf build/deb
	mkdir -p build/deb/{Applications,DEBIAN}
	cp -r $(APP) build/deb/Applications/
	rm -f "build/deb/Applications/Mini vMac.app/embedded.mobileprovision"
	find build/deb/Applications -name _CodeSignature -exec rm -rvf {} +
	find build/deb/Applications -type f -perm -111 -exec $(LDID) {} \;
	cp apt-control build/deb/DEBIAN/control
	echo Installed-Size: `du -ck build/deb | tail -1 | cut -f 1` >> build/deb/DEBIAN/control
	echo Version: $(VERSION) >> build/deb/DEBIAN/control
	COPYFILE_DISABLE="" COPY_EXTENDED_ATTRIBUTES_DISABLE="" dpkg-deb -Zgzip -b build/deb build/minivmac4ios-$(VERSION).deb

clean:
	rm -rf build

$(APP): 
	$(XCODEBUILD) -project $(PROJECT) -scheme $(SCHEME) -configuration $(CONFIGURATION) -derivedDataPath build

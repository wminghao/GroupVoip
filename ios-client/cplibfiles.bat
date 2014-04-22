mkdir vlclibs
cd vlclibs
mkdir Release-iphoneos
cp -r /Users/wminghao/Develop/vlc-iOS/VLCKit/build/Release-iphoneos/libMobileVLCKit.a Release-iphoneos
mkdir Release-iphonesimulator
cp -r /Users/wminghao/Develop/vlc-iOS/VLCKit/build/Release-iphonesimulator/libMobileVLCKit.a Release-iphonesimulator
mkdir Debug-iphoneos
cp -r /Users/wminghao/Develop/vlc-iOS/VLCKit/build/Debug-iphoneos/libMobileVLCKit.a Debug-iphoneos
mkdir Debug-iphonesimulator
cp -r /Users/wminghao/Develop/vlc-iOS/VLCKit/build/Debug-iphonesimulator/libMobileVLCKit.a Debug-iphonesimulator
cp -r /Users/wminghao/Develop/vlc-iOS/VLCKit/build/Release-iphoneos/include .
cd ..

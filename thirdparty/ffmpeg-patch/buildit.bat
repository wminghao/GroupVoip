echo "===start rebuilding ffmpeg==="
echo "===remove the .sum-ffmpeg files to force rebuild==="
rm ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-armv7/.sum-ffmpeg
rm ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-armv7s/.sum-ffmpeg
rm ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-arm64/.sum-ffmpeg
rm ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneSimulator-x86_64/.sum-ffmpeg
rm ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneSimulator-i386/.sum-ffmpeg
echo "===copy libavformat files to the target==="
cp libav-HEAD-254c95c/libavformat/flv.h ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneSimulator-i386/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flv.h ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneSimulator-x86_64/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flv.h ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-arm64/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flv.h ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-armv7/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flv.h ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-armv7s/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flvdec.c ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneSimulator-i386/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flvdec.c ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneSimulator-x86_64/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flvdec.c ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-arm64/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flvdec.c ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-armv7/ffmpeg/libavformat/
cp libav-HEAD-254c95c/libavformat/flvdec.c ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/iPhoneOS-armv7s/ffmpeg/libavformat/
echo "===tar ball ffmpeg==="
tar -czvf ffmpeg-HEAD.tar.gz libav-HEAD-254c95c
cp ffmpeg-HEAD.tar.gz ~/Develop/vlc-iOS/VLCKit/MobileVLCKit/ImportedSources/vlc/contrib/tarballs/
echo "===finished rebuilding ffmpeg==="

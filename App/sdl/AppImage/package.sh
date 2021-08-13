# https://github.com/AppImage/docs.appimage.org/blob/master/source/packaging-guide/manual.rst

MYAPP=HPRDP

gcc -o main ../main.c `pkg-config --libs --cflags mpv sdl2` -std=c99

mkdir -p $MYAPP.AppDir/
# wget https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64 -O $MYAPP.AppDir/AppRun
chmod +x $MYAPP.AppDir/AppRun
mv $MYAPP.desktop $MYAPP.AppDir/
mv myapp.png $MYAPP.AppDir/
mkdir -p $MYAPP.AppDir/usr/bin/
mv main $MYAPP.AppDir/usr/bin/myapp
mkdir -p $MYAPP.AppDir/usr/lib/
for file in $(ldd ../main | cut -d' ' -f3); do cp $file $MYAPP.AppDir/usr/lib/; done

mkdir _out
cd _out
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -O appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage
./appimagetool-x86_64.AppImage ../$MYAPP.AppDir/

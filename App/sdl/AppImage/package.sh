# https://github.com/AppImage/docs.appimage.org/blob/master/source/packaging-guide/manual.rst

PASS_LIBS=passlibs.txt
LDD_LIBS=ldd_paths.txt
EXCLUDE_LIST=excludelist
MAIN=chimera

# Build the program
gcc -o $MAIN ../main.c `pkg-config --libs --cflags mpv sdl2 SDL2_ttf` -std=c99
if [[ $? -ne 0 ]]; then return; fi

# Filter out breaking shared libs
wget https://raw.githubusercontent.com/AppImage/pkg2appimage/master/excludelist -O $EXCLUDE_LIST
ldd $MAIN|tr -d '\t'|grep -v "^/"|cut -d' ' -f 3 > $LDD_LIBS
chmod +x cleanup_libs.py
./cleanup_libs.py -i $LDD_LIBS -e $EXCLUDE_LIST -o $PASS_LIBS

chmod +x $MAIN
chmod +x AppDir/AppRun

mkdir -p AppDir/usr/bin/
mkdir -p AppDir/usr/lib/
mv $MAIN AppDir/usr/bin/
for file in $(cat $PASS_LIBS); do cp $file AppDir/usr/lib/; done

mkdir _out
cd _out
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -O appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage
./appimagetool-x86_64.AppImage ../AppDir/

# Cleanup
cd ..
rm $PASS_LIBS
rm $LDD_LIBS
rm $EXCLUDE_LIST

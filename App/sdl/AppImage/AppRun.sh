#!/bin/bash

# Credits to Simon Peter: https://gitlab.com/inkscape/inkscape/-/blob/158a1947ebe96272f9cbf6608c6696869979ae22/packaging/appimage/AppRun#L63

MAIN="$APPDIR/usr/bin/myapp" ; shift
BINARY_NAME=$(basename "$ARGV0")

# Prefer to run the bundled executables (e.g., Python)
export PATH="${APPDIR}/usr/bin:${PATH}"

# Bundle everything if a private ld-linux-x86-64.so.2 is there

if [ -e "$APPDIR/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2" ] ; then
  echo "Run experimental bundle that bundles everything"
  export GCONV_PATH="$APPDIR/usr/lib/x86_64-linux-gnu/gconv"
  export FONTCONFIG_FILE="$APPDIR/etc/fonts/fonts.conf"
  export LIBRARY_PATH="$APPDIR/usr/lib":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/lib":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/usr/lib/i386-linux-gnu":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/lib/i386-linux-gnu":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/usr/lib/i386-linux-gnu/pulseaudio":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/usr/lib/i386-linux-gnu/alsa-lib":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/usr/lib/x86_64-linux-gnu":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/lib/x86_64-linux-gnu":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/usr/lib/x86_64-linux-gnu/pulseaudio":$LIBRARY_PATH
  export LIBRARY_PATH="$APPDIR/usr/lib/x86_64-linux-gnu/alsa-lib":$LIBRARY_PATH
  export GI_TYPELIB_PATH="${APPDIR}/usr/lib/x86_64-linux-gnu/girepository-1.0"
  export LIBRARY_PATH=$GDK_PIXBUF_MODULEDIR:$LIBRARY_PATH
  export LD_LIBRARY_PATH="$LIBRARY_PATH" 
  exec "${APPDIR}/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2" --inhibit-cache --library-path "${LIBRARY_PATH}" "${MAIN}" "$@"
else
  exec "${MAIN}" "$@"
fi


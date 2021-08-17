#!/bin/bash

# Download linuxdeploy and conda plugin
mkdir _out
wget -c "https://raw.githubusercontent.com/TheAssassin/linuxdeploy-plugin-conda/master/linuxdeploy-plugin-conda.sh" -P _out/
wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -P _out/
# we need python 3.7 or lower for this plugin to work due to a later deprecation
sed -i '/miniconda_url=.*/i \    miniconda_installer_filename=Miniconda3-py37_4.10.3-Linux-x86_64.sh' _out/linuxdeploy-plugin-conda.sh
chmod +x _out/linuxdeploy-x86_64.AppImage _out/linuxdeploy-plugin-conda.sh

mkdir -p AppDir/usr/bin
cp ../client.sh AppDir/usr/bin/
chmod +x AppDir/usr/bin/client.sh
cp ../client/src/client.py AppDir/usr/bin/

cat > kvm-client.desktop <<\EOF
[Desktop Entry]
Version=1.0
Name=ChimeraDesk-KVM
Icon=keyboard
Exec=client.sh
Categories=Network;Utility;System;
Terminal=true
Type=Application
StartupNotify=true
MimeType=application/x-extension-fcstd;
EOF

export CONDA_PACKAGES="pip"
# export PIP_REQUIREMENTS="-e ../../client/"
export PIP_REQUIREMENTS="pynput"
export CONDA_PYTHON_VERSION="3.7"
cd _out
./linuxdeploy-x86_64.AppImage --appdir ../AppDir -d ../kvm-client.desktop -i ../keyboard.png --plugin conda --output appimage
cd ..

# Clean up 
rm kvm-client.desktop
rm -rf AppDir

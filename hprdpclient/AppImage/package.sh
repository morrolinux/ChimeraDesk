# Clean up 
rm -rf AppDir

# Download linuxdeploy and conda plugin
wget -c "https://raw.githubusercontent.com/TheAssassin/linuxdeploy-plugin-conda/master/linuxdeploy-plugin-conda.sh"
wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
# we need python 3.7 or lower for this plugin to work due to a later deprecation
sed '/miniconda_url=.*/i \    miniconda_installer_filename=Miniconda3-py37_4.10.3-Linux-x86_64.sh' linuxdeploy-plugin-conda.sh
chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-conda.sh

mkdir -p AppDir/usr/bin
chmod +x client.sh
cp client.sh AppDir/usr/bin/

cat > kvm-client.desktop <<\EOF
[Desktop Entry]
Version=1.0
Name=HPRDP-KVM
Icon=keyboard
Exec=client.sh
Categories=Network;Utility;System;
Terminal=true
Type=Application
StartupNotify=true
MimeType=application/x-extension-fcstd;
EOF

export CONDA_PACKAGES="pip"
export PIP_REQUIREMENTS="-e ../client/"
export CONDA_PYTHON_VERSION="3.7"
./linuxdeploy-x86_64.AppImage --appdir AppDir -d kvm-client.desktop -i keyboard.png --plugin conda --output appimage

rm kvm-client.desktop

QHY_SDK=qhy_sdk_linux64_22.03.11

unzip ${QHY_SDK}.zip
cd ${QHY_SDK}
sudo bash install.sh
cd ..
rm -rf ${QHY_SDK}

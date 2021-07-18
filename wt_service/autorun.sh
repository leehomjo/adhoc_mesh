# !/bin/sh
ifconfig eth0 192.168.104.21 up
mount /dev/mmcblk1p2 /mnt
cd /mnt/wt_service/00BoaIndexInstall/Install
cp -f boa /bin
cd etc
cp -r boa /etc
cp -f mime.types /etc
cd ..
cd var
cp -r log_boa /var
cp -r www /var
cd /bin
./boa

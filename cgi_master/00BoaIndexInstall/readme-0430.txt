
u_disk: 
mount /dev/sda1 /mnt/

cd /mnt/00BoaIndexInstall/Install
cp -f boa /bin  (/bin/boa)

cd etc
cp -r boa /etc     (/etc/boa/boa.conf)
cp -f mime.types /etc 

cd.. 
cd var  
cp -r log_boa /var 
cp -r www /var 

cd /bin
./boa
 
PC 网页访问192.168.1.100
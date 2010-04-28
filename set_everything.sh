#Get the number of the arm
NUMBER=`echo $HOSTNAME | tr -d arm `

#cp the module to the right place
#cp rt73_ts7500.ko /lib/modules/2.6.24.4/kernel/drivers/net/wireless/rt2x00/
#depmod -a

#copy the modprobe blacklist blacklist
#cp blacklist /etc/modprobe.d/blacklist

#cp the network interfaces config file
sed s/#CHANGE_NUMBER#/$NUMBER/g interfaces_global > interfaces
cp interfaces /etc/network/interfaces

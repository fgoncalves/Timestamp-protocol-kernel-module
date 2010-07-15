sudo service network-manager stop

#This is for the wifi
ifconfig eth1 down
ifconfig eth1 up
iwconfig eth1 essid teste mode Ad-Hoc channel 1 ap 02:0C:F1:B5:CC:5D key off
ifconfig eth1 192.168.0.102

#This is for the wired
#ifconfig eth0 192.168.1.102 netmask 255.255.255.0

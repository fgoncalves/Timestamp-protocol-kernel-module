sudo service network-manager stop

ifconfig eth1 down
ifconfig eth1 up
iwconfig eth1 essid teste mode Ad-Hoc channel 1 ap 02:0C:F1:B5:CC:5D key off
ifconfig eth1 192.168.0.102

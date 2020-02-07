#!/bin/bash

tux1="172.16.$20.1"
tux2="172.16.$21.1"
tux41="172.16.$20.254"
tux42="172.16.$21.253"
rc="172.16.$21.254"

vlan1="172.16.$20.0"
vlan2="172.16.$21.0"

tux1() {
	ip route flush table main &&
	ifconfig eth0 down &&
	ifconfig eth0 up &&
	ifconfig eth0 "$tux1/24" &&

	route add -net "$vlan2/24" gw $tux41 &&
	route add default gw $tux41 &&

	echo 0 > /proc/sys/net/ipv4/ip_forward &&
	echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts &&

	echo ""
	echo "----------------------------"
	echo "|IP:          $tux1  |"
	echo "|VLAN1:        $vlan1  |"
	echo "|VLAN GW:     $tux41|"
	echo "|Default GW:  $tux41|"
	echo "----------------------------"
	echo ""
	ifconfig &&
	route -n
}
tux2() {
	ip route flush table main &&
	ifconfig eth0 down &&
	ifconfig eth0 up &&
	ifconfig eth0 "$tux2/24" &&

	route add -net "$vlan1/24" gw $tux42 &&
	route add default gw $rc &&

	echo 0 > /proc/sys/net/ipv4/ip_forward &&
	echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts &&

	echo ""
	echo "----------------------------"
	echo "|IP:          $tux2  |"
	echo "|VLAN2:        $vlan2  |"
	echo "|VLAN GW:     $tux42|"
	echo "|Default GW:  $rc|"
	echo "----------------------------"
	echo ""
	ifconfig &&
	route -n
}
tux4(){
	ip route flush table main &&
	ifconfig eth0 down &&
	ifconfig eth0 up &&
	ifconfig eth0 "$tux41/24" &&
	ifconfig eth1 down &&
	ifconfig eth1 up &&
	ifconfig eth1 "$tux42/24" &&

	route add default gw $rc &&

	echo 1 > /proc/sys/net/ipv4/ip_forward &&
	echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts &&

	echo ""
	echo "----------------------------"
	echo "|IP1:          $tux41  |"
	echo "|IP2:          $tux42  |"
	echo "|VLAN1:        $vlan1  |"
	echo "|VLAN2:        $vlan2  |"
	echo "|Default GW:  $rc|"
	echo "----------------------------"
	echo ""
	ifconfig &&
	route -n
}

if [ "$#" -ne "2" ]
then echo "USAGE: test.sh <tuxN> <BancadaN>"
elif [ $1 -eq 1 ]
then tux1
elif [ $1 -eq 2 ]
then tux2
elif [ $1 -eq 4 ]
then tux4
else 
	echo "Invalid Tux number"
fi

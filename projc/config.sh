#!/bin/bash

sudo ip tuntap add dev tun1 mode tun
sudo ifconfig tun1 10.5.51.2/24 up

IP_OF_ETH0=`ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'`

sudo ip rule add from $IP_OF_ETH0 table 9 priority 8
sudo ip route add table 9 to 18/8 dev tun1
sudo ip route add table 9 to 128.30/16 dev tun1

sudo ifconfig eth1 192.168.201.2/24 up
sudo ifconfig eth2 192.168.202.2/24 up
sudo ifconfig eth3 192.168.203.2/24 up
sudo ifconfig eth4 192.168.204.2/24 up
sudo ifconfig eth5 192.168.205.2/24 up
sudo ifconfig eth6 192.168.206.2/24 up
sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP
sudo ip route add table 9 to 128.9.160.91 dev tun1

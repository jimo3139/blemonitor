1, Start the blescan program on the rpi first. It should print rssi info once a second.
2, exec ute the ble/ble.html file on the PC.
3, Set the IP address to the rpi's IP
4, Set the port number to the port used when starting the blescan program. Below uses 8000.
5, Refresh the web page.

If the blescan program doesn't restart on the rpi, execute the commands below to restart it.

sudo killall blescan
sudo service bluetooth restart
sudo service dbus restart
sudo hciconfig hci0 down
sudo hciconfig hci0 up
sudo systemctl daemon-reload
sudo service bluetooth restart
sudo hciconfig hci0 up
sudo ./blescan 8000 /home/blescan

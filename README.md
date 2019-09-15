# open_service

raven_helper is a program that will be running on a vps to deliver all data.
service_open is a program that will be running on the machine which contains service you want to make it open to the world.

For example:
You want machine 192.168.1.1 to open 3389 port service to the world through vps 123.130.25.30
then you should run the command like below on 123.130.25.30
#./raven_helper 40001 40002
and run the command like below on 192.168.1.1
#./service_open 123.130.25.30 40001 3389

And now you may be able to access 192.168.1.1 from any places in the wolrd like this below:
1. open windows cmd prompt.
2. type command : mstsc -v 123.130.25.30:40002
3. input the username and password of 192.168.1.1 to access it.

sudo rm /dev/message_slot
sudo rmmod message_slot.ko
make
sudo insmod message_slot.ko
sudo mknod /dev/message_slot c 247 0
sudo chmod 777 /dev/message_slot
gcc -o sender ./message_sender.c
gcc -o reader ./message_reader.c
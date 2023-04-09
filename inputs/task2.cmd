// Task 2: Nested directories  //
/////////////////////////////////

// user    0  --> root
// user 1000  --> alice
// user 1001  --> bob

user  0
mkdir /etc,        0x0, 0x0
open  /etc/passwd,   2, 0x8
open  /etc/group,    2, 0x8
close 0
close 1

mkdir /etc/default,   0x0, 0x0
link  /etc/dbus.conf, /etc/default/dbus, 0x0
open  /etc/dbus.conf,   3, 0x8
write 0, data1, 10
close 0

// open file directly
open  /etc/dbus.conf,    3, 0x0
read  0, dbus, 10
close 0

// open file via symbolic link
open  /etc/default/dbus, 3, 0x0
read  0, dbus, 10
close 0

///////
// Multi-level nesting
///////
mkdir /home,              0x0, 0x0
mkdir /home/alice,        0x0, 0x0
mkdir /home/alice/.ssh,   0x0, 0x0
mkdir /home/bob,          0x0, 0x0
mkdir /home/bob/.ssh,     0x0, 0x0

user  1000
open  /home/alice/.ssh/config,   5, 0x8
write 0, data1, 10
close 0
open  /home/alice/.ssh/config,    5, 0x0
read  0, buf, 15
close 0


user  1001
open  /home/bob/.ssh/config,     5, 0x8
write 0, data2, 10
close 0
open  /home/bob/.ssh/config,      5, 0x0
read  0, buf, 15
close 0

list

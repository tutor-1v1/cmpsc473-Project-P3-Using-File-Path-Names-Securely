// Task 3a: Symbolic links to files  //
///////////////////////////////////////

user  0
mkdir /bin,          0x0, 0x0
open  /bin/cp.1,       1, 0x8
write 0, data1, 10
close 0
link  /bin/cp.1, /bin/cp, 0x0

open  /bin/cp,         1, 0x0
read  0, buf, 10
close 0


// Extended testing

// user    0 --> root
// user  200 --> httpd
// user 1000 --> adversary

user  200
mkdir /var,               0x1, 0x0
mkdir /var/www,           0x1, 0x0
open  /var/www/index.html,  1, 0x8
open  /var/www/.htpasswd,   1, 0x8
write 1, data2, 10
close 0
close 1

user 1000
mkdir /tmp,               0x1, 0x0
link /var/www/.htpasswd, /tmp/steal, 0x0
list

// success
user 200
open /tmp/steal,            1, 0x0
read 0, buf, 10
close 0

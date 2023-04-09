// Task 4c: FLAG_SAVEDNAME //
/////////////////////////////

// user    0  --> root
// user  200  --> httpd
// user 1000  --> adversary

// success
user  200
open  /tmp/STEAL,          1, 0x0
read  0, buf, 5
close 0
open  /var/WWW/.HtPasswd,  1, 0x0
open  /var/Www/index.HTML, 1, 0x0
read  0, buf, 5
read  1, buf, 5
close 0
close 1

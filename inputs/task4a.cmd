// Task 4a: FLAG_NOFOLLOW  //
/////////////////////////////

// user    0  --> root
// user  200  --> httpd
// user 1000  --> adversary

// success: follow directory symlinks
user  0
open  /sbin/cp.1,          1, 0x1
read  0, buf, 20
close 0

user  200
open  /tmp/html/.htpasswd, 1, 0x1
read  0, buf, 20
close 0

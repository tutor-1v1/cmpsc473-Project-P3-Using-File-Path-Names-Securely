// Task 3b: Symbolic links to directories  //
/////////////////////////////////////////////

user  0
link  /bin,    /sbin, 0x0

open  /sbin/cp.1,  1, 0x0
read  0, buf, 10
close 0


// Extended testing
user 1000
link /var/www, /tmp/html, 0x0
list

// success
user  200
open  /tmp/html/.htpasswd, 1, 0x0
read  0, buf, 10
close 0

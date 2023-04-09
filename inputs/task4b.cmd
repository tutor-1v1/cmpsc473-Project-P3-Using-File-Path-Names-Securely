// Task 4b: FLAG_NOFOLLOW_UNTRUSTED  //
///////////////////////////////////////

// user    0  --> root
// user  200  --> httpd
// user 1000  --> adversary

// success: nofollow_untrusted
user  0
open  /bin/cp,    1, 0x20
read  0, buf, 5
close 0

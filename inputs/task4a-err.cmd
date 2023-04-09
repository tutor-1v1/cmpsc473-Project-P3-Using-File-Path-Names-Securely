// Task 4a (err): Error conditions //
/////////////////////////////////////

// user    0  --> root
// user  200  --> httpd
// user 1000  --> adversary

// fail: nofollow
user 0
open /bin/cp,    1, 0x1
open /tmp/steal, 1, 0x1

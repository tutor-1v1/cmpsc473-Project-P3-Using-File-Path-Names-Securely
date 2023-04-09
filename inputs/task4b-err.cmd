// Task 4b (err): Error conditions //
/////////////////////////////////////

// user    0  --> root
// user  200  --> httpd
// user 1000  --> adversary

// fail: nofollow_untrusted
user 200
open /tmp/steal,          1, 0x20
open /tmp/html/.htpasswd, 1, 0x20

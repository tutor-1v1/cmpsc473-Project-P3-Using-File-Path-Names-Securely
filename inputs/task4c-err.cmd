// Task 4c (err): Error conditions //
/////////////////////////////////////

// user    0  --> root
// user  200  --> httpd
// user 1000  --> adversary

// fail open: savedname
user 200
open /var/WwW/.htpasswd,   1, 0x4
open /TMP/Steal,           1, 0x4
open /Tmp/Html/index.html, 1, 0x4

// fail create
open /vaR/WWW/file.txt,    1, 0xC

// fail mkdir
mkdir /var/WWW/images,   0x1, 0x4

//fail: nofollow + savedname
open /TMP/STEAL,           1, 0x5

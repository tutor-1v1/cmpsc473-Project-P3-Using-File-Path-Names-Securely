// Task 3c: Symbolic links to files/directories  //
///////////////////////////////////////////////////

user  0
open  /sbin/cp,   1, 0x0
read  0, buf, 10
close 0

open  /bin/cp,    1, 0x0
read  0,    buf, 10
close 0

open  /sbin/cp.1, 1, 0x0
read  0, buf, 10
close 0

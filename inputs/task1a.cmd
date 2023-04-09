// Task 1a: Prepare fcb_t for symlink  //
/////////////////////////////////////////

// Create a regular file
open  /foo,   0, 0x8
write 0,  data1, 0x0
close 0

// Create a directory
mkdir /opt, 0x1, 0x0

// List items
list

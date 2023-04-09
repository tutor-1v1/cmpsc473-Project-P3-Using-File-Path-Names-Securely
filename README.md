# cmpsc473 Project P3 Using File Path Names Securely

**_This is an individual project.  Do you own work!  Due: April 28, 2023 at 11:59pm_**

**_See Chapters 39 and 40 in "Operating Systems: Three Easy Pieces" for background.  We’ll discuss this in class next week.  Read this intro section by Tu._**

Perhaps surprisingly, opening a file creates several security risks.  The “open” system call takes as input a file pathname string, such as “/var/mail/root”, and returns a file descriptor to the file system object managed by the operating system that represents the file stored on the persistent storage (e.g., disk or flash device).  See the commands supported by our filesystem under the “Command Interface” section below.

When we open a file, the pathname is resolved by element-by-element - (1) the “/” (root); (2) then “/var”; (3) then “/var/mail/”; and (4) finally “/var/mail/root”.  This process is called **pathname resolution**, where each element of the pathname is converted into an operating system resource.  Normally, steps (1-3) result in the transformation of the elements into file system objects (inodes) that represent directories, one for “/”, one for “/var”, and one for “/var/mail”.  Then, step (4) resolves the last element “root” in the directory “/var/mail” into an inode that represents a file.

While this seems like a straightforward process, the complexity of file systems introduces risks that may enable an adversary to tamper with the process, preventing you from opening the file you intended.  We will examine three issues:



1. **Symbolic links**: Symbolic links are a special kind of file system object.  Instead of storing file data, symbolic links store a path to a file.  Again, if the directory “/var/mail” is writable by others, then anyone could create “/var/mail/root” as a symbolic link to any file you want, even one you do not have access to.  Suppose you wanted to open the privileged file “/etc/shadow” that stores password hashes.  You could create a symbolic link “/var/mail/root” whose value is the pathname “/etc/shadow”.  Should the root user try to open “/var/mail/root”, the file system will detect that that file system object is an inode for a symbolic link and then perform further pathname resolution on its value to open “/etc/shadow” (root has access to that file).
2. **Name Collisions**: Another issue is that individual file systems may resolve elements of a pathname differently, creating the potential for unexpected name collisions.   Historically, different operating systems treat file names differently: e.g., Linux is case-sensitive and Windows is case-preserving.   Thus, a Linux directory would create two files for “ROOT” and “root”, but only one file would be accessed for both of these names in Windows and Mac OS X.  Recently, one Linux file system (ext4) has adopted the approach that each directory in a file system may be either case-sensitive or case-preserving.  For example, if the directory “/var/mail” is case-preserving, and the file “ROOT” is added first, then the name “root” will refer to the file “ROOT”, i.e., the same file system object as “ROOT”.  

Your task in this project will be to extend the provided file system implementation to create file system objects for symbolic links and subdirectories (Tasks 1 and 2), enable pathname resolution using symbolic links (Task 3), and to enforce security constraints to prevent misuse of pathname resolution (Task 4) to prevent misuse of symbolic links and name collisions.  

For Task 1, you will implement the function **fileLink**, which will create a file system object to store the specialized information for a symbolic link.  A symbolic link file system object must store a string that this the pathname to assess when resolving the symbolic link.  Thus, we need to store the symbolic link’s pathname as file data.  Also, we need to tell the file system whether a file is a symbolic link, instead of a directory or regular file.  In addition, we will store the user id for the creator for enforce security constraints later.

For Task 2, you will implement the function **fsParentDir** to implement a typical hierarchical directory structure for a file system.  A directory file system object has as its data a set of directory entries (dentry) for each file system object in that directory.   Your efforts will enable us to store directory entries that are directories themselves.  

For Task 3, you will implement the function **fsResolveName** to enable the pathname resolution mechanism to use symbolic links whose targets may either be files or directories.  The pathname resolution will need to determine whether the pathname element (e.g., “root”) currently being evaluated from a full pathname (e.g., “/var/mail/root”) corresponds to a directory entry that is a symbolic link, a file, or a directory.  In the case that the directory entry is a symbolic link, the pathname resolution must apply the symbolic link’s value (e.g., “/etc/shadow”) to continue the resolution.  Note that the symbolic link may appear in the middle of the pathname overall pathname (e.g., “mail” could be a symbolic link in “/var/mail/root”).  

For Task 4, you will extend **fsResolveName** and **fsFindDentry** to enforce constraints in pathname resolution to prevent attacks by: (1) preventing a few uses of symbolic links that can enable attacks (e.g, prevent use of symbolic links created by one user to access files of another user) and (2) preventing use of files accessed via name collisions (e.g., the created name must equal the name used in resolution).  These are specific constraint types, some of which are already supported by the Linux file system APIs.

Below, we detail the requirements of these 4 tasks and then we present the structure of the file system.  The file system is designed analogously to many UNIX file systems, with abstract file system objects for user processes to operate on (e.g., files, directories, and symlinks), operating system objects to store information internally (e.g., inodes and directory entries), and storage-level data structures for storing the file data persistently (e.g., blocks).   

Note that like a file system, the results of each command are stored persistently.  We achieve this by storing your file system memory layout in a file itself.  Thus, as each command is run, the changes to the file system (in memory) are stored in a file on your computer persistently.  So, as you are debugging you may need to remove (delete) the file system that you have created and start again.  Fortunately, the test scripts are not very long.  We provide a set of test scripts whose correct operation will account for a significant portion of the project score (around 80%).  We may also test with other stress test scripts in addition. 


## Task Descriptions

For all the tasks, you will only need to modify the files cmpsc473-filesys.c and cmpsc473-disk.c. However, you should **not modify** the Makefile, cmpsc473-filesys.h, and cmpsc473-p3.c.


### Task #1: Support creating symbolic links

In this task, you will add support to create a special file type called a symbolic link (or a symlink). You will begin by adding code to record the file type and assign the file owner when any new file is being created - to record necessary info to distinguish symlinks from regular files and to control their use later.  For this, you will need to set the appropriate fields in the file’s control block (or fcb_t) structure. Then, you will implement the fileLink(...) function which creates a special file of the type symbolic link (denoted by FTYPE_SYMLINK). You will store the target of the link inside the data block(s) pointed to by the symlink’s fcb_t structure.

**1a: Prepare fcb_t for symlink**

The "owner" field in file_t structure identifies the user who created the file . Its corresponding on-disk counterpart is the "uid" field in the fcb_t structure. You need to set the appropriate "uid" in the function diskCreateFile(...) using the input arguments.

 

The "type" field in the fcb_t structure (and file_t structure) identifies the type of the file. It can have the following values defined in cmpsc473-filesys.h:



* FTYPE_REGULAR representing a regular file
* FTYPE_SYMLINK representing symbolic links

 

You need to set the appropriate file type in the function diskCreateFile(...) using the input arguments.

 

**1b: Create symbolic links**

You will implement fileLink(...) in cmpsc473-filesys.c. To create a new file, refer to the fileCreate(...). You should set the type field of file_t (and/or fcb_t) to FTYPE_SYMLINK. Next, open this newly created file, write the contents of the "target" variable into it and close the file. You may use the existing functions fileWrite(...) and fileClose(...) for this purpose.


### Task #2: Support nested directories

The code base given to you only supports creating a single directory inside '/'. In this task, you will be implementing fsParentDir(...) to support resolution of nested directories i.e., directory within a directory.

 

Beginning at the directory '/', you will need to locate the appropriate directory entry (dentry) using the pathname argument provided.  For each element in a pathname (e.g., “mail” in “/var/mail/root”) you find its corresponding dentry object using the path structure (path_t) derived from the pathname. From the dentry, you will retrieve the dir_t structure representing the directory. You need to repeat this for every directory in the path.

 

You should set the "stopat" field inside the path_t structure to the index of the last successful dentry resolution (i.e., depth of the path). You will use this for task 3b.


### Task #3: Support following symbolic links

In this task, you will extend fsResolveName(...) to support resolving symbolic links. In case of an error, you should set the err field of path_t structure to E_Resolve.

 

**3a: You will add support for resolving symbolic links to files.**

The provided code in fsResolveName(...), resolves the directories in the path using fsParentDir(...) and then retrieves the dentry for the next name in the path. In the absence of symbolic links, this code will correctly find existing files by their path names.

However, the underlying file pointed to by the dentry might be a symbolic link. You can check for this using the type field in file_t. Since symbolic links point to other files, you will need to once again resolve the path, but this time using the link's value as the target path. Recall that the target path for a symlink is stored as the file's data. You will read the contents of the file (i.e., the symbolic link value) to identify the actual target file. You will now resolve the path of this target file and return its corresponding path_t structure.

You can optionally choose to implement the fsGetLinkTarget(...) helper function which will retrieve the symbolic link's target by reading the file's contents.

You can assume that symbolic links to files do not point to other symbolic links.

 

**3b: You will add support for resolving symbolic links to directories.**

When resolving the path, fsParentDir(...)  may also encounter a symbolic link (but to another directory). If this happens, you will have unresolved elements in the original path (e.g., if “/var/mail/root” is the path and “mail” is a symbolic link).  After resolving the symbolic link, the remaining unresolved elements (e.g., “root” in the path above)  in original pathname should now be looked up in the symbolic link's target directory.

Using the strategy from 3a, you will again retrieve the link's target, but this time the target becomes the parent directory for the unresolved path element instead of being an absolute path to another file.

You can optionally choose to implement the pathMergeWithSymlink(...) function to construct a new path from a partially resolved path and a symbolic link's target.

Example: Consider the following layout:


```bash
/bin        # directory
/bin/cp.1   # regular file
/bin/cp     # symbolic link to /bin/cp.1
/sbin*      # symbolic link to /bin
```


When you open /sbin/cp.1, you will be effectively opening the file /bin/cp.1 because /sbin points to /bin.  You may assume that a symbolic link to a directory *will not* point to another symbolic link to a directory.

 

**3c: Support mixed use of symbolic links**

Ensure that you can resolve paths with both a directory symbolic link and a file symbolic link. For the example in 3b, opening /sbin/cp should effectively open /bin/cp.1.

 

Note: If you implemented tasks 3a and 3b correctly, then this task should work automatically.


### Task #4: Support security constraints

You will extend fsResolveName(...) to support the “constrains” values of FLAG_NOFOLLOW and FLAG_NOFOLLOW_UNTRUSTED. You will also extend fsFindDentry(...) to support the FLAG_SAVEDNAME constraint.

The idea is that user programs that use your filesystem can include these flags to prevent unsafe behaviors during pathname resolution.  For example, FLAG_NOFOLLOW must prevent a fileCreate/Open from using any symbolic links.  The flag FLAG_NOFOLLOW_UNTRUSTED must restrict fileCreate/Open to only use a symbolic link if the owner of the symbolic link file is also the owner of the link’s target.  The flag FLAG_SAVEDNAME must restrict fileCreate/Open to only access a file whose “saved” name used at creation is the same as the name provided in the system call (e.g., same case).

 

**4a: Support FLAG_NOFOLLOW**

When FLAG_NOFOLLOW is set in a “constrain” value (e.g., when passed as an argument to opening a file), you will prevent the traversal of symbolic links to files. You will also need to set the err field inside the path_t structure to E_Link.

 

Note that you will still need to allow traversal of symbolic links to directories.

 

**4b: Support FLAG_NOFOLLOW_UNTRUSTED**

When FLAG_NOFOLLOW_UNTRUSTED is set in a “constrain” value, you will only follow symbolic links (to files as well as directories) that are accessible to the current user. In other words, the owner of the symbolic link should match the currently active user. The error to report in this case is E_UntrustedLink.

**4c: Support FLAG_SAVEDNAME**

When FLAG_SAVEDNAME is set in a “constrain” value, you will prevent accessing files/directories using differing case names. The case of the name being used needs to match the case of the name stored on the disk, even if the directory is case-preserving. You will implement this enforcement check in the diskCheckDentryConstraint(...) function and call it from the fsFindDentry(...) function.

Note that this flag will have no effect on case-sensitive directories.


## The File System

The provided RAM disk file system stores its "disk" configuration in memory when you run the project. That is, the layout of the disk is exactly the layout in memory. The file system supports case-insensitive directories and is also **case-preserving**. This means that when new files, or directories are created inside case-insensitive directories, the file system remembers the exact case used to create it. You can continue to resolve all names within the case-insensitive directories using any variation of the case unless using additional security constraints.

The file system is defined by its structure shown below.


![file system layout](imgs/disklayout.png "file system layout")


The file system consists of a series of **blocks**, which although in memory in your project correspond to the layout of the file system on disk. The main structures used to implement the different types of file system metadata, including blocks are defined in the file **cmpsc473-filesys.h** in the project code. **Do not modify this file.**

Each block is prefaced by its block metadata specified in a **dblock_t** structure. The block metadata determines the type of block (**free**, which also indicates whether the type is to be determined), the next block in a list of free blocks (**next**), and metadata about the block in a union data structure (**st**), which may either reference a bitmap (for a dentry block) or the end of the block data (e.g., for data blocks).

The last field (**data[0]**) is a reference to the block's data. Note that although this field is declared as an array of size 0, it is actually just a reference to a field whose size is determined at runtime. In the case of blocks, the data in the block is determined by the block type (see BLOCK definitions in cmpsc473-filesys.h). While this may be uncommon in user-space programs, it is common in the Linux kernel - to avoid wasting memory for objects whose size is only known at runtime.

Below, we detail the file system structure shown in the figure above.



* Block 0 is the **file system block** or **superblock**, which defines the overall structure of the file system using a structure of type **dfilesys_t**. This block states the number of blocks in the file system (**bsize** field), the offset in blocks to the next free block (**firstfree** field), and the offset to the root directory block of the file system (**root** field). \

* Block 1 stores the **root directory block** of type **ddir_t**, which is the only directory in our filesystem. The flags field determines if the directory is case-sensitive, or case-preserving. Each on-disk directory stores a set of references to directory entries (dentries) using a hash table. The on-disk directory stores the number of buckets in its hashtable (**buckets**), the free dentry block (see below) for storing the next directory entry (**freeblk**), and the first free dentry slot index in that dentry block (**free**). The last field (**data[0]** indicates the start of the hash table (the first bucket) for the directory block. \

* Block 2 is a **directory entry (dentry) block**, which stores a series of directory entries (dentries) of type **ddentry_t**. The dentries store information about each entry in a directory (i.e., file or subdirectory). In this project, we only have files, so there are only dentries for files. Each dentry records its file's **file control block** (**block**), which is the first block for a file, the next dentry in the directory's hash table (**next**), and the file name (**name[0]**) and name length (**name_size**). \
 \
  Thus, the directory entry hash table starts in the directory block - by finding the bucket a file name corresponds to - and then traverses directory entries in dentry blocks using the next pointer to find the **next_dentry** block and dentry **next_slot** using the **ddh_t** structures. That is probably the most complicated thing about this file system. \

* Block 3 is a **file control block**, which stores the metadata for a particular file using type **fcb_t**. The file control block stores the file permissions available to your one process to this file (**flags**), the owner of this file (**uid**), the size of the file in bytes (**size**), the type of the file: regular file, or symbolic link (**type**), the file data blocks (**blocks**, up to 10 in this project), and the first file attribute block (**attr_block**, see Extended Attributes below). There will be several file control blocks - one for each file created. \

* Block 4 and 5 are **data blocks**, which are used to store the file data. The **dblock_t** structure at the start of every block is used to manage the file data, which is written starting in the **data[0]** field (and the **data[1]** field), and whose current length is recorded by the **data_end** field (in the union).


### Running


```bash
# Compile
make

# Run
./cmpsc473-p3 disk.img cmdfile # or
./cmpsc473-p3 disk.img cmdfile1 [ cmdfile2 ... ]
```


where, cmdfile are the command files inside "inputs/". disk.img can be used verbatim. It represents the underlying file system where your changes will be written. By default, output.log is color coded. If you have difficulty viewing colors, then you can remove it using:


```bash
sed -e 's/\x1b\[[0-9;]*m//g' outputs/task1a.log
```


To view the output files,


```bash
less -R outputs/task1a.log  # or
cat outputs/task1a.log
```


Each command file inside "inputs/" directory persistently updates the disk.img file. Hence, the correct output for each task is contingent on running the previous task's command file successfully. The correct order tasks for the tasks is:


```bash
rm -f disk.img
./cmpsc473-p3 disk.img inputs/task1a.cmd
./cmpsc473-p3 disk.img inputs/task1b.cmd
./cmpsc473-p3 disk.img inputs/task1b-err.cmd
./cmpsc473-p3 disk.img inputs/task2.cmd
./cmpsc473-p3 disk.img inputs/task3a.cmd
./cmpsc473-p3 disk.img inputs/task3a-err.cmd
./cmpsc473-p3 disk.img inputs/task3b.cmd
./cmpsc473-p3 disk.img inputs/task3b-err.cmd
./cmpsc473-p3 disk.img inputs/task3c.cmd
./cmpsc473-p3 disk.img inputs/task3c-err.cmd
./cmpsc473-p3 disk.img inputs/task4a.cmd
./cmpsc473-p3 disk.img inputs/task4a-err.cmd
./cmpsc473-p3 disk.img inputs/task4b.cmd
./cmpsc473-p3 disk.img inputs/task4b-err.cmd
./cmpsc473-p3 disk.img inputs/task4c.cmd
./cmpsc473-p3 disk.img inputs/task4c-err.cmd
```



### Command File (.cmd) Format

**Available commands in our file system:**


```bash
// This is a comment
user       uid
mkdir      name,   flags,    constrain
open       name,   flags,    constrain   # returns fd
link       target, name,     constrain   # "name" adds path "target"
write      fd,     filename, unused_int
read       fd,     buffer,   size
print      fd                            # prints entire file
close      fd
list                                     # outputs file system
```


**Description of the parameters:**

Any line that begins with "**//**" is treated as a comment and ignored. All blank lines are ignored. Note that comments at the end of the command line are not supported, e.g., "**list** // ignore comment" is invalid.

"**name**" is the absolute path to a file (i.e., starts with ‘/’), directory, or symbolic link. Applies to mkdir, open and link.

For open, "**flags**" represents the file permissions, or mode (i.e., rwx) to create or open the file.

For mkdir, "**flags**" sets attributes on the directory. When set to D_ICASE, the directory becomes **case-insensitive**.

"**constrain**" controls the behavior of the name resolution. It behaves similarly to the standard Linux variants that start with "O_...". Applies to mkdir, open and link.



* **FLAG_CREAT**: Create the file if it does not exist. Only applies to open.
* **FLAG_EXCL**: When used with FLAG_CREAT, open will fail if the file already exists. Only applies to open.
* **FLAG_NOFOLLOW**: Do not follow symbolic links to files. Note that symbolic links to directories within the path component are still followed. Only applies to open and mkdir.
* **FLAG_NOFOLLOW_ANY**: Do not follow any symbolic links.
* **FLAG_SAVEDNAME**: Succeeds only if the case of all the components in the "name" matches the case that was used when creating the respective component.
* **FLAG_NOFOLLOW_UNTRUSTED**: Do not follow links created by other users. Broadly, do not follow links if the link's owner is not the owner of the target resource (file or directory) obtained by resolving the link’s target.

The above flags are defined in "cmpsc473-filesys.h".


## Grading

Your project will be graded based on which the input command files produce the correct results. All command files can be found in the "inputs/" directory. The expected console output can be found in the "outputs/" directory.

You are provided with a grading script called "grade.sh" which will output your current grade out of 9 points. To use the script:


```bash
./grade     # your grade, or
./grade -v  # verbose mode -- show diff from expected output
```


When we grade your project, we will start with the base student template provided to you. Next, we will copy the files cmpsc473-file.c, cmpsc473-disk.c, cmpsc473-disk.h, cmpsc473-util.c, and cmpsc473-util.h from your submitted version into the base student template and run the "grade.sh" script. So, be sure to **not modify any other files**, or **create new files** in the repository because these won't be graded.

For grading, we will use the **latest commit** of the default branch in your repository.

The rubric for each command file is as follow:


<table>
  <tr>
   <td><strong>Tasks</strong>
   </td>
   <td><strong>Command File</strong>
   </td>
   <td><strong>Grade</strong>
   </td>
  </tr>
  <tr>
   <td>Task 1 (3pts.)
   </td>
   <td>inputs/task1a.cmd
   </td>
   <td>0.5
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task1b.cmd
   </td>
   <td>2
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task1b-err.cmd
   </td>
   <td>0.5
   </td>
  </tr>
  <tr>
   <td>Task 2 (1pt.)
   </td>
   <td>inputs/task2.cmd
   </td>
   <td>1
   </td>
  </tr>
  <tr>
   <td>Task 3 (3pts.)
   </td>
   <td>inputs/task3a.cmd
   </td>
   <td>0.75
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task3a-err.cmd
   </td>
   <td>0.25
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task3b.cmd
   </td>
   <td>0.75
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task3b-err.cmd
   </td>
   <td>0.25
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task3c.cmd
   </td>
   <td>0.75
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task3c-err.cmd
   </td>
   <td>0.25
   </td>
  </tr>
  <tr>
   <td>Task 4 (2pts.)
   </td>
   <td>inputs/task4a.cmd
   </td>
   <td>0.2
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task4a-err.cmd
   </td>
   <td>0.4
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task4b.cmd
   </td>
   <td>0.2
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task4b-err.cmd
   </td>
   <td>0.5
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task4c.cmd
   </td>
   <td>0.2
   </td>
  </tr>
  <tr>
   <td>
   </td>
   <td>inputs/task4c-err.cmd
   </td>
   <td>0.5
   </td>
  </tr>
</table>


# cmpsc473 Project P3 Using File Path Names Securely
# WeChat: cstutorcs

# QQ: 749389476

# Email: tutorcs@163.com

# Computer Science Tutor

# Programming Help

# Assignment Project Exam Help

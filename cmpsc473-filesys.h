https://tutorcs.com
WeChat: cstutorcs
QQ: 749389476
Email: tutorcs@163.com
#ifndef FILESYS_H
#define FILESYS_H

/**********************************************************************

   File          : cmpsc473-filesys.h

   Description   : This file contains the file system prototypes

***********************************************************************/
/**********************************************************************
Copyright (c) 2023 The Pennsylvania State University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of The Pennsylvania State University nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

/* defines */
#define TRUE 1
#define FALSE 0

/* file system in general */
#define FS_BLOCKS 200
#define FS_METADATA_BLOCKS 1   // fs block
#define FS_DIRINIT_BLOCKS 2    // dir block and first dentry block
#define FS_BLOCKSIZE 512
#define MAX_NAME_SIZE 40
#define MAX_PATH_SIZE 255
#define MAX_PATH_DEPTH 10
#define MAX_LINK_DEPTH 10
#define MAX_LINE_WIDTH 60      // for printing to console
#define BLK_INVALID  0xFFFFFFFF
#define BLK_SHORT_INVALID  0xFFFF

/* disk defines */
#define DENTRY_MAP 0xFF  /* 8 entries per block */
#define DENTRY_MAX 8     /* 8 entries per block */

/* file types */
#define FTYPE_UNSPECIFIED  0   /* for fsFindFile() */
#define FTYPE_REGULAR      1   /* regular file */
#define FTYPE_SYMLINK      2   /* symbolic link */

/* dentry types */
#define DTYPE_UNSPECIFIED  0   /* for fsFindFile() */
#define DTYPE_FILE         1   /* file */
#define DTYPE_DIR          2   /* directory */

/* directory flags */
#define D_NONE           0x0   /* no flag (default) */
#define D_ICASE          0x1   /* case-insensitive directory */

/* flags for open and creat */
#define FLAG_NOFOLLOW                  0b1   /* do not follow symbolic links to files */
#define FLAG_EXCL                     0b10   /* do not follow symbolic links */
#define FLAG_SAVEDNAME               0b100   /* ensure given name matches the creation name */
#define FLAG_CREAT                  0b1000   /* create file if it does not exist */
#define FLAG_NOFOLLOW_ANY          0b10000   /* do not follow any symbolic links in entire path */
#define FLAG_NOFOLLOW_UNTRUSTED   0b100000   /* do not follow untrusted symbolic links to files */

/* block free values */
#define FREE_BLOCK 0
#define DENTRY_BLOCK 1
#define FILE_BLOCK 2
#define FILE_DATA 3
#define FS_BLOCK 4
#define DIR_BLOCK 5
#define ATTR_BLOCK 6
#define JOURNAL_BLOCK 7

/* file system defines */
#define FS_FILETABLE_SIZE 60
#define PROC_FILETABLE_SIZE 20
#define FS_BCACHE_BLOCKS 50
#define FILE_BLOCKS 10

/* macros */

/* translate between disk location and memory location */
#define disk2addr( mbase, doffset ) ( ((char *)mbase)+doffset )
#define addr2disk( mptr, mbase ) ( ((char *)mptr)-mbase )
#define block2addr( mbase, block_num ) ( ((char *) mbase ) + ( FS_BLOCKSIZE * block_num ))
#define addr2block( mptr, mbase ) ( (((char *) mptr ) - ((char *) mbase)) / FS_BLOCKSIZE )


static inline unsigned int fsMakeKey( char *name, int limit, unsigned int name_size ) 
{
	int len = name_size;
	int i, val = 1;

	for ( i = 0; i < len; i++ ) {
		val = ( (int)name[i] * val ) % limit;
	}
  
	return val;
}

/* describe a user */
typedef struct user {
	int uid;
} user_t;

/**********************************************************************

    Structure    : dblock_t 
    Purpose      : corresponds to actual block on the disk -- includes
                   whether the block is free (a next block index if it 
                   is free), a dentry map (for free dentries slots in 
                   a dentry block) and data end (e.g., file data, but 
                   means general block data)

***********************************************************************/

/* data structure for the blocks on disk */
typedef struct dblock {
	unsigned int free;         /* is the block free? also the type of data stored  */
	union status {
		unsigned int dentry_map; /* free dentries bitmap -- out of 10 per block */
		unsigned int data_end;   /* end pointer of file data in block */
	} st;
	unsigned int next;         /* next disk block index -- in free list */
	char data[0];              /* the rest of the block is data -- depends on block size */
} dblock_t;

typedef char block_t;        /* block is a character array of size FS_BLOCKSIZE */


/**********************************************************************

    Structure    : fcb_t 
    Purpose      : this is the file control block -- persistent storage
                   for file attributes and the file data blocks 
                   (actual number of data blocks max determined by FILE_BLOCKS)

***********************************************************************/

/* on-disk data structure for a file -- file control block */
typedef struct file_control_block {
	unsigned int flags;      /* file flags */
	unsigned int uid;        /* file owner's uid */
	unsigned short type;     /* file type */
	unsigned int size;       /* size of the file on disk */
	unsigned int attr_block; /* index to first attribute block */
	unsigned int blocks[0];  /* indices to data blocks for rest of file block */
} fcb_t;


/**********************************************************************

    Structure    : file_t 
    Purpose      : this corresponds to an inode -- the in-memory 
                   representation for the file (system-wide).  Includes
                   attributes, including name (names are stored
                   with dentries on the disk -- not fcb), reference count, 
                   reference to the fcb location (in ram-disk), and 
                   a set of file blocks

***********************************************************************/

/* in-memory data structure for a file (inode) */
typedef struct dir dir_t;
typedef struct file {
	unsigned int flags;                /* file flags */
	user_t owner;                      /* file owner */
	unsigned short type;               /* file type */
	unsigned int size;                 /* file size */
	unsigned int ct;                   /* reference count */
	dir_t *parent;                     /* inside which directory? */
	fcb_t *diskfile;                   /* fcb pointer in ramdisk */
	unsigned int attr_block; /* index to first attribute block */
	unsigned int blocks[FILE_BLOCKS];  /* direct blocks */
	unsigned int name_size;            /* file name length */
	char name[0];                      /* file name */
} file_t;


/**********************************************************************

    Structure    : fstat_t 
    Purpose      : this corresponds to the per-process file structure.
                   Mainly care about the current offset for the file
                   for determining where to read or write

***********************************************************************/

/* in-memory data structure for a file operation status */
typedef struct fstat {
	file_t *file;          /* pointer to system-wide in-memory file reference */
	unsigned int offset;   /* current offset index for reads/writes/seeks */
} fstat_t;


/**********************************************************************

    Structure    : ddh_t 
    Purpose      : represents a location for a dentry on the disk -- 
                   which block and which slot in the block -- used 
                   for dentry hash table entries (linked list)

***********************************************************************/

/* disk directory entry hash component */
typedef struct ddentry_hash {
	unsigned short next_dentry; /* next dentry block in hash table */
	unsigned short next_slot;   /* next dentry slot in hash table */
} ddh_t;


/**********************************************************************

    Structure    : ddentry_t  
    Purpose      : represents an on-disk directory entry -- includes
                   the file name and index for the first block in the file
                   (also reference to next dentry for hash table's list)

***********************************************************************/

/* disk directory entry on disk */
typedef struct ddentry {
	unsigned int block;         /* block number of first block of file */
	ddh_t next;                 /* next dentry in hash table */
	unsigned int type;          /* type: file or dirctory */
	unsigned int name_size;     /* file name length */
	char name[MAX_NAME_SIZE];   /* file name - allocate space since preallocated in dblock */
} ddentry_t;


/**********************************************************************

    Structure    : dentry_t  
    Purpose      : represents an in-memory directory entry -- includes
                   the file name, file pointer, on-disk dentry memory 
                   location, and next entry in the in-memory hash table

***********************************************************************/

/* in-memory directory entry stores a reference to a file in a directory */
typedef struct dir dir_t; // forward reference to use here
typedef struct dentry {
	union {
		file_t *file;          /* file reference */
		dir_t *dir;            /* directory reference */
	};
	ddentry_t *diskdentry;     /* reference to corresponding on-disk structure */
	struct dentry *next;       /* next dentry in the list */
	unsigned int type;         /* type: file or directory */
	unsigned int name_size;    /* file name length */
	char name[0];              /* file name */
} dentry_t;


/**********************************************************************

    Structure    : ddir_t  
    Purpose      : represents an on-disk directory -- each directory 
                   stores a hash table of on-disk directory entries 
                   (only the first is stored in this block -- the rest 
                    are obtained from ddh_t info), a reference to the 
                    first free spot for a new ddentry (freeblk for block
                    and free for free dentry slot)
                   data stores the hash table buckets

***********************************************************************/

/* disk data structure for the directory */
/* directory stores a hash table to find files */
typedef struct ddir {
	unsigned int buckets;        /* number of hash table buckets in the directory */
	unsigned int freeblk;        /* number of the next free block for dentry */
	unsigned int free;           /* first free dentry in block */
	unsigned int flags;          /* flags: e.g., case-insensitive */
	ddh_t data[0];               /* reference to the hash table */
} ddir_t;


/**********************************************************************

    Structure    : dir_t  
    Purpose      : represents an in-memory directory -- each directory 
                   stores a hash table of in-memory directory entries
                   (normal in-memory hash table) and reference to 
                   disk directory in ramdisk

***********************************************************************/

typedef struct dir {
	unsigned int bucket_size; /* number of buckets in the dentry hash table */
	unsigned int flags;       /* flags: e.g., case-insensitive */
	dentry_t **buckets;       /* hash table for directory entries */
	ddir_t *diskdir;          /* reference to the directory in ramdisk */
} dir_t;


/**********************************************************************

    Structure    : proc_t
    Purpose      : represents the in memory process -- just to store 
                   a reference the per-process file table

***********************************************************************/

/* in-memory per-process file table */
typedef struct process {
	fstat_t **fstat_table;     /* per-process open file table */
} proc_t;


/**********************************************************************

    Structure    : dfilesys
    Purpose      : represents the on-disk file system -- stores the 
                   overall file system information: number of blocks, 
                   the firstfree block on disk, and the block for the 
                   root directory

***********************************************************************/

/* data structure for a file system */
typedef struct dfilesys {
	unsigned int bsize;     /* number of blocks in the file system */
	unsigned int firstfree; /* offset in blocks to the first free block */ 
	unsigned int root;      /* offset to the root directory block */
} dfilesys_t;


/**********************************************************************

    Structure    : filesys
    Purpose      : represents the in-memory file system (superblock) 
                   and ad hoc info about bootstrapping the ramdisk --
                   fd refs to the file descriptor for the ramdisk file,
                   base is the base memory address of the ramdisk, 
                   sb is file stat for the ramdisk file,
                   dir is the in-memory root directory, 
                   filetable is the system-wide, in-memory file table, 
                   proc is our process (yes only one at a time, please)
                   function pointers for the file system commands

***********************************************************************/

/* in-memory info for the file system */
typedef struct filesys {
	int fd;                 /* mmap'd fs file descriptor */
	void *base;             /* mmap pointer -- start of mmapped region */
	struct stat sb;         /* stat buffer for mmapped file */
	dir_t *dir;             /* root directory in the file system (only directory) */
	file_t **filetable;     /* system-wide open file table */
	block_t **block_cache;  /* cache of blocks read into memory */
	proc_t *proc;           /* process making requests to the fs */
	int (*mkdir)( char *name, unsigned int flags, unsigned int constrain );  /* directory create */
	int (*create)( char *name, unsigned int flags, unsigned int constrain ); /* file create */
	int (*link)( char *target, char *name, unsigned int constrain );         /* create link */
	int (*open)( char *name, unsigned int flags, unsigned int constrain );   /* file open */
	void (*list)( void );                                 /* list directory */
	void (*close)( unsigned int fd );                     /* close file descriptor */
	int (*switchuser)( unsigned int new_uid );            /* switch current user */
	int (*read)( unsigned int fd, char *buf, unsigned int bytes );       /* file read */
	int (*write)( unsigned int fd, char *buf, unsigned int bytes );      /* file write */
	int (*seek)( unsigned int fd, unsigned int index );    /* file seek */
} filesys_t;


/* describe a path name */
typedef struct path {
	int num;                      /* number of components in the path */
	int stopat;                   /* stop parsing at index */
	int err;                      /* error number */
	dir_t *dir;                   /* dir at index stopat */
	dentry_t *dentry;             /* dentry at index stopat+1 */
	char *name[MAX_PATH_DEPTH];   /* name of each component */
} path_t;

/* populate info. about dentry for listing */
typedef struct listinfo {
	char *suffix;                 /* suffix for entry for visual interpretation */
	int ncolors;                  /* number of color chars in suffix */
	char describe[MAX_NAME_SIZE]; /* info on this dentry */
} listinfo_t;

/* Global variables */
extern filesys_t *fs;
extern dfilesys_t *dfs;
extern user_t user;


/**********************************************************************

    Function    : fsReadDir
    Description : Return directory entry for name
    Inputs      : name - name of the file 
                  name_size - length of file name
    Outputs     : new in-memory directory or NULL

***********************************************************************/

extern dir_t *fsReadDir( char *name, unsigned int name_size );

/**********************************************************************

    Function    : fsRootDir
    Description : Retreive '/' for the file system
    Inputs      : None
    Outputs     : pointer to the '/' dir

***********************************************************************/

extern dir_t *fsRootDir();

/**********************************************************************

    Function    : fsParentDir
    Description : Keep resolving directories until possible. Sets
                  p->stopat to the last successfully found dentry.
    Inputs      : path - to resolve
                  constrain - flags to control resolution
    Outputs     : tne parent dir

***********************************************************************/

extern dir_t *fsParentDir( struct path *p, unsigned int constrain );

/**********************************************************************

    Function    : fsFileInitialize
    Description : Create a memory file for the specified file
    Inputs      : parent - its parent directory
                  dentry - directory entry
                  name - name of the file 
                  flags - flags for file access
                  type  - type of file
                  name_size - length of file name
                  fcb - file control block (on-disk) reference for file (optional)
    Outputs     : new file or NULL

***********************************************************************/

extern file_t *fsFileInitialize( dir_t *parent, dentry_t *dentry, char *name, unsigned int flags, 
				 unsigned short type, unsigned int name_size, fcb_t *fcb );

/**********************************************************************

    Function    : fsDirInitialize
    Description : Create a memory dir for the dentry
    Inputs      : dentry - directory entry
                  flags - flags for dir
                  diskdir - on-disk directory reference  (optional)
    Outputs     : new dir or NULL

***********************************************************************/

extern dir_t *fsDirInitialize( dentry_t *dentry, unsigned int flags, ddir_t *diskdir );

/**********************************************************************

    Function    : fsDentryInitialize
    Description : Create a memory dentry for this file
    Inputs      : name - file name 
                  disk_dentry - on-disk dentry object (optional)
                  name_size - length of file name
                  type - dentry type
    Outputs     : new directory entry or NULL

***********************************************************************/

extern dentry_t *fsDentryInitialize( char *name, ddentry_t *disk_dentry, unsigned int name_size, unsigned int type );

/**********************************************************************

    Function    : fsAddDentry
    Description : Add the dentry to its directory
    Inputs      : dir - directory object
                  dentry - dentry object
    Outputs     : 0 if success, -1 if error 

***********************************************************************/

extern int fsAddDentry( dir_t *dir, dentry_t *dentry );

/**********************************************************************

    Function    : fsAddFile
    Description : Add the file to the system-wide open file cache
    Inputs      : filetable - system-wide file table 
                  file - file to be added
    Outputs     : a file descriptor, or -1 on error 

***********************************************************************/

extern int fsAddFile( file_t **filetable, file_t *file);

/**********************************************************************

    Function    : fileCreate
    Description : Create directory entry and file object
    Inputs      : name - name of the file 
                  flags - creation options 
                  constrain - flags to control resolution
    Outputs     : new file descriptor or -1 if error

***********************************************************************/

extern int fileCreate( char *name, unsigned int flags, unsigned int constrain );

/**********************************************************************

    Function    : fileOpen
    Description : Open directory entry of specified name
    Inputs      : name - name of the file 
                  flags - creation options
                  constrain - flags to control resolution
    Outputs     : new file descriptor or -1 if error

***********************************************************************/

extern int fileOpen( char *name, unsigned int flags, unsigned int constrain );

/**********************************************************************

    Function    : fileWrite
    Description : Write specified number of bytes starting at the current file index
    Inputs      : fd - file descriptor
                  buf - buffer to write
                  bytes - number of bytes to write
    Outputs     : number of bytes written

***********************************************************************/

extern int fileWrite( unsigned int fd, char *buf, unsigned int bytes );

/**********************************************************************

    Function    : fileRead
    Description : Read specified number of bytes from the current file index
    Inputs      : fd - file descriptor
                  buf - buffer for placing data
                  bytes - number of bytes to read
    Outputs     : number of bytes read

***********************************************************************/

extern int fileRead( unsigned int fd, char *buf, unsigned int bytes );

/**********************************************************************

    Function    : fsFindDentry
    Description : Retrieve the dentry for this file name
    Inputs      : dir - directory in which to look for name 
                  name - name of the file 
                  name_size - length of file name
                  constrain - flags to control resolution
    Outputs     : new dentry or NULL if error

***********************************************************************/

extern dentry_t *fsFindDentry( dir_t *dir, char *name, unsigned int name_size, unsigned int constrain );

/**********************************************************************

    Function    : fsFindFile
    Description : Retrieve the in-memory file for this file name
    Inputs      : parent - dir containing the file
                  dentry - dentry in which to look for name 
                  name - name of the file 
                  flags - flags requested for this file
                  type - type of file
                  name_size - length of file name
    Outputs     : new file or NULL if error

***********************************************************************/

extern file_t *fsFindFile( dir_t *parent, dentry_t *dentry, char *name, unsigned int flags, unsigned short type, unsigned int name_size );

/**********************************************************************

    Function    : listDirectory
    Description : print the files in the root directory currently
    Inputs      : none
    Outputs     : number of bytes read

***********************************************************************/

extern void listDirectory( void );


/**********************************************************************

    Function    : fileClose
    Description : close the file associated with the file descriptor
    Inputs      : fd - file descriptor
    Outputs     : none

***********************************************************************/

extern void fileClose( unsigned int fd );

/**********************************************************************

    Function    : fileSeek
    Description : Adjust offset in per-process file entry
    Inputs      : fd - file descriptor
                  index - new offset 
    Outputs     : 0 on success, -1 on failure

***********************************************************************/

extern int fileSeek( unsigned int fd, unsigned int index );

/**********************************************************************

    Function    : fileWrite
    Description : Write specified number of bytes starting at the current file index
    Inputs      : fd - file descriptor
                  buf - buffer to write
                  bytes - number of bytes to write
    Outputs     : number of bytes written

***********************************************************************/

extern int fileWrite( unsigned int fd, char *buf, unsigned int bytes );

/**********************************************************************

    Function    : dirCreate
    Description : Create directory entry and dir object
    Inputs      : name - name of the dir 
                  flags - creation options
                  constrain - flags to control resolution
    Outputs     : 0 on success or -1 if error

***********************************************************************/

extern int dirCreate( char *name, unsigned int flags, unsigned int constrain );

/**********************************************************************

    Function    : fileLink
    Description : Create a symbolic link to a file
    Inputs      : target - name of the target file
                  name - name of the file 
                  constrain - flags to control resolution
    Outputs     : 0 on success or -1 if error

	Notes:
	 - Only supports links to files and not directories.
	 - Need to implement FLAG_NOFOLLOW if dir symlinks are supported.

***********************************************************************/

extern int fileLink( char *target, char *name, unsigned int constrain );

/**********************************************************************

    Function    : fsGetLinkTarget
    Description : Get the target for a symbolic link file
    Inputs      : file - in-memory pointer to the file
                  target - buffer to write the symbolic link's target
                  target_len - size of target buffer in bytes
    Outputs     : 0 on success or -1 if error

***********************************************************************/

extern int fsGetLinkTarget( file_t *file, char *target, unsigned int target_len );

/**********************************************************************

    Function    : switchUser
    Description : Adjust offset in per-process file entry
    Inputs      : uid - new user uid
    Outputs     : 0 on success, -1 on failure

***********************************************************************/

extern int switchUser( unsigned int new_uid );

/**********************************************************************

    Function    : pathParse
    Description : Construct path_t from a standard UNIX path. Only
                  absolute path strings are supported.
    Inputs      : path - standard UNIX path string
    Outputs     : the path object

***********************************************************************/

extern path_t *pathParse( char *path );

/**********************************************************************

    Function    : pathNextName
    Description : Fetch the next dentry name for resolution
    Inputs      : p - the path
    Outputs     : name of dentry

***********************************************************************/

extern char *pathNextName( path_t *p );

/**********************************************************************

    Function    : pathToStr
    Description : Convert path_t to standard UNIX path. The path
                  is parsed till stopat + 1.
    Inputs      : p - the path
                  buf - write path to this buffer
                  buf_size - size of buf (in bytes)
    Outputs     : None

***********************************************************************/

extern void pathToStr( path_t *p, char *buf, unsigned int buf_size );

/**********************************************************************

    Function    : pathFullyResolved
    Description : Are all the dir elements in the path resolved?
    Inputs      : p - the path
    Outputs     : TRUE or FALSE

***********************************************************************/

extern int pathFullyResolved( path_t *p );

/**********************************************************************

    Function    : pathMergeWith
    Description : Get a new path based on a partially resolved path
                  and its symbolic link's target. It is used to resolve
                  paths that contain symbolic links to directories.
    Inputs      : p - base path that is partially resolved
                  newpath - the target name
    Outputs     : the new combined path

***********************************************************************/

extern path_t *pathMergeWithSymlink( path_t *p, char *newpath );

/**********************************************************************

    Function    : pathPrint
    Description : Print the path. The stopat index is marked with ^.
    Inputs      : p - the path
    Outputs     : None

***********************************************************************/

extern void pathPrint( path_t *p );

/**********************************************************************

    Function    : pathError
    Description : Report error during path resolution
    Inputs      : p - the path
    Outputs     : None

***********************************************************************/

extern void pathError( path_t *p );

/**********************************************************************

    Function    : pathFree
    Description : Destroy path and free the memory
    Inputs      : p - path to free
    Outputs     : None

***********************************************************************/

extern void pathFree( path_t *p );

#endif

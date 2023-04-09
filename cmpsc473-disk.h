https://tutorcs.com
WeChat: cstutorcs
QQ: 749389476
Email: tutorcs@163.com
#ifndef DISK_H
#define DISK_H

/**********************************************************************

   File          : cmpsc473-disk.h

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

/* macros */
#define block2offset( block ) ( FS_BLOCKSIZE * block )
#define dentry2offset( dentry_index ) ( sizeof(dblock_t) + ( sizeof(ddentry_t) * dentry_index )) // for ddentry's
#define FS_BLKDATA ( FS_BLOCKSIZE - sizeof(dblock_t) )


/* set the index in bitmap to 0 */
#define clearbit( bitmap, index, max ) ( bitmap &= ~( 1 << ( index % max )))

/* get the first free index in the bitmap */
static inline int findbit( unsigned int bitmap, unsigned int max ) 
{
	int i;

	for ( i = 0; i < max; i++ ) {
		if ( bitmap & ( 1 << i )) {
			return i;
		}
	}
	return BLK_INVALID;
}


static inline ddh_t *diskDirBucket( ddir_t *diskdir, int key ) 
{
	ddh_t *ddh = (ddh_t *)diskdir->data;

	ddh += key;

	return ddh;
}

/* functions */

/**********************************************************************

    Function    : diskDirInitialize
    Description : Initialize the root directory on disk
    Inputs      : directory reference
    Outputs     : 0 if success, -1 on error

***********************************************************************/

extern int diskDirInitialize( ddir_t *ddir );

/**********************************************************************

    Function    : diskReadDir
    Description : Retrieve the on-disk directory -- only one in this case
    Inputs      : name - name of the file 
                  name_size - length of file name
    Outputs     : on-disk directory or -1 if error

***********************************************************************/

extern ddir_t *diskReadDir( char *name, unsigned int name_size );


/**********************************************************************

    Function    : diskCreateDentry
    Description : Create disk entry for the dentry on directory
    Inputs      : base - ptr to base of file system on disk
                  dir - in-memory directory
                  dentry - in-memory dentry
    Outputs     : none

***********************************************************************/

extern void diskCreateDentry( unsigned long long base, dir_t *dir, dentry_t *dentry );

/**********************************************************************

    Function    : diskCreateDir
    Description : Create dir types for the new directory
    Inputs      : base - ptr to base of file system on disk
                  dentry - in-memory dentry
                  dir - in-memory file
    Outputs     : 0 on success, <0 on error 

***********************************************************************/

extern int diskCreateDir( unsigned long long base, dentry_t *dentry, dir_t *dir );

/**********************************************************************

    Function    : diskCreateFile
    Description : Create file block for the new file
    Inputs      : base - ptr to base of file system on disk
                  dentry - in-memory dentry
                  file - in-memory file
    Outputs     : 0 on success, <0 on error

***********************************************************************/

extern int diskCreateFile( unsigned long long base, dentry_t *dentry, file_t *file );

/**********************************************************************

    Function    : diskFindFile
    Description : Retrieve the on-disk file from the on-disk dentry
    Inputs      : disk_dentry - on-disk dentry
    Outputs     : on-disk file control block or NULL if error

***********************************************************************/

extern fcb_t *diskFindFile( ddentry_t *disk_dentry );

/**********************************************************************

    Function    : diskFindDentry
    Description : Retrieve the on-disk dentry from the disk directory
    Inputs      : diskdir - on-disk directory
                  name - name of the file 
                  name_size - length of file name
    Outputs     : on-disk dentry or NULL if error

***********************************************************************/

extern ddentry_t *diskFindDentry( ddir_t *diskdir, char *name, unsigned int name_size );

/**********************************************************************

    Function    : diskWrite
    Description : Write the buffer to the disk
    Inputs      : disk_offset - reference to location where offset is stored on disk
                  block - index to block to be written
                  buf - data to be written
                  bytes - the number of bytes to write
                  offset - offset from start of file
                  sofar - bytes written so far
    Outputs     : number of bytes written or -1 on error 

***********************************************************************/

extern unsigned int diskWrite( unsigned int *disk_offset, unsigned int block, 
			       char *buf, unsigned int bytes, 
			       unsigned int offset, unsigned int sofar );

/**********************************************************************

    Function    : diskRead
    Description : read the buffer from the disk
    Inputs      : block - index to file block to read
                  buf - buffer for data
                  bytes - the number of bytes to read
                  offset - offset from start of file
                  sofar - bytes read so far 
    Outputs     : number of bytes read or -1 on error 

***********************************************************************/

extern unsigned int diskRead( unsigned int block, char *buf, unsigned int bytes, 
			      unsigned int offset, unsigned int sofar );

/**********************************************************************

    Function    : diskGetBlock
    Description : Get the block corresponding to this file location
    Inputs      : file - in-memory file pointer
                  index - block index in file
    Outputs     : block index or BLK_INVALID

***********************************************************************/

extern unsigned int diskGetBlock( file_t *file, unsigned int index );

/**********************************************************************

    Function    : allocDblock
    Description : Get a free data block
    Inputs      : index - index for the block found or BLK_INVALID
                  blk_type - the type of use for the block
    Outputs     : 0 on success, <0 on error                  

***********************************************************************/

extern int allocDblock( unsigned int *index, unsigned int blk_type );

/**********************************************************************

    Function    : deallocDblock
    Description : Return block to the free pool
    Inputs      : index - index for the block
    Outputs     : 0 on success, <0 on error

***********************************************************************/

/**********************************************************************

    Function    : diskFindDir
    Description : Retrieve the on-disk dentry from the on-disk dentry
    Inputs      : disk_dentry - on-disk dentry
    Outputs     : on-disk dentry or NULL if error

***********************************************************************/

extern ddir_t *diskFindDir( ddentry_t *disk_dentry );


/**********************************************************************

    Function    : diskCanonicalizeName
    Description : Canonicalize name for the given on-disk dir
    Inputs      : diskdir - canonicalize for this diskdir
                  name - name to canonicalize
                  name_size - length of name string in bytes
                  target - buffer where the canonicalized name is written
                  target_size - length of target buffer in bytes
    Outputs     : TRUE (or 1) on success; FALSE (or 0) on error                  

***********************************************************************/

extern int diskCanonicalizeName( ddir_t *diskdir, char *name, unsigned int name_size, char *target, unsigned int target_size );


/**********************************************************************

    Function    : diskStrCompare
    Description : Compare names for the given on-disk dir
    Inputs      : diskdir - compare name for this diskdir
                  name1 - first name string
                  name1_size - length of name1 string in bytes
                  name2 - second name string
                  name2_size - length of name2 string in bytes
    Outputs     : TRUE (or 1) on match; otherwise FALSE (or 0)                  

***********************************************************************/

extern int diskStrCompare( ddir_t *diskdir, char *name1, unsigned int name1_size, char *name2, unsigned int name2_size );


/**********************************************************************

    Function    : diskCheckDentryConstraint
    Description : Does this on-disk dentry pass the constraint?
    Inputs      : disk_dentry - compare name with this on-disk dentry
                  name - to match with the on-disk dentry
                  name_size - length of name string in bytes
                  constrain - for resolving the name; e.g., FLAG_SAVEDNAME
    Outputs     : TRUE (or 1) on match; otherwise FALSE (or 0)                  

***********************************************************************/

extern int diskCheckDentryConstraint( ddentry_t *disk_dentry, char *name, unsigned int name_size, unsigned int constrain );

#endif

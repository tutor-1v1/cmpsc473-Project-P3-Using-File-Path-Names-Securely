https://tutorcs.com
WeChat: cstutorcs
QQ: 749389476
Email: tutorcs@163.com

/**********************************************************************

   File          : cmpsc473-disk.c

   Description   : File system function implementations
                   (see .h for applications)

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

/* Include Files */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

/* Project Include Files */
#include "cmpsc473-filesys.h"
#include "cmpsc473-disk.h"
#include "cmpsc473-list.h"
#include "cmpsc473-util.h"

/* Definitions */

/* program variables */

/* Functions */


/**********************************************************************

    Function    : diskDirInitialize
    Description : Initialize the root directory on disk
    Inputs      : directory reference
    Outputs     : 0 if success, -1 on error

***********************************************************************/

int diskDirInitialize( ddir_t *ddir )
{
	/* Local variables */
	dblock_t *first_dentry_block;
	int i;
	ddh_t *ddh; 

	/* clear disk directory object */
	memset( ddir, 0, FS_BLOCKSIZE );

	/* initialize disk directory fields */
	ddir->buckets = ( FS_BLOCKSIZE - sizeof(ddir_t) ) / (sizeof(ddh_t));
	ddir->freeblk = FS_METADATA_BLOCKS+1;    /* fs+super+directory */
	ddir->free = 0;                          /* dentry offset in that block */
	ddir->flags = 0;                         /* root dir is case-sensitive */

	/* assign first dentry block - for directory itself */
	first_dentry_block = (dblock_t *)disk2addr( fs->base, 
						    (( FS_METADATA_BLOCKS+1 ) 
						     * FS_BLOCKSIZE ));
	memset( first_dentry_block, 0, FS_BLOCKSIZE );
	first_dentry_block->free = DENTRY_BLOCK;
	first_dentry_block->st.dentry_map = DENTRY_MAP;
	first_dentry_block->next = BLK_INVALID;

	/* initialize ddir hash table */
	ddh = (ddh_t *)ddir->data;     /* start of hash table data -- in ddh_t's */
	for ( i = 0; i < ddir->buckets; i++ ) {
		(ddh+i)->next_dentry = BLK_SHORT_INVALID;
		(ddh+i)->next_slot = BLK_SHORT_INVALID;
	}

	return 0;  
}


/**********************************************************************

    Function    : diskReadDir
    Description : Retrieve the on-disk directory -- only one in this case
    Inputs      : name - name of the file 
                  name_size - length of name
    Outputs     : on-disk directory or -1 if error

***********************************************************************/

/* Deprecated in lieu of diskFindDir() */
ddir_t *diskReadDir( char *name, unsigned int name_size ) 
{
	return ((ddir_t *)block2addr( fs->base, dfs->root ));
}


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

int diskCanonicalizeName( ddir_t *diskdir, char *name, unsigned int name_size, char *target, unsigned int target_size )
{
	if ( target_size < name_size + 1 )
		return FALSE; /* not enough space */

	/* copy name to target */
	int i;
	for ( i = 0; i < strlen(name); i++ )
		if ( (diskdir->flags & D_ICASE) )
			/* case-insensitive directory */
			target[i] = tolower(name[i]);
		else
			target[i] = name[i];
	target[i] = '\0';

	return TRUE;
}


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

int diskStrCompare( ddir_t *diskdir, char *name1, unsigned int name1_size, char *name2, unsigned int name2_size )
{
	int i;

	if ( name1_size != name2_size )
		return FALSE;

	if ( diskdir->flags & D_ICASE ) {
		/* case-insensitive match */
		for ( i = 0; i < name1_size; i++ ) {
			if ( tolower(name1[i]) != tolower(name2[i]) )
				return FALSE;
		}
		return TRUE;
	} else if ( strncmp( name1, name2, name1_size ) == 0 )
		return TRUE;

	return FALSE;
}


/**********************************************************************

    Function    : diskCheckDentryConstraint
    Description : Does this on-disk dentry pass the constraint?
    Inputs      : disk_dentry - compare name with this on-disk dentry
                  name - to match with the on-disk dentry
                  name_size - length of name string in bytes
                  constrain - for resolving the name; e.g., FLAG_SAVEDNAME
    Outputs     : TRUE (or 1) on match; otherwise FALSE (or 0)                  

***********************************************************************/

int diskCheckDentryConstraint( ddentry_t *disk_dentry, char *name, unsigned int name_size, unsigned int constrain )
{
	/* Task 4c: Verify the constaint FLAG_SAVEDNAME */

	return TRUE;
}


/**********************************************************************

    Function    : diskFindDentry
    Description : Retrieve the on-disk dentry from the disk directory
    Inputs      : diskdir - on-disk directory
                  name - name of the file 
                  name_size - length of file name
    Outputs     : on-disk dentry or NULL if error

***********************************************************************/

ddentry_t *diskFindDentry( ddir_t *diskdir, char *name, unsigned int name_size ) 
{
	char canonical_name[MAX_NAME_SIZE];
	if ( !diskCanonicalizeName( diskdir, name, name_size, canonical_name, MAX_NAME_SIZE ) )
		return (ddentry_t *)NULL;

	int key = fsMakeKey( canonical_name, diskdir->buckets, name_size );
	ddh_t *ddh = (ddh_t *)&diskdir->data[key];

	// find block in cache?  if not get from disk and put in cache

	while (( ddh->next_dentry != BLK_SHORT_INVALID ) || ( ddh->next_slot != BLK_SHORT_INVALID )) {
		dblock_t *dblk = (dblock_t *)disk2addr( fs->base, (block2offset( ddh->next_dentry )));
		ddentry_t *disk_dentry = (ddentry_t *)disk2addr( dblk, dentry2offset( ddh->next_slot ));
    
		if ( diskStrCompare( diskdir, disk_dentry->name, disk_dentry->name_size, name, name_size ) )
			return disk_dentry;

		ddh = &disk_dentry->next;
	}

	return (ddentry_t *)NULL;  
}


/**********************************************************************

    Function    : diskFindFile
    Description : Retrieve the on-disk file from the on-disk dentry
    Inputs      : disk_dentry - on-disk dentry
    Outputs     : on-disk file control block or NULL if error

    LAYOUT: HEADER + fcb_t

***********************************************************************/

fcb_t *diskFindFile( ddentry_t *disk_dentry ) 
{
	if ( disk_dentry->block != BLK_INVALID ) {
		dblock_t *blk =  (dblock_t *)disk2addr( fs->base, (block2offset( disk_dentry->block )));
		return (fcb_t *)disk2addr( blk, sizeof(dblock_t) );
	}

	errorMessage("diskFindFile: no such file");
	printf("\nfile name = %s\n", disk_dentry->name);
	return (fcb_t *)NULL;  
}


/**********************************************************************

    Function    : diskFindDir
    Description : Retrieve the on-disk dentry from the on-disk dentry
    Inputs      : disk_dentry - on-disk dentry
    Outputs     : on-disk dentry or NULL if error

***********************************************************************/

ddir_t *diskFindDir( ddentry_t *disk_dentry )
{
	if ( disk_dentry->block != BLK_INVALID )
		return (ddir_t *) disk2addr( fs->base, (block2offset( disk_dentry->block )));

	errorMessage("diskFindDir: no such dir");
	printf("\ndir name = %s\n", disk_dentry->name);
	return (ddir_t *)NULL;  
}


/**********************************************************************

    Function    : diskCreateDentry
    Description : Create disk entry for the dentry on directory
    Inputs      : base - ptr to base of file system on disk
                  dir - in-memory directory
                  dentry - in-memory dentry
    Outputs     : none

***********************************************************************/

void diskCreateDentry( unsigned long long base, dir_t *dir, dentry_t *dentry ) 
{
	ddir_t *diskdir = dir->diskdir;
	ddentry_t *disk_dentry;
	dblock_t *dblk, *nextblk;
	ddh_t *ddh;
	int empty = 0;
	int key;
	char canonical_name[MAX_NAME_SIZE];

	// create buffer cache for blocks retrieved from disk - not mmapped

	/* find location for new on-disk dentry */
	dblk = (dblock_t *)disk2addr( base, (block2offset( diskdir->freeblk )));
	disk_dentry = (ddentry_t *)disk2addr( dblk, dentry2offset( diskdir->free ));

	/* associate dentry with ddentry */
	dentry->diskdentry = disk_dentry;  
  
	/* update disk dentry with dentry's data */
	memcpy( disk_dentry->name, dentry->name, dentry->name_size );  // check bounds in dentry
	disk_dentry->name[dentry->name_size] = 0;   // null terminate
	disk_dentry->name_size = dentry->name_size;
 	disk_dentry->block = BLK_INVALID;

	/* push disk dentry into on-disk hashtable */
	if ( !diskCanonicalizeName( diskdir, disk_dentry->name, disk_dentry->name_size, canonical_name, MAX_NAME_SIZE ) ) {
		warningMessage( "diskCreateDentry: failed to canonicalize name: ");
		fprintf( stderr, "%s\n", disk_dentry->name );
	}
	key = fsMakeKey( canonical_name, diskdir->buckets, disk_dentry->name_size );
	ddh = diskDirBucket( diskdir, key );
	/* at diskdir's hashtable bucket "key", make this disk_dentry the next head
	   and link to the previous head */
	disk_dentry->next.next_dentry = ddh->next_dentry;   
	disk_dentry->next.next_slot = ddh->next_slot;       
	ddh->next_dentry = diskdir->freeblk;
	ddh->next_slot = diskdir->free;

	/* set this disk_dentry as no longer free in the block */
	clearbit( dblk->st.dentry_map, diskdir->free, DENTRY_MAX );   

	/* update free reference for dir */
	/* first the block, if all dentry space has been consumed */
	if ( dblk->st.dentry_map == 0 ) { /* no more space for dentries here */
		/* need another directory block for disk dentries */
		/* try "next" block first until no more */
		unsigned int next_index = dblk->next;
		while ( next_index != BLK_INVALID ) {
			nextblk = (dblock_t *)disk2addr( base, block2offset( next_index ));
			if ( nextblk->st.dentry_map != 0 ) {
				diskdir->freeblk = next_index;
				dblk = nextblk;
				goto done;
			}
			next_index = nextblk->next;
		}
		
		/* get next file system free block for next dentry block */
		diskdir->freeblk = dfs->firstfree;
      
		/* update file system's free blocks */
		nextblk = (dblock_t *)disk2addr( base, block2offset( dfs->firstfree ));
		dfs->firstfree = nextblk->next;
		nextblk->free = DENTRY_BLOCK;   /* this is now a dentry block */
		nextblk->st.dentry_map = DENTRY_MAP;
		nextblk->next = BLK_INVALID;
		dblk = nextblk;
	}

done:
	/* now update the free entry slot in the block */
	/* find the empty dentry slot */
	empty = findbit( dblk->st.dentry_map, DENTRY_MAX );
	diskdir->free = empty;

	if (empty == BLK_INVALID ) {
		errorMessage("diskCreateDentry: bad bitmap");
		return;
	}      
}


/**********************************************************************

    Function    : diskCreateDir
    Description : Create dir types for the new directory
    Inputs      : base - ptr to base of file system on disk
                  dentry - in-memory dentry
                  dir - in-memory file
    Outputs     : 0 on success, <0 on error 

	BLOCK LAYOUT for ddir:     NO HEADER + ddir_t
	BLOCK LAYOUT for ddentry:  HEADER + ddentry_t

***********************************************************************/

int diskCreateDir( unsigned long long base, dentry_t *dentry, dir_t *dir )
{
	ddir_t *ddir;
	dblock_t *dblk;
	int i;
	unsigned int blk_ddir, blk_ddentry;
	ddh_t *ddh;
	ddentry_t *disk_dentry;

	allocDblock( &blk_ddir, DIR_BLOCK );
	if ( blk_ddir == BLK_INVALID ) {
		return -1;
	}

	allocDblock( &blk_ddentry, DENTRY_BLOCK );
	if ( blk_ddir == BLK_INVALID ) {
		return -1;
	}
  
	/* find a file block in file system */
	ddir = (ddir_t *) disk2addr( base, (block2offset( blk_ddir )));
	dblk = (dblock_t *) disk2addr( base, (block2offset( blk_ddentry )));

	/* clear disk directory object */
	memset( ddir, 0, FS_BLOCKSIZE );

	/* initialize disk directory fields */
	ddir->buckets = ( FS_BLOCKSIZE - sizeof(ddir_t) ) / (sizeof(ddh_t));
	ddir->freeblk = blk_ddentry;      /* block that stores dentries */
	ddir->free = 0;                   /* dentry offset in that block */
	ddir->flags = dir->flags;         /* root dir is case-sensitive */

	/* initialize dentry block */
	memset( dblk, 0, FS_BLOCKSIZE );
	dblk->free = DENTRY_BLOCK;
	dblk->st.dentry_map = DENTRY_MAP;
	dblk->next = BLK_INVALID;

	/* initialize ddir hash table */
	ddh = (ddh_t *)ddir->data;     /* start of hash table data -- in ddh_t's */
	for ( i = 0; i < ddir->buckets; i++ ) {
		(ddh+i)->next_dentry = BLK_SHORT_INVALID;
		(ddh+i)->next_slot = BLK_SHORT_INVALID;
	}

	/* associate in-memory & on-disk structures */
	dir->diskdir = ddir;
	dir->bucket_size = ddir->buckets;
	// TODO: hacky? repeated from fsDirInitialize()
	dir->buckets = (!dir->buckets) ? malloc(ddir->buckets * sizeof(dentry_t *)) : NULL;

	/* get on-disk dentry */
	disk_dentry = dentry->diskdentry;

	/* set dir's block & type in on-disk dentry */
	disk_dentry->block = blk_ddir;
	disk_dentry->type = DTYPE_DIR;

	return 0;
}


/**********************************************************************

    Function    : diskCreateFile
    Description : Create file block for the new file
    Inputs      : base - ptr to base of file system on disk
                  dentry - in-memory dentry
                  file - in-memory file
    Outputs     : 0 on success, <0 on error 

	BLOCK LAYOUT for fcb:   HEADER + fcb_t
	BLOCK LAYOUT for data:  HEADER + data (see diskWrite())

***********************************************************************/

int diskCreateFile( unsigned long long base, dentry_t *dentry, file_t *file )
{
	dblock_t *fblk;
	fcb_t *fcb;
	ddentry_t *disk_dentry;
	int i;
	unsigned int block;

	allocDblock( &block, FILE_BLOCK );

	if ( block == BLK_INVALID ) {
		return -1;
	}  
  
	/* find a file block in file system */
	fblk = (dblock_t *)disk2addr( base, (block2offset( block )));
	fcb = (fcb_t *)disk2addr( fblk, sizeof( dblock_t ));   /* file is offset from block info */

	/* associate file with the on-disk file */
	file->diskfile = fcb;

	/* Task 1a: Prepare fcb_t for symlink */
	/* Set the uid and type files of fcb */

	/* set file data into file block */
	fcb->flags = file->flags;
	/* XXX initialize attributes */
	fcb->attr_block = BLK_INVALID;    /* no block yet */

	/* initial on-disk block information for file */  
	for ( i = 0; i < FILE_BLOCKS; i++ ) {
		fcb->blocks[i] = BLK_INVALID;   /* initialize to empty */
	}

	/* get on-disk dentry */
	disk_dentry = dentry->diskdentry;

	/* set file's block & type in on-disk dentry */
	disk_dentry->block = block;
	disk_dentry->type = DTYPE_FILE;

	return 0;
}


/**********************************************************************

    Function    : diskWrite
    Description : Write the buffer to the disk
    Inputs      : disk_offset - pointer to place where offset is stored on disk
                  block - index to block to be written
                  buf - data to be written
                  bytes - the number of bytes to write
                  offset - offset from start of file
                  sofar - bytes written so far
    Outputs     : number of bytes written or -1 on error 

***********************************************************************/

unsigned int diskWrite( unsigned int *disk_offset, unsigned int block, 
			char *buf, unsigned int bytes, 
			unsigned int offset, unsigned int sofar )
{
	dblock_t *dblk;
	char *start, *end, *data;
	int block_bytes;
	unsigned int blk_offset = offset % FS_BLKDATA;

	/* compute the block addresses and range */
	dblk = (dblock_t *)disk2addr( fs->base, (block2offset( block )));
	data = (char *)disk2addr( dblk, sizeof(dblock_t) );
	start = (char *)disk2addr( data, blk_offset );
	end = (char *)disk2addr( fs->base, (block2offset( (block+1) )));
	block_bytes = min(( end - start ), ( bytes - sofar ));

	/* do the write */
	memcpy( start, buf, block_bytes );
  
	/* compute new offset, and update in fcb if end is extended */
	offset += block_bytes;
  
	if ( offset > *disk_offset ) {
		*disk_offset = offset;
	}

	return block_bytes;  
}


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

unsigned int diskRead( unsigned int block, char *buf, unsigned int bytes, 
		       unsigned int offset, unsigned int sofar )
{
	dblock_t *dblk;
	char *start, *end, *data;
	int block_bytes;
	unsigned int blk_offset = offset % FS_BLKDATA;

	/* compute the block addresses and range */
	dblk = (dblock_t *)disk2addr( fs->base, (block2offset( block )));
	data = (char *)disk2addr( dblk, sizeof(dblock_t) );
	start = (char *)disk2addr( data, blk_offset );
	end = (char *)disk2addr( fs->base, (block2offset( (block+1) )));
	block_bytes = min(( end - start ), ( bytes - sofar ));

	/* do the read */
	memcpy( buf, start, block_bytes );

	return block_bytes;  
}


/**********************************************************************

    Function    : diskGetBlock
    Description : Get the block corresponding to this file location
    Inputs      : file - in-memory file pointer
                  index - block index in file
    Outputs     : block index or BLK_INVALID

***********************************************************************/

unsigned int diskGetBlock( file_t *file, unsigned int index )
{
	fcb_t *fcb = file->diskfile;
	unsigned int dblk_index;

	if ( fcb == NULL ) {
		errorMessage("diskGetBlock: No file control block for file");
		return BLK_INVALID;
	}

	/* if the index is already in the file control block, then return that */
	dblk_index = fcb->blocks[index]; 
 
	if ( dblk_index != BLK_INVALID ) {
		return dblk_index;
	}

	allocDblock( &dblk_index, FILE_DATA );

	if ( dblk_index == BLK_INVALID ) {
		return BLK_INVALID;
	}

	// P3: Meta-Data 
	/* update the fcb with the new block */
	fcb->blocks[index] = dblk_index;

	return dblk_index;
}


/**********************************************************************

    Function    : allocDblock
    Description : Get a free data block
    Inputs      : index - index for the block found or BLK_INVALID
                  blk_type - the type of use for the block
    Outputs     : 0 on success, <0 on error                  

***********************************************************************/

int allocDblock( unsigned int *index, unsigned int blk_type ) 
{
	dblock_t *dblk;

	/* if there is no free block, just return */
	if ( dfs->firstfree == BLK_INVALID ) {
		*index = BLK_INVALID;
		return BLK_INVALID;
	}

	/* get from file system's free list */
	*index = dfs->firstfree;

	/* update the filesystem's next free block */
	dblk = (dblock_t *)disk2addr( fs->base, (block2offset( *index )));

	/* mark block as a file block */
	dblk->free = blk_type;

	/* update next freeblock in file system */
	// P3 - metadata below
	dfs->firstfree = dblk->next;

	return 0;
}

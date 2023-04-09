https://tutorcs.com
WeChat: cstutorcs
QQ: 749389476
Email: tutorcs@163.com
/**********************************************************************

   File          : cmpsc473-filesys.c

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

/* Project Include Files */
#include "cmpsc473-filesys.h"
#include "cmpsc473-disk.h"
#include "cmpsc473-list.h"
#include "cmpsc473-util.h"

/* Definitions */

/* macros */

/* program variables */

/* Functions */

/**********************************************************************

    Function    : pathParse
    Description : Construct path_t from a standard UNIX path. Only
                  absolute path strings are supported.
    Inputs      : path - standard UNIX path string
    Outputs     : the path object

***********************************************************************/

path_t *pathParse( char *path )
{
	path_t *p;
	char *pathdup, *str;
	char *sep = "/";

	if ( !path || strlen(path) > MAX_PATH_SIZE )
		return NULL;

	pathdup = strdup( path );
	p = malloc( sizeof( path_t ) );
	p->num = 0;
	p->stopat = -1;
	p->err = 0;
	p->dir = NULL;
	p->dentry = NULL;

	/* first element is always '/' */
	p->name[p->num++] = strdup( "/" );

	/* break into compoments */
	while( (str = strsep( &pathdup, sep )) != NULL ) {
		if ( strlen( str ) == 0 )
			continue; /* ignore multiple '/' */

		p->name[p->num] = strdup( str );
		// printf("%p: %s\n", p->name[p->num], p->name[p->num]);
		p->num++;
	}

	free( pathdup );
	return p;
}


/**********************************************************************

    Function    : pathNextName
    Description : Fetch the next dentry name for resolution
    Inputs      : p - the path
    Outputs     : name of dentry

***********************************************************************/

char *pathNextName( path_t *p )
{
	if ( !p
	     || p->num < 2              /* ignore if only root */
	     || p->stopat < 0           /* traversal not done */
		 || p->stopat == p->num-1 ) /* at the end */
		return NULL;

	return p->name[p->stopat+1];
}


/**********************************************************************

    Function    : pathToStr
    Description : Convert path_t to standard UNIX path. The path
                  is parsed till stopat + 1.
    Inputs      : p - the path
                  buf - write path to this buffer
                  buf_size - size of buf (in bytes)
    Outputs     : None

***********************************************************************/

void pathToStr( path_t *p, char *buf, unsigned int buf_size )
{
	int len=0;
	int till;
	int i;

	if ( !p )
		return;

	/* till */
	if ( p->stopat < 0 )
		till = p->num-1;
	else
		till = p->stopat+1; /* include the file being resolved */

	/* write to buffer */
	len = snprintf( buf, buf_size, "/" );
	for ( i = 1; i <= till; i++ ) {
		len += snprintf( buf+len, buf_size-len, "%s/", p->name[i] );
	}

	/* sanity */
	buf[buf_size-1] = '\0';
	if ( len <= buf_size )
		buf[len-1] = '\0'; // remove '/'
}


/**********************************************************************

    Function    : pathFullyResolved
    Description : Are all the dir elements in the path resolved?
    Inputs      : p - the path
    Outputs     : TRUE or FALSE

***********************************************************************/

int pathFullyResolved( path_t *p )
{
	if ( (p->stopat + 1) == (p->num - 1) )
		return TRUE;
	else
		return FALSE;
}


/**********************************************************************

    Function    : pathMergeWith
    Description : Get a new path based on a partially resolved path
                  and its symbolic link's target. It is used to resolve
                  paths that contain symbolic links to directories.
    Inputs      : p - base path that is partially resolved
                  newpath - the target name
    Outputs     : the new combined path

***********************************************************************/

path_t *pathMergeWithSymlink( path_t *p, char *newpath )
{
	/* Task 3a (optional) */
	
	return NULL;
}


/**********************************************************************

    Function    : pathPrint
    Description : Print the path. The stopat index is marked with ^.
    Inputs      : p - the path
    Outputs     : None

***********************************************************************/

void pathPrint( path_t *p )
{
	int i;

	if ( !p )
		return;

	for ( i = 0; i < p->num; i++ )
		printf("[%s]%s -> ", p->name[i], (p->stopat == i) ? "^" : "" );
	printf("END\n");
}


/**********************************************************************

    Function    : pathError
    Description : Report error during path resolution
    Inputs      : p - the path
    Outputs     : None

***********************************************************************/

void pathError( path_t *p )
{
	int len = 0, size = MAX_NAME_SIZE;
	char msg[size];

	if ( !p )
		return;

	/* Error context: name that already exits */
	if ( p->err == E_Exists
		&& p->stopat + 1 > 0
		&& p->stopat + 1 < p->num
		) {
		len += snprintf( msg+len, size-len, ": " );
		pathToStr( p, msg+len, size-len );
		msg[size-1] = '\0';
		fserror( p->err, msg );
		return;
	}

	fserror( p->err, NULL );
	printf( ERRCOLOR("Path: ") );
	pathPrint( p );
}


/**********************************************************************

    Function    : pathFree
    Description : Destroy path and free the memory
    Inputs      : p - path to free
    Outputs     : None

***********************************************************************/

void pathFree( path_t *p )
{
	int i;

	if ( !p )
		return;

	for ( i = p->num; i < p->num; i++ )
		free( p->name[i]) ;
	free( p );
}


/**********************************************************************

    Function    : fsRootDir
    Description : Retreive '/' for the file system
    Inputs      : None
    Outputs     : pointer to the '/' dir

***********************************************************************/

dir_t *fsRootDir()
{
	dir_t *dir;
	ddir_t *diskdir;

	/* there is only one directory in this file system */
	if ( fs->dir != NULL ) {
		return fs->dir;
	}

	/* otherwise retrieve the directory from the disk */
	diskdir = diskReadDir( "/", 1 );

	dir = fsDirInitialize( NULL, D_NONE, diskdir );

	/* assign the root directory to the in-memory file system */
	fs->dir = dir;
	
	return dir;
}


/**********************************************************************

    Function    : fsParentDir
    Description : Keep resolving directories until possible. Sets
                  p->stopat to the last successfully found dentry.
    Inputs      : path - to resolve
                  constrain - flags to control resolution
    Outputs     : tne parent dir

***********************************************************************/

dir_t *fsParentDir( struct path *p, unsigned int constrain )
{
	/* Task 2 */
	dir_t *pdir = fsRootDir();
	p->stopat = 0;

	/* Add code here */

	return pdir;
}


/**********************************************************************

    Function    : fsReadDir
    Description : Return directory entry for name
    Inputs      : name - name of the file 
                  name_size - length of file name
    Outputs     : new in-memory directory or NULL

***********************************************************************/

/* Deprecated: replaced with fsParentDir() */
dir_t *fsReadDir( char *name, unsigned int name_size )
{
	dir_t *dir;
	ddir_t *diskdir;

	/* there is only one directory in this file system */
	if ( fs->dir != NULL ) {
		return fs->dir;
	}

	/* otherwise retrieve the directory from the disk */
	diskdir = diskReadDir( name, name_size );
	dir = fsDirInitialize( NULL, D_NONE, diskdir );

	/* assign the root directory to the in-memory file system */
	fs->dir = dir;
	
	return dir;
}


/**********************************************************************

    Function    : fsFindDentry
    Description : Retrieve the dentry for this file name
    Inputs      : dir - directory in which to look for name 
                  name - name of the file 
                  name_size - length of file name
                  constrain - flags to control resolution
    Outputs     : new dentry or NULL if error

***********************************************************************/

dentry_t *fsFindDentry( dir_t *dir, char *name, unsigned int name_size, unsigned int constrain )
{
	char canonical_name[MAX_NAME_SIZE];
	if ( !diskCanonicalizeName( dir->diskdir, name, name_size, canonical_name, MAX_NAME_SIZE ) )
		return (dentry_t *)NULL;

	int key = fsMakeKey( canonical_name, dir->bucket_size, name_size );
	dentry_t *dentry = inList( dir, dir->buckets[key], name, name_size );


	/* Task 4c: Using diskCheckDentryConstraint(...), verify the constraints
	 * for this dentry. If the constraints do not match, then return NULL.
	 */



	/* if not cached already, have to get it from the disk */
	if ( dentry == NULL ) {
		ddentry_t *disk_dentry = diskFindDentry( dir->diskdir, name, name_size );

		if ( disk_dentry == NULL ) {
			return (dentry_t *)NULL;
		}

		/* constraint handling */
		if ( diskCheckDentryConstraint( disk_dentry, name, name_size, constrain ) == FALSE )
			goto error;

		/* build a blank in-memory directory entry for the new file */
		dentry = fsDentryInitialize( name, disk_dentry, name_size, disk_dentry->type );    
	}


	return dentry;

error:
	// errorMessage("fsFindDentry: does not match saved name");
	return (dentry_t *)NULL;
}


/**********************************************************************

    Function    : fsMatchFile
    Description : Test a file for given conditions
    Inputs      : file - file_t to match against
                  name - name of the file
                  parent - dir containing the file
                  flags - flags requested for this file
                  type - type of file
                  name_size - length of file name
    Outputs     : TRUE or FALSE

***********************************************************************/

int fsMatchFile( file_t *file, char *name, dir_t *parent, unsigned int flags, unsigned short type, unsigned int name_size )
{
	if ( file == NULL )
		goto false;

	/* setup */
	if ( type == FTYPE_SYMLINK || file->type == FTYPE_SYMLINK )
		flags = 0; /* ignore flags for symlinks */

	/* matching */
	if ( name_size >= 0 && file->name_size != name_size )
		goto false;

	if ( name && !diskStrCompare( parent->diskdir, file->name, file->name_size, name, name_size ) )
		goto false;

	if ( (flags | file->flags) != file->flags )
		goto false;

	if ( parent != file->parent )
		goto false;

	if ( type != FTYPE_UNSPECIFIED )
		if ( type != file->type )
			goto false;

	return TRUE;

false:
	return FALSE;
}


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

file_t *fsFindFile( dir_t *parent, dentry_t *dentry, char *name, unsigned int flags, unsigned short type, unsigned int name_size )
{
	file_t *file = dentry->file;

	/* if not cached already, have to get it from the disk */
	if ( file == NULL ) {
		ddentry_t *disk_dentry = dentry->diskdentry;
		fcb_t *fcb;  /* on disk file */

		if ( disk_dentry == NULL ) {
			errorMessage("fsFindFile: no on-disk entry must create one first");
			return (file_t *)NULL;
		}

		fcb = diskFindFile( disk_dentry );

		if ( fcb == NULL ) {
			errorMessage("fsFindFile: no on-disk file --  must create one first");
			return (file_t *)NULL;
		}

		/* build a blank in-memory directory entry for the new file */
		file = fsFileInitialize( parent, dentry, name, fcb->flags, fcb->type, name_size, fcb );
	}

	if ( fsMatchFile(file, name, parent, flags, type, name_size) )
		return file;

	return NULL;
}

/**********************************************************************

    Function    : fsCacheFindFile
    Description : Find if file is in the system-wide file table
    Inputs      : filetable - system-wide file table
                  name - file name
                  parent - dir containing the file
                  flags - file flags
                  type - type of file
                  name_size - length of file name
    Outputs     : file or NULL

***********************************************************************/

file_t *fsCacheFindFile( file_t **filetable, char *name, dir_t *parent, unsigned int flags, unsigned short type, unsigned int name_size )
{
	int i;

	for ( i = 0; i < FS_FILETABLE_SIZE; i++ ) {
		file_t *file = filetable[i];
		if(fsMatchFile(file, name, parent, flags, type, name_size))
			return file;
	}
  
	return (file_t *)NULL;
}

/**********************************************************************

    Function    : fsDentryInitialize
    Description : Create a memory dentry for this file
    Inputs      : name - file name 
                  disk_dentry - on-disk dentry object (optional)
                  name_size - length of file name
                  type - dentry type
    Outputs     : new directory entry or NULL

***********************************************************************/

dentry_t *fsDentryInitialize( char *name, ddentry_t *disk_dentry, unsigned int name_size, unsigned int type )
{
	dentry_t *dentry = (dentry_t *)malloc(sizeof(dentry_t)+name_size+1);

	if ( dentry == NULL ) {
		errorMessage("fsDentryInitialize: could not alloc dentry");
		return NULL;
	}

	dentry->file = (file_t *) NULL;
	dentry->dir = (dir_t *) NULL;
	memcpy( dentry->name, name, name_size );
	dentry->name[name_size] = 0;   // null terminate
	dentry->name_size = name_size;
	dentry->type = disk_dentry ? disk_dentry->type : type;
	dentry->diskdentry = disk_dentry;
	dentry->next = (dentry_t *) NULL;

	return dentry;
}


/**********************************************************************

    Function    : fsDirInitialize
    Description : Create a memory dir for the dentry
    Inputs      : dentry - directory entry
                  flags - flags for dir
                  diskdir - on-disk directory reference  (optional)
    Outputs     : new dir or NULL

***********************************************************************/

dir_t *fsDirInitialize( dentry_t *dentry, unsigned int flags, ddir_t *diskdir )
{
	dir_t *dir = (dir_t *) malloc(sizeof(dir_t));

	if ( dir == NULL ) {
		errorMessage("fsDirInitialize: could not alloc dir");
		return (dir_t *)NULL;
	}

	/* create in-memory dir */
	dir->diskdir     = diskdir;
	dir->flags       = ( diskdir ? diskdir->flags : flags );
	dir->bucket_size = ( diskdir ? diskdir->buckets : 0 );
	dir->buckets     = ( diskdir ? malloc(diskdir->buckets * sizeof(dentry_t *)) : NULL );

	/* associate dentry with file */
	if (dentry) {
		dentry->type = DTYPE_DIR;
		dentry->dir = dir;
	}
	return dir ;
}


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

file_t *fsFileInitialize( dir_t *parent, dentry_t *dentry, char *name, unsigned int flags, unsigned short type, unsigned int name_size, fcb_t *fcb )
{
	file_t *file = (file_t *)malloc(sizeof(file_t)+name_size+1);

	if ( file == NULL ) {
		errorMessage("fsFileInitialize: could not alloc file");
		return NULL;
	}

	/* construct file object */
	file->flags = flags;
	file->owner.uid = ( fcb ? fcb->uid : user.uid ); /* set to current user at create time */
	file->type = type;
	file->size = ( fcb ? fcb->size : 0 );  /* fcb is only null at create time */
	file->diskfile = fcb;
	file->ct = 0;
	file->parent = parent;
	file->attr_block = BLK_INVALID;
	memset( file->blocks, BLK_INVALID, FILE_BLOCKS * sizeof(unsigned int) );
	memcpy( file->name, name, name_size );
	file->name[name_size] = 0; // null terminate
	file->name_size = name_size;

	/* associate dentry with file */
	dentry->type = DTYPE_FILE;
	dentry->file = file;

	return file;
}


/**********************************************************************

    Function    : fsAddDentry
    Description : Add the dentry to its directory
    Inputs      : dir - directory object
                  dentry - dentry object
    Outputs     : 0 if success, -1 if error 

***********************************************************************/

int fsAddDentry( dir_t *dir, dentry_t *dentry )
{
	int key;
	char canonical_name[MAX_NAME_SIZE];

	if ( !diskCanonicalizeName( dir->diskdir, dentry->name, dentry->name_size, canonical_name, MAX_NAME_SIZE ) ) {
		errorMessage("fsAddDentry: cannot canonicalize name");
		return -1;
	}

	key = fsMakeKey( canonical_name, dir->bucket_size, dentry->name_size );

	if ( key >= dir->bucket_size ) {
		errorMessage("fsAddDentry: problem with the bucket count");
		return -1;
	}

	/* add to in-memory directory representation -- not same as disk list */
	addToList( &dir->buckets[key], dentry );

	return 0;
}


/**********************************************************************

    Function    : fsAddFile
    Description : Add the file to the system-wide open file cache
    Inputs      : filetable - system-wide file table 
                  file - file to be added
    Outputs     : an index, or -1 on error 

***********************************************************************/

int fsAddFile( file_t **filetable, file_t *file) 
{
	int i;

	for ( i = 0; i < FS_FILETABLE_SIZE; i++ ) {
		if ( filetable[i] == NULL ) {
			filetable[i] = file;
			file->ct++;
			// printf("ADD[%d]: parent=%p name=%s\n", i, file->parent, file->name);
			return i;
		}
	}
  
	errorMessage("fsAddFile: system-wide file table is full");
	return -1;
}


/**********************************************************************

    Function    : fsAddProcFile
    Description : Add the file to the per-process open file cache
    Inputs      : proc - process
                  file - file to be added
    Outputs     : a file descriptor, or -1 on error 

***********************************************************************/

int fsAddProcFile( proc_t *proc, file_t *file) 
{
	int i;
	fstat_t *fstat;

	/* make the fstat structure for the per-process table */
	fstat = (fstat_t *)malloc(sizeof(fstat));
	fstat->file = file;
	fstat->offset = 0;

	/* add to the per-process file table */
	for ( i = 0; i < PROC_FILETABLE_SIZE; i++ ) {
		if ( proc->fstat_table[i] == NULL ) {
			proc->fstat_table[i] = fstat;
			/* return the index (file descriptor) */
			return i;
		}
	}

	errorMessage("fsAddProcFile: per-process file table is full");
	free(fstat);
	return -1;
}


/**********************************************************************

    Function    : fsUserHasPerm
    Description : Can the user open the file?
    Inputs      : file - for which file?
                  user - against which user
    Outputs     : TRUE or FALSE

***********************************************************************/

int fsUserHasPerm( file_t *file, user_t *user )
{
	if ( file->owner.uid == user->uid )
		return TRUE;

	return FALSE;
}


/**********************************************************************

    Function    : fsResolveName
    Description : Resolve a standard UNIX path name based on the
                  provided constraint.

***********************************************************************/

path_t *fsResolveName( char *name, unsigned int constrain )
{
	int name_size;
	path_t *path;
	dir_t *dir;
	dentry_t *dentry = NULL;

	/* Tasks: 3a, 3b, 3c, 4a, 4b
	 * -------------------------
	 *
	 * For all the above functions, add code to this function.
	 *
	 * For 3a, you may optionally want to implement the functions
	 * fsGetLinkTarget(...) and pathMergeWithSymlink(...).
	 */


	/* convert UNIX path to path_t */
	path = pathParse( name );
	if ( !path ) {
		fserror( E_Resolve, NULL ); /* cannot parse path name */
		return NULL;
	}

	/* get the in-memory representation of our directory (perhaps on disk) */
	dir = fsParentDir( path, constrain );
	if ( dir == NULL ) {
		path->err = E_Resolve; /* cannot retrieve parent dir */
		goto error;
	}

	/* get the dentry name to resolve */
	name = pathNextName( path );
	if ( !name )
		goto end;
	name_size = strlen( name );

	/* retrieve in-memory dentry for this name */
	dentry = fsFindDentry( dir, name, name_size, constrain );

end:
	path->dir = dir;
	path->dentry = dentry;
	return path;

error:
	pathError( path );
	pathFree( path );
	return NULL;
}


/**********************************************************************

    Function    : dirCreate
    Description : Create directory entry and dir object
    Inputs      : name - name of the dir 
                  flags - creation options
                  constrain - flags to control resolution
    Outputs     : 0 on success or -1 if error

***********************************************************************/

int dirCreate( char *name, unsigned int flags, unsigned int constrain ) {
	int rtn = -1;
	unsigned int name_size;
	path_t *path;
	dir_t *newdir;
	dentry_t *newdentry;

	/* resolve the path */
	path = fsResolveName( name, constrain );
	if ( !path )
		return -1;

	/* get the dentry name */
	name = pathNextName( path );
	if ( !name )
		goto error;
	name_size = strlen( name );

	/* Again retrieve the in-memory dentry for this name because
	   we need to ignore the saved name constraint to find existing entries. */
	path->dentry = fsFindDentry( path->dir, name, name_size, (constrain & ~FLAG_SAVEDNAME) );

	if ( path->dentry ) {
		path->err = E_Exists; /* already exists */
		goto error;
	}

	/* =======  now build the dir */
	/* build a blank in-memory directory entry for the new dir */
	newdentry = fsDentryInitialize( name, (ddentry_t *)NULL, name_size, DTYPE_DIR );

	/* add in-memory dentry to in-memory directory */
	fsAddDentry( path->dir, newdentry );

	/* create dir in memory */
	newdir = fsDirInitialize( newdentry, flags, (ddir_t *)NULL );

	/* add dentry to disk */
	diskCreateDentry( (unsigned long long)fs->base, path->dir, newdentry );

	/* add dir to disk */
	rtn = diskCreateDir( (unsigned long long)fs->base, newdentry, newdir );

	pathFree( path );
	return rtn;

error:
	pathError( path );
	pathFree( path );
	return rtn;
}


/**********************************************************************

    Function    : fileCreateHelper
    Description : Create directory entry and file object using
                  path_t. Also called from fileOpen() when using
                  FLAG_CREAT.
    Inputs      : path - to create
                  flags - creation options
                  constrain - flags to control resolution
    Outputs     : new file descriptor or -1 if error

***********************************************************************/

int fileCreateHelper( path_t *path, unsigned int flags, unsigned int constrain )
{
	int index, fd;
	int rtn;
	unsigned int name_size;
	char *name;
	file_t *file;
	dentry_t *newdentry;

	/* get the dentry name */
	name = pathNextName( path );
	if ( !name )
		goto error;
	name_size = strlen( name );

	/* =======  verify file does not already exist -- first in the filetable */
	file = fsCacheFindFile( fs->filetable, name, path->dir, flags, FTYPE_REGULAR, name_size );
	if ( file ) {
		path->err = E_AlreadyOpen; /* file already exists in file table */
		goto error;
	}

	/* Again retrieve the in-memory dentry for this name because
	   we need to ignore the saved name constraint to find existing entries. */
	path->dentry = fsFindDentry( path->dir, name, name_size, (constrain & ~FLAG_SAVEDNAME) );

	if ( path->dentry ) {
		path->err = E_Exists; /* already exists on disk */
		goto error;
	}

	/* =======  now build the file */
	/* build a blank in-memory directory entry for the new file */
	newdentry = fsDentryInitialize( name, (ddentry_t *)NULL, name_size, DTYPE_FILE );

	/* add in-memory dentry to in-memory directory */
	fsAddDentry( path->dir, newdentry );

	/* create file in memory */
	file = fsFileInitialize( path->dir, newdentry, name, flags, FTYPE_REGULAR, name_size, (fcb_t *)NULL );

	/* add dentry to disk */
	diskCreateDentry( (unsigned long long)fs->base, path->dir, newdentry );

	/* add file to disk */
	rtn = diskCreateFile( (unsigned long long)fs->base, newdentry, file );
	if ( rtn ) {
		errorMessage("fileCreate: fail diskCreateFile()");
		goto error;
	}

	/* add file in system-wide file table */
	index = fsAddFile( fs->filetable, file );
	if ( index < 0 )
		goto error;

	/* add file to per-process file table */
	fd = fsAddProcFile( fs->proc, file );

	return fd;

error:
	rtn = -path->err;
	pathError( path );
	return rtn;
}

/**********************************************************************

    Function    : fileCreate
    Description : Create directory entry and file object
    Inputs      : name - name of the file 
                  flags - creation options
                  constrain - flags to control resolution
    Outputs     : new file descriptor or -1 if error

***********************************************************************/

/* Deprecate in lieu of open() with FLAG_CREAT */
int fileCreate( char *name, unsigned int flags, unsigned int constrain )
{
	path_t *path;
	int fd;
	
	/* resolve the path */
	path = fsResolveName( name, constrain );
	if ( !path )
		return -1;

	/* create the file */
	fd = fileCreateHelper( path, flags, constrain );

	pathFree( path );
	return fd;
}


/**********************************************************************

    Function    : fileLink
    Description : Create a symbolic link to a file
    Inputs      : target - name of the target file
                  name - name of the file 
                  constrain - flags to control resolution
    Outputs     : 0 on success or -1 if error

***********************************************************************/

int fileLink( char *target, char *name, unsigned int constrain )
{
	/* only follow directory symlinks */
	constrain |= FLAG_NOFOLLOW;

	/* Task 1b: Create a symbolic link */


	return -1;
}


/**********************************************************************

    Function    : fsGetLinkTarget
    Description : Get the target for a symbolic link file
    Inputs      : file - in-memory pointer to the file
                  target - buffer to write the symbolic link's target
                  target_len - size of target buffer in bytes
    Outputs     : 0 on success or -1 if error

***********************************************************************/

int fsGetLinkTarget( file_t *file, char *target, unsigned int target_len )
{
	/* Task 3a (optional) */

	return 0;
}


/**********************************************************************

    Function    : fileOpen
    Description : Open directory entry of specified name
    Inputs      : name - name of the file 
                  flags - creation options (or mode)
                  constrain - flags to control resolution
    Outputs     : new file descriptor or -1 if error

***********************************************************************/

int fileOpen( char *name, unsigned int flags, unsigned int constrain )
{
	int index, fd;
	file_t *file;
	int rtn = -1;
	unsigned int name_size;
	path_t *path;
	int flag_create         = constrain & FLAG_CREAT;
	int flag_fail_if_exists = constrain & FLAG_EXCL;

	/* resolve the path */
	// TODO: Only follow owner's links based on flags
	path = fsResolveName( name, constrain );
	if ( !path )
		return -1;

	/* get the dentry name */
	name = pathNextName( path );
	if ( !name )
		goto error;
	name_size = strlen( name );


	/* search for file in the system-wide file table */
	file = fsCacheFindFile( fs->filetable, name, path->dir, flags, FTYPE_REGULAR, name_size );
	if ( file ) {
		path->err = E_AlreadyOpen;
		goto error;
	}

	/* create file if it does not exist */
	if ( !path->dentry ) {
		if ( flag_create ) {
			fd = fileCreateHelper( path, flags, constrain );
			pathFree( path );
			return fd;
		} else {
			path->err = E_DoesNoExist; /* no such file */
			goto error;
		}
	}

	/* get the file from disk */
	file = fsFindFile( path->dir, path->dentry, name, flags, FTYPE_REGULAR, name_size );
	if ( !file ) {
		path->err = E_Open; /* fail due to bad flags or opening a symlink */
		goto error;
	}

	if ( flag_fail_if_exists ) {
		path->err = E_OpenEXCL; /* not allowed to open file due to FLAG_EXCL */
		goto error;
	}

	if ( !fsUserHasPerm( file, &user ) ) {
		path->err = E_NoPerm; /* bad user */
		goto error;
	}

	/* add file in system-wide file table */
	index = fsAddFile( fs->filetable, file );
	if ( index < 0 )
		goto error;

	/* add file to per-process file table */
	fd = fsAddProcFile( fs->proc, file );

	pathFree( path );
	return fd;

error:
	pathError( path );
	pathFree( path );
	return rtn;
}


char *filetype(ddentry_t *disk_dentry) {
	fcb_t *fcb;

	switch(disk_dentry->type) {
	case DTYPE_DIR:
		return "Dir";

	case DTYPE_FILE:
		fcb = diskFindFile(disk_dentry);
		if (fcb->type == FTYPE_SYMLINK)
			return "Link";
		else if (fcb->type == FTYPE_REGULAR)
			return "File";
	}

	return "Unknown";
}


/**********************************************************************

    Function    : listDentryInfo
    Description : get entry info. about the dentry
    Inputs      : disk_dentry - whose info. should be populated
                  listinfo - populate structure based on disk_dentry
    Outputs     : none

***********************************************************************/

void listDentryInfo( ddentry_t *disk_dentry, listinfo_t *info )
{
	int len = 0;
	int size;
	fcb_t *fcb;
	ddir_t *ddir;

	if ( !info ) {
		errorMessage("listDentryInfo: listinfo_t is NULL");
		return;
	}
	size = sizeof(info->describe);
	info->suffix = "";
	info->ncolors = 0;

	switch ( disk_dentry->type ) {
	case DTYPE_FILE:
		fcb = diskFindFile( disk_dentry );
		len += snprintf(info->describe, size, " uid=%d mode=%d", fcb->uid, fcb->flags);

		/* populate listinfo_t */
		switch ( fcb->type ) {
		case FTYPE_REGULAR:
			info->suffix = "";
			info->ncolors = 0;
			break;

		case FTYPE_SYMLINK:
			info->suffix = LINKCOLOR("*");
			info->ncolors = strlen(LINKCOLOR(""));
			break;

		default: /* unknown file type */
			len += snprintf(info->describe+len, size, " type=%d", fcb->type);
			info->suffix = LINKCOLOR("(?)");
			info->ncolors = strlen(LINKCOLOR(""));
			break;
		}
		break;;

	case DTYPE_DIR:
		ddir = diskFindDir( disk_dentry );
		len += snprintf(info->describe, size, " flags=0x%x", ddir->flags);

		info->suffix = "/";
		info->ncolors = 0;
		break;

	default:
		/* unknown dentry type */
		len += snprintf(info->describe, size, "%s", "??");
		info->suffix = LINKCOLOR("??");
		info->ncolors = strlen(LINKCOLOR(""));
	}

	/* ensure NULL termination */
	if ( size > 0 )
		info->describe[size-1] = '\0';
}


/**********************************************************************

    Function    : listDirectoryat
    Description : print the files in the root directory currently
    Inputs      : level - level to print at
                  diskdir - on-disk dir to list
    Outputs     : none; recursively print directory

***********************************************************************/

void listDirectoryat( int level, ddir_t *diskdir )
{
	ddh_t *ddh;
	int i, n;
	listinfo_t info;

	/* list the names of all the files reachable from this directory */
	/* more appropriate file: disk.c */
	ddh = (ddh_t *)&diskdir->data[0];
	for ( i = 0; i < diskdir->buckets; i++ ) {
		ddh_t *thisddh = (ddh+i);   // ****
		while ( thisddh->next_dentry != BLK_SHORT_INVALID ) {
			dblock_t *dblk = (dblock_t *)disk2addr( fs->base, (block2offset( thisddh->next_dentry )));
			ddentry_t *disk_dentry = (ddentry_t *)disk2addr( dblk, dentry2offset( thisddh->next_slot ));

			/* populate info on dentry */			
			listDentryInfo( disk_dentry, &info );

			/* print entry */
			n = printf("%*s %s%s", level * 4, "|_", disk_dentry->name, info.suffix);
			n -= info.ncolors;
			printf("%*s\n", MAX_LINE_WIDTH-n, info.describe);

			/* Recurse */
			if ( disk_dentry->type == DTYPE_DIR )
				listDirectoryat( level + 1  , diskFindDir(disk_dentry) );

			thisddh = &disk_dentry->next;
		}
	}
}


/**********************************************************************

    Function    : listDirectory
    Description : print the files in the root directory currently
    Inputs      : none
    Outputs     : none

***********************************************************************/

void listDirectory( void )
{
	listDirectoryat(0, fsRootDir()->diskdir);
}


/**********************************************************************

    Function    : fileClose
    Description : close the file associated with the file descriptor
    Inputs      : fd - file descriptor
    Outputs     : none

***********************************************************************/

void fileClose( unsigned int fd )
{
	fstat_t *fstat;
	file_t *file;
	int i;

	/* get the file in per-process file structure */
	fstat = fs->proc->fstat_table[fd];

	if ( fstat == NULL ) {
		fserror( E_NoSuchFD, NULL ); /* no file corresponds to fd */
		return;
	}

	file = fstat->file;
  
	/* reduce reference count */
	file->ct--;
  
	/* if ref count is 0, then remove file from system-wide table */
	if ( file->ct == 0 ) {
		/* note: could save the index in the filetable */
		for ( i = 0; i < FS_FILETABLE_SIZE; i++ ) {
			if ( file == fs->filetable[i] ) {
				/* remove entry from the filetable */
				fs->filetable[i] = (file_t *)NULL;
				// printf("DEL[%d]: parent=%p name=%s\n", i, file->parent, file->name);

				/* for this, we need to set: dentry->file = NULL
				 * otherwise re-opening and writing to a file will
				 * result in use-after-free bug.
				 *
				 * See: fsFindFile()
				 */
				// free( file );
				break;
			}
		}
	}

	/* free mark entry as free */
	fs->proc->fstat_table[fd] = NULL;
}


/**********************************************************************

    Function    : fileRead
    Description : Read specified number of bytes from the current file index
    Inputs      : fd - file descriptor
                  buf - buffer for placing data
                  bytes - number of bytes to read
    Outputs     : number of bytes read

***********************************************************************/

int fileRead( unsigned int fd, char *buf, unsigned int bytes )
{
	fstat_t *fstat = fs->proc->fstat_table[fd];
	file_t *file;
	int total = 0;

	if ( fstat == NULL ) {
		errorMessage("fileRead: No file corresponds to fd");
		return -1;
	}

	file = fstat->file;

	if ( file == NULL ) {
		errorMessage("fileRead: No file corresponds to fstat");
		return -1;
	}

	/* read limit is either size of buffer or distance to end of file */
	bytes = min( bytes, ( file->size - fstat->offset ));

	/* read the file from the offset */
	while ( total < bytes ) {   /* more to write */
		int index = fstat->offset / ( FS_BLOCKSIZE - sizeof(dblock_t) );
		unsigned int block = file->blocks[index];
		int block_bytes;

		/* if block has not been brought into memory, copy it */
		if ( block == BLK_INVALID ) {
			block = diskGetBlock( file, index );
			file->blocks[index] = block;
      
			if ( block == BLK_INVALID ) {
				errorMessage("fileRead: Could get block from the disk");
				return -1;
			}
		}

		if ( index >= FILE_BLOCKS ) {
			errorMessage("fileRead: Max size of file reached");
			return total;
		}

		/* read this block */
		block_bytes = diskRead( block, buf, bytes, 
					fstat->offset, total );

		/* update the total written and the file offset as well */
		total += block_bytes; 
		fstat->offset += block_bytes;
		buf += block_bytes;
	}
  
	return total;
}


/**********************************************************************

    Function    : fileWrite
    Description : Write specified number of bytes starting at the current file index
    Inputs      : fd - file descriptor
                  buf - buffer to write
                  bytes - number of bytes to write
    Outputs     : number of bytes written

***********************************************************************/

int fileWrite( unsigned int fd, char *buf, unsigned int bytes )
{
	fstat_t *fstat = fs->proc->fstat_table[fd];
	file_t *file;
	unsigned int total = 0;

	if ( fstat == NULL ) {
		errorMessage("fileWrite: No file corresponds to fd");
		return -1;
	}

	/* get file structure */
	file = fstat->file;

	if ( file == NULL ) {
		errorMessage("fileWrite: No file corresponds to fstat");
		return -1;
	}

	/* write to the file */
	while ( total < bytes ) {   /* more to write */
		int index = fstat->offset / ( FS_BLOCKSIZE - sizeof(dblock_t) );
		unsigned int block = file->blocks[index];
		unsigned int block_bytes;

		/* if block has not been brought into memory, copy it */
		if ( block == BLK_INVALID ) {
			block = diskGetBlock( file, index );
			file->blocks[index] = block;
      
			if ( block == BLK_INVALID ) {
				errorMessage("fileWrite: Could get block from the disk");
				return -1;
			}
		}

		if ( index >= FILE_BLOCKS ) {
			errorMessage("fileWrite: Max size of file reached");
			return total;
		}

		/* write to this block */
		block_bytes = diskWrite( &(file->diskfile->size), block, buf, bytes, 
					 fstat->offset, total );

		/* update the total written and the file offset as well */
		total += block_bytes; 
		fstat->offset += block_bytes;
		buf += block_bytes;
	}

	/* update the file's size (if necessary) */
	if ( fstat->offset > file->size ) {
		file->size = fstat->offset;
	}

	return total;
}


/**********************************************************************

    Function    : fileSeek
    Description : Adjust offset in per-process file entry
    Inputs      : fd - file descriptor
                  index - new offset 
    Outputs     : 0 on success, -1 on failure

***********************************************************************/

int fileSeek( unsigned int fd, unsigned int index )
{
	fstat_t *fstat = fs->proc->fstat_table[fd];
	file_t *file;

	if ( fstat == NULL ) {
		errorMessage("fileSeek: No file corresponds to fd");
		return -1;
	}

	file = fstat->file;

	if ( file == NULL ) {
		errorMessage("fileSeek: No file corresponds to fstat");
		return -1;
	}

	if ( index <= file->size ) {
		fstat->offset = index;
	}

	return 0;
}


/**********************************************************************

    Function    : switchUser
    Description : Adjust offset in per-process file entry
    Inputs      : uid - new user uid
    Outputs     : 0 on success, -1 on failure

***********************************************************************/

int switchUser( unsigned int new_uid )
{
	int i;

	if ( user.uid == new_uid )
		return 0; /* already same user */

	/* search for open files in system-wise file table */
	for ( i = 0; i < FS_FILETABLE_SIZE; i++ ) {
		file_t *file = fs->filetable[i];
		if( file ) {
			errorMessage("fileSwitchUser: file table not empty");
			return -1;
		}
	}

	user.uid = new_uid;
	return 0;
}

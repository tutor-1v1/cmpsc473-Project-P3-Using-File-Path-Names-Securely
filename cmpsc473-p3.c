https://tutorcs.com
WeChat: cstutorcs
QQ: 749389476
Email: tutorcs@163.com

/**********************************************************************

   File          : cmpsc473-p4.c

   Description   : This is the main file for the file system project
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
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Project Include Files */
#include "cmpsc473-filesys.h"
#include "cmpsc473-disk.h"
#include "cmpsc473-list.h"
#include "cmpsc473-util.h"

/* Definitions */
#define CLIENT_USAGE "USAGE: cmpsc473-p3 <fs_file> <cmd_file1> <cmd_file2> ...\n"
#define MAX_ARGS 5
#define MAX_LINE_SIZE 750

/* macros */

/* program variables */
char cmd_delims[] = {' ', '('};
char arg_delims[] = {","};


/* Functions */
int(*mkdirfn)( char *name, unsigned int flags, unsigned int constrain ) = dirCreate;
int(*createfn)( char *name, unsigned int flags, unsigned int constrain ) = fileCreate;
int(*linkfn)( char *target, char *name, unsigned int constrain ) = fileLink;
int(*openfn)( char *name, unsigned int flags, unsigned int constrain ) = fileOpen;
void(*listfn)( void ) = listDirectory;
void(*closefn)( unsigned int fd ) = fileClose;
int(*readfn)( unsigned int fd, char *buf, unsigned int bytes ) = fileRead;
int(*writefn)( unsigned int fd, char *buf, unsigned int bytes ) = fileWrite;
int(*seekfn)( unsigned int fd, unsigned int index ) = fileSeek;
int(*switchuserfn)( unsigned int new_uid ) = switchUser;


/* Global variables */
filesys_t *fs;
dfilesys_t *dfs;
user_t user;


/**********************************************************************

    Function    : processInitialize
    Description : initialize this process structure
    Inputs      : none
    Outputs     : 0 if successful, other represents error or termination

***********************************************************************/

int processInitialize( void ) 
{
	/* create the process */
	fs->proc = (proc_t *)malloc(sizeof(proc_t));

	if ( fs->proc == NULL ) {
		errorMessage("processInitialize: process allocation failed");
		return -1;
	}
   
	/* create the process's per-process file table */
	fs->proc->fstat_table = (fstat_t **)malloc(PROC_FILETABLE_SIZE * sizeof(fstat_t *));

	if ( fs->proc->fstat_table == NULL ) {
		errorMessage("processInitialize: fstat_table allocation failed");
		return -1;
	}

	/* setup user */
	user.uid = 0;

	return 0;
}


/**********************************************************************

    Function    : fsInitialize
    Description : Initialize the file system provided via the file 
    Inputs      : filesys - in-memory file system info
                  fsfile - file containing the state of the file system
    Outputs     : none

***********************************************************************/

void fsInitialize( filesys_t *fs, char *fsfile ) 
{
	int creating = 0;
	int fssize;

	/* set the commands */
	fs->mkdir = mkdirfn;
	fs->create = createfn;
	fs->link = linkfn;
	fs->open = openfn;
	fs->list = listfn;
	fs->close = closefn;
	fs->read = readfn;
	fs->write = writefn;
	fs->seek = seekfn;
	fs->switchuser = switchuserfn;

	/* open the fsfile -- contains the file system */
	if (( fs->fd = open( fsfile, O_CREAT | O_RDWR, S_IRWXU )) < 0 ) {
		errorMessage("fsInitialize: cannot open <fsfile>");
		exit( -1 );
	}

	/* get the file size */
	if ( fstat( fs->fd, &fs->sb ) < 0 ) {
		close( fs->fd );
		errorMessage("fsInitialize: cannot stat <fsfile>");
		exit( -1 );
	}

	fssize = fs->sb.st_size;

	/* Initialize the file system information */
	if ( fssize == 0 ) {   /* a new fsfile */

		/* set that we are creating a new file system */
		creating = 1;

		/* compute the size of the file system on disk */
		/* file system struct + (no of blocks * (size of block metadata + size of block data)) */
		fssize = FS_BLOCKS * FS_BLOCKSIZE;

		if ( ftruncate( fs->fd, fssize ) < 0 ) {
			close( fs->fd );
			errorMessage("fsInitialize: cannot ftruncate for a new <fsfile>");
			exit( -1 );
		}
	}

	/* mmap the fsfile */
	if (( fs->base = dfs = (dfilesys_t *)mmap( 0, fssize, PROT_READ | PROT_WRITE, 
						   MAP_SHARED, fs->fd, 0 )) == (void *)-1 ) {
		close( fs->fd );
		errorMessage("fsInitialize: cannot mmap <fsfile>");
		exit( -1 );
	}

	if ( creating ) {    /* create the filesystem */
		dblock_t *blk, *prev = (dblock_t *) NULL; 
		int i;

		dfs->bsize = FS_BLOCKS;                  /* number of blocks total */
		dfs->root = FS_METADATA_BLOCKS;          /* root fs */

		/* make the root directory -- first (only) on-disk dentry block */
		if ( diskDirInitialize( (ddir_t *)block2addr( fs->base, dfs->root )) < 0 ) {
			errorMessage("fsInitialize: fail in disk directory initialization");
			exit( -1 );
		}

		dfs->firstfree = FS_METADATA_BLOCKS+     /* offset to first free data block */
			FS_DIRINIT_BLOCKS;      

		/* initialize the other blocks on disk as free blocks -- link together */
		for ( i = dfs->firstfree; i < FS_BLOCKS; i++ ) {      /* start at first free block */
			blk = (dblock_t *)block2addr( fs->base, i );
			blk->free = FREE_BLOCK;
			blk->st.data_end = 0;
			if ( prev != NULL ) {
				prev->next = (unsigned int)addr2block( blk, fs->base );
			}
			prev = blk;
		}
	}

	/* in-memory file system initialization - file table and block cache */
	fs->filetable = (file_t **)malloc( FS_FILETABLE_SIZE * sizeof(file_t *));
	memset( fs->filetable, 0, FS_FILETABLE_SIZE * sizeof(file_t *));
	fs->block_cache = (block_t **)malloc( FS_BCACHE_BLOCKS * FS_BLOCKSIZE );
	memset( fs->block_cache, 0, FS_BCACHE_BLOCKS * FS_BLOCKSIZE );
}


/**********************************************************************

    Function    : dir_flags_to_str
    Description : Get string representation of a directory's flags 
    Inputs      : flags - all flags
                  buffer - array to populate
                  buffer_size - size of array
    Outputs     : none

***********************************************************************/

void dir_flags_to_str( unsigned int flags, char *buffer, int buffer_size ) {
	int has_witten = FALSE;
	int len = 0;
	char *buf = buffer;

	/* populate flags */
	if ( flags & D_ICASE ) {
		len += snprintf( buf+len, buffer_size-len, "D_ICASE | " );
		has_witten = TRUE;
	}

	/* no flags */
	if ( !has_witten )
		len += snprintf( buf+len, buffer_size-len, "NONE" );
	else
		buf[ len - 3 ] = '\0'; // remove trailing " | "
	
	/* ensure NULL termination */
	if ( buffer_size > 0 )
		buffer[ buffer_size-1 ] = '\0';
}


/**********************************************************************

    Function    : constrain_to_str
    Description : Get string representation of the given constraints 
    Inputs      : constrain - all flags
                  buffer - array to populate
                  buffer_size - size of array
    Outputs     : none

***********************************************************************/

void constrain_to_str( unsigned int constrain, char *buffer, int buffer_size ) {
	int has_witten = FALSE;
	int len = 0;
	char *buf = buffer;

	/* populate constrains */
	if ( constrain & FLAG_CREAT ) {
		len += snprintf( buf+len, buffer_size-len, "FLAG_CREAT | " );
		has_witten = TRUE;
	}

	if ( constrain & FLAG_EXCL ) {
		len += snprintf( buf+len, buffer_size-len, "FLAG_EXCL | " );
		has_witten = TRUE;
	}

	if ( constrain & FLAG_NOFOLLOW ) {
		len += snprintf( buf+len, buffer_size-len, "FLAG_NOFOLLOW | " );
		has_witten = TRUE;
	}

	if ( constrain & FLAG_NOFOLLOW_ANY ) {
		len += snprintf( buf+len, buffer_size-len, "FLAG_NOFOLLOW_ANY | " );
		has_witten = TRUE;
	}

	if ( constrain & FLAG_NOFOLLOW_UNTRUSTED ) {
		len += snprintf( buf+len, buffer_size-len, "FLAG_NOFOLLOW_UNTRUSTED | " );
		has_witten = TRUE;
	}

	if ( constrain & FLAG_SAVEDNAME ) {
		len += snprintf( buf+len, buffer_size-len, "FLAG_SAVEDNAME | " );
		has_witten = TRUE;
	}

	/* no flags */
	if ( !has_witten )
		len += snprintf( buf+len, buffer_size-len, "NONE" );
	else
		buf[ len - 3 ] = '\0'; // remove trailing " | "
	
	/* ensure NULL termination */
	if ( buffer_size > 0 )
		buffer[ buffer_size-1 ] = '\0';
}


/**********************************************************************

    Function    : processCmd
    Description : run a file system command
    Inputs      : fs - in-memory file system
                  cmdline - command line
    Outputs     : 0 if successful, other represents error or termination

***********************************************************************/

int processCmd( filesys_t *fs, char *cmdline ) 
{
	char *cmd, *arg[MAX_ARGS];
	char strbuffer[MAX_LINE_SIZE];
	int i = 0, j, rtn;
	char *buf;

	/* parse command and args */
	cmd = strtok( cmdline, cmd_delims );
	while (( cmd != NULL ) && ((arg[i++] = strtok( NULL, arg_delims )) != NULL ));

	/* remove newline & whitespace at beginning of cmd */
	while (cmd != NULL && (*cmd == '\n' || *cmd == '\r' || *cmd == ' '))
		cmd++;

	/* ignore empty lines */
	if (strlen(cmd) == 0)
		return 0;
  
	/* remove whitespace at beginning of each arg */
	for ( j = 0; j < (i-1); j++ ) {
		int k = 0;
		while ( arg[j][k] == ' ' ) k++;
		arg[j] += k;
	}

	/* run commands */
	if (( strncmp( cmd, "creat", min( strlen(cmd), 5 )) == 0 ) && ( arg[3] == NULL )) {
		unsigned int mode = atoi( arg[1] );
		unsigned int constrain = strtol( arg[2], NULL, 16 );
		constrain_to_str( constrain, strbuffer, MAX_LINE_SIZE );

		printf("%s: name = %s; mode = %d; constrain = (%s)\n", CMDCOLOR("creat"), arg[0], mode, strbuffer);
		rtn = fs->create( arg[0], mode, constrain );
		printf("ret: fd=%d\n", rtn);
	}
	else if (( strncmp( cmd, "mkdir", min( strlen(cmd), 5 )) == 0 ) && ( arg[3] == NULL )) {
		char strbuffer2[MAX_LINE_SIZE];
		unsigned int flags = strtol( arg[1], NULL, 16 );
		unsigned int constrain = strtol( arg[2], NULL, 16 );
		dir_flags_to_str( flags, strbuffer, MAX_LINE_SIZE );
		constrain_to_str( constrain, strbuffer2, MAX_LINE_SIZE );

		printf("%s: name = %s; flags = (%s); constrain = (%s)\n", CMDCOLOR("mkdir"), arg[0], strbuffer, strbuffer2 );
		fs->mkdir( arg[0], flags, constrain );
	}
	else if (( strncmp( cmd, "link", min( strlen(cmd), 4 )) == 0 ) && ( arg[3] == NULL )) {
		unsigned int constrain = strtol( arg[2], NULL, 16 );
		constrain_to_str( constrain, strbuffer, MAX_LINE_SIZE );

		printf("%s: target = %s; name = %s; constrain = (%s)\n", CMDCOLOR("link"), arg[0], arg[1], strbuffer );
		fs->link( arg[0], arg[1], constrain);
	}
	else if (( strncmp( cmd, "open", min( strlen(cmd), 4 )) == 0 ) && ( arg[3] == NULL )) {
		unsigned int flags = atoi( arg[1] );
		unsigned int constrain = strtol( arg[2], NULL, 16 );
		constrain_to_str( constrain, strbuffer, MAX_LINE_SIZE );

		printf("%s: name = %s; flags = %d; constrain = (%s)\n", CMDCOLOR("open"), arg[0], flags, strbuffer );
		rtn = fs->open( arg[0], flags, constrain );
		printf("ret: fd=%d\n", rtn);
	}
	else if (( strncmp( cmd, "list", min( strlen(cmd), 4 )) == 0 ) && ( arg[0] == NULL )) {
		printf("%s: void\n", CMDCOLOR("list"));
		fs->list( );
	}
	else if (( strncmp( cmd, "close", min( strlen(cmd), 5 )) == 0 ) && ( arg[1] == NULL )) {
		printf("%s: %d\n", CMDCOLOR("close"), atoi( arg[0] ));
		fs->close( atoi( arg[0] ));
	}
	else if (( strncmp( cmd, "read", min( strlen(cmd), 4 )) == 0 ) && ( arg[3] == NULL )) {
		buf = (char *)malloc(atoi( arg[2] ) +1);
		memset( buf, 0, atoi( arg[2] ) +1);
		printf("%s: fd = %d; buffer = %s; size = %d\n", CMDCOLOR("read"), atoi( arg[0] ), 
		       arg[1], atoi( arg[2] ));
		fs->read( atoi( arg[0] ), buf, atoi( arg[2] ));
		buf[ atoi( arg[2] ) ] = 0;
		printf("\n ======= read: %d; %d bytes ==========\n", atoi( arg[0] ), 
		       atoi( arg[2] ));
		printf("%s\n", buf);
		free( buf );
	}
	else if (( strncmp( cmd, "write", min( strlen(cmd), 5 )) == 0 ) && ( arg[3] == NULL )) {
		struct stat sb;
		int file_size, fd;

		printf("%s: fd = %d; buf file = %s; unused = %d\n", CMDCOLOR("write"), atoi( arg[0] ), 
		       arg[1], atoi( arg[2] ));
		if (( fd = open( arg[1], O_RDONLY )) < 0 ) {
			errorMessage("write command: cannot open <file>");
			return -1;
		}

		if ( fstat( fd, &sb ) < 0 ) {
			close( fd );
			errorMessage("write command: cannot stat <file>");
			return -1;
		}
    
		file_size = sb.st_size;
		buf = (char *) malloc(file_size);

		read( fd, buf, file_size );

		fs->write( atoi( arg[0] ), buf, file_size );
	}
	else if (( strncmp( cmd, "seek", min( strlen(cmd), 4 )) == 0 ) && ( arg[2] == NULL )) {
		printf("%s: fd = %d; size = %d\n", CMDCOLOR("seek"), atoi( arg[0] ), 
		       atoi( arg[1] ));
		fs->seek( atoi( arg[0] ), atoi( arg[1] ));
	}
	else if (( strncmp( cmd, "print", min( strlen(cmd), 5 )) == 0 ) && ( arg[1] == NULL )) {
		buf = (char *)malloc( 10000 );
		printf("%s: fd = %d", CMDCOLOR("print"), atoi( arg[0] ));
		fs->seek( atoi( arg[0] ), 0);
		fs->read ( atoi( arg[0] ), buf, 10000);
		printf("\n ======= file contents: %d ==========\n", atoi( arg[0] ));
		printf("%s\n", buf);
		free( buf );
	}
	else if (( strncmp( cmd, "user", min( strlen(cmd), 4 )) == 0 ) && ( arg[1] == NULL )) {
		int new_uid = atoi( arg[0] );
		printf("%s: uid = %d;\n", CMDCOLOR("user"), new_uid);
		fs->switchuser( new_uid );
	}
	else if ((strlen(cmd) >= 2) && (cmd[0] == '/') && (cmd[1] == '/')) {
		/* ignore comment */
	}
	/* remove file */
	else {
		printf(ERRCOLOR("no match\n"));
	}

	return( 0 );
}


/**********************************************************************

    Function    : runSuite
    Description : run the file system commands 
    Inputs      : fs - in-memory file system
                  cmdfile - file system command file
    Outputs     : none

***********************************************************************/

int runSuite( filesys_t *fs, char *cmdfile ) 
{
	int fd, rc;
	char cmdline[MAX_LINE_SIZE];

	/* open the command file */
	if (( fd = open( cmdfile, O_RDONLY )) < 0 ) {
		errorMessage("runSuite: cannot open <cmdfile>");
		return -1;
	}

	/* read a command line */
	while (( readline( fd, cmdline, MAX_LINE_SIZE )) != 0 ) {

		/* run the command */
		rc = processCmd( fs, cmdline );
    
		if ( rc != 0 )
			break;
	}

	close( fd );

	/* collect stats */
  
	return 0;
}


/**********************************************************************

    Function    : printStats
    Description : output the memory management statistics 
    Inputs      : 
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/

void printStats( void )
{
	// To be added later
	printf("***********  Done *************\n");
}


/**********************************************************************

    Function    : main
    Description : this is the main function for project #4
    Inputs      : argc - number of command line parameters
                  argv - the text of the arguments
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/

int main( int argc, char **argv ) 
{
	int i;

	/* allocate the in-memory file system object */
	fs = (filesys_t *)malloc(sizeof(filesys_t));
  
	/* Check for arguments */
	if ( argc < 3 ) 
	{
		/* Complain, explain, and exit */
		errorMessage( "missing or bad command line arguments\n" );
		printf( CLIENT_USAGE );
		exit( -1 );
	}

	/* setup file system */
	fsInitialize( fs, argv[1] );

	/* create this process */
	processInitialize( );

	/* run the test suite against the memory manager */
	for ( i=2; i != argc; i++ ) {
		printf("************ Running commands from file %s on filesystem in file %s **********\n", argv[i], argv[1] );

		runSuite( fs, argv[i] );
	
		/* print the statistics */
		printStats( );
	}

	close( fs->fd );

	exit( 0 );
}

https://tutorcs.com
WeChat: cstutorcs
QQ: 749389476
Email: tutorcs@163.com
#ifndef UTIL_H
#define UTIL_H

/**********************************************************************

   File          : cmpsc473-util.h

   Description   : It contains the functional prototypes for the 
                   needed applications.

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

/* Defines */
#define max( a, b ) ( ( a > b ) ? a : b )
#define min( a, b ) ( ( a < b ) ? a : b )

/* Errors */
#define MAX_ERR_SIZE 255

enum FSErrors
{
    E_Generic = 1,
    E_AlreadyOpen,     // file already exists in file table
    E_Open,            // cannot open file
    E_OpenEXCL,        // cannot open file due to FLAG_EXCL
    E_Exists,          // file/dir already exists on disk
    E_NoPerm,          // do not have perm to open
    E_MaxLinkDepth,    // too many links
    E_MalformedPath,   // cannot parse pathname
    E_DoesNoExist,     // file does not exist
    E_UntrustedLink,   // encountered untrusted link
    E_Link,            // got link in path
    E_DirLink,         // got link to dir in path
    E_Resolve,         // error during path resolution
    E_NoSuchFD,        // no such fd is open
};

/* Colors */
#define USE_COLORS
#define COLOR_RED     "\x1B[31m"
#define COLOR_RED_BG  "\x1B[41m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_END     "\x1B[0m"

#ifdef USE_COLORS
#define CMDCOLOR( str )   COLOR_GREEN  str COLOR_END
#define LINKCOLOR( str )  COLOR_RED    str COLOR_END
#define WARNCOLOR( str )  COLOR_YELLOW str COLOR_END
#define ERRCOLOR( str )   COLOR_RED    str COLOR_END
#else
#define CMDCOLOR( str )   str
#define LINKCOLOR( str )  str
#define WARNCOLOR( str )  str
#define ERRCOLOR( str )   str
#endif


/* Functional Prototypes */

/**********************************************************************

    Function    : errorMessage
    Description : prints an error mesage to stderr
    Inputs      : msg - pointer to string message
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/
int errorMessage( char *msg );

/**********************************************************************

    Function    : warningMessage
    Description : prints an warning mesage to stderr
    Inputs      : msg - pointer to string message
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/
int warningMessage( char *msg );

/**********************************************************************

    Function    : fserror
    Description : convert error number to string
    Inputs      : err - error number
                  msg - custom error message
    Outputs     : none

***********************************************************************/
void fserror( int err, char *msg );

/**********************************************************************

    Function    : printBuffer
    Description : prints buffer to stdout
    Inputs      : msg - header message
                  buf - the buffer
                  len - the length of the buffer
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/
void printBuffer( char *msg, char *buf, int len );

/**********************************************************************

    Function    : readline
    Description : reads a line from file referenced by fd
    Inputs      : fd - file descriptor
                  buf - output buffer 
                  maxlen - size of buffer
    Outputs     : number of bytes if successful, -1 if failure

***********************************************************************/

extern int readline(int fd, char *buf, int maxlen);

#endif

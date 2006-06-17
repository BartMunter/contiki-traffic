/**
 * \addtogroup sys
 * @{
 */

/**
 * \defgroup cfs The Contiki file system interface
 *
 * The Contiki file system interface (CFS) defines an abstract API for
 * reading directories and for reading and writing files. The CFS API
 * is intentionally simple. The CFS API is modeled after the POSIX
 * file API, and slightly simplified.
 *
 * @{
 */

/**
 * \file
 *         CFS header file.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: cfs.h,v 1.1 2006/06/17 22:41:15 adamdunkels Exp $
 */
#ifndef __CFS_H__
#define __CFS_H__

#include "cfs/cfs-service.h"

/**
 * Specify that cfs_open() should open a file for reading.
 *
 * This constant indicates to cfs_open() that a file should be opened
 * for reading. CFS_WRITE should be used if the file is opened for
 * writing, and CFS_READ + CFS_WRITE indicates that the file is opened
 * for both reading and writing.
 *
 * \sa cfs_open()
 */
#define CFS_READ  0

/**
 * Specify that cfs_open() should open a file for writing.
 *
 * This constant indicates to cfs_open() that a file should be opened
 * for writing. CFS_READ should be used if the file is opened for
 * reading, and CFS_READ + CFS_WRITE indicates that the file is opened
 * for both reading and writing.
 *
 * \sa cfs_open()
 */
#define CFS_WRITE 1

/**
 * \brief      Open a file.
 * \param name The name of the file.
 * \param flags CFS_READ, or CFS_WRITE, or both.
 * \return     A file descriptor, if the file could be opened, or -1 if
 *             the file could not be opened.
 *
 *             This function opens a file and returns a file
 *             descriptor for the opened file. If the file could not
 *             be opened, the function returns -1. The function can
 *             open a file for reading or writing, or both.
 *
 *             An opened file must be closed with cfs_close().
 *
 * \sa         CFS_READ
 * \sa         CFS_WRITE
 * \sa         cfs_close()
 */
int cfs_open(const char *name, int flags);

/**
 * \brief      Close an open file.
 * \param fd   The file descriptor of the open file.
 *
 *             This function closes a file that has previously been
 *             opened with cfs_open().
 */
void cfs_close(int fd);

/**
 * \brief      Read data from an open file.
 * \param fd   The file descriptor of the open file.
 * \param buf  The buffer in which data should be read from the file.
 * \param len  The number of bytes that should be read.
 * \return     The number of bytes that was actually read from the file.
 *
 *             This function reads data from an open file into a
 *             buffer. The file must have first been opened with
 *             cfs_open() and the CFS_READ flag.
 */
int cfs_read(int fd, char *buf, unsigned int len);

/**
 * \brief      Write data to an open file.
 * \param fd   The file descriptor of the open file.
 * \param buf  The buffer from which data should be written to the file.
 * \param len  The number of bytes that should be written.
 * \return     The number of bytes that was actually written to the file.
 *
 *             This function reads writes data from a memory buffer to
 *             an open file. The file must have been opened with
 *             cfs_open() and the CFS_WRITE flag.
 */
int cfs_write(int fd, char *buf, unsigned int len);

/**
 * \brief      Seek to a specified position in an open file.
 * \param fd   The file descriptor of the open file.
 * \param offset The position in the file.
 * \return     The new position in the file.
 *
 *             This function moves the file position to the specified
 *             position in the file. The next byte that is read from
 *             or written to the file will be at the position given by
 *             the offset parameter.
 */
int cfs_seek(int fd, unsigned int offset);

/**
 * \brief      Open a directory for reading directory entries.
 * \param dirp A pointer to a struct cfs_dir that is filled in by the function.
 * \param name The name of the directory.
 * \return     0 or -1 if the directory could not be opened.
 *
 * \sa         cfs_readdir()
 * \sa         cfs_closedir()
 */
int cfs_opendir(struct cfs_dir *dirp, const char *name);

/**
 * \brief      Read a directory entry
 * \param dirp A pointer to a struct cfs_dir that has been opened with cfs_opendir().
 * \param dirent A pointer to a struct cfs_dirent that is filled in by cfs_readdir()
 * \retval 0   If a directory entry was read.
 * \retval 0   If no more directory entries can be read.
 *
 * \sa         cfs_opendir()
 * \sa         cfs_closedir()
 */
int cfs_readdir(struct cfs_dir *dirp, struct cfs_dirent *dirent);

/**
 * \brief      Close a directory opened with cfs_opendir().
 * \param dirp A pointer to a struct cfs_dir that has been opened with cfs_opendir().
 *
 * \sa         cfs_opendir()
 * \sa         cfs_readdir()
 */
int cfs_closedir(struct cfs_dir *dirp);

struct cfs_service_interface *cfs_find_service(void);

#define cfs_open(name, flags)   (cfs_find_service()->open(name, flags))
#define cfs_close(fd)           (cfs_find_service()->close(fd))
#define cfs_read(fd, buf, len)  (cfs_find_service()->read(fd, buf, len))
#define cfs_write(fd, buf, len) (cfs_find_service()->write(fd, buf, len))
#define cfs_seek(fd, off)       (cfs_find_service()->seek(fd, off))

#define cfs_opendir(dirp, name) (cfs_find_service()->opendir(dirp, name))
#define cfs_readdir(dirp, ent)  (cfs_find_service()->readdir(dirp, ent))
#define cfs_closedir(dirp)      (cfs_find_service()->closedir(dirp))


#endif /* __CFS_H__ */

/** @} */
/** @} */

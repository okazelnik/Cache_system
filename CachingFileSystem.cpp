/*
 * CachingFileSystem.cpp
 *
 *  Author: Netanel Zakay, HUJI, 67808  (Operating Systems 2015-2016).
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fuse.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

#define _FILE_OFFSET_BITS  64
#define EMPTY_STRING ""
#define LOG_FILE "filesystem.log"
#define FLAGS O_RDONLY | O_DIRECT | O_SYNC
#define READ_ONLY O_RDONLY
#define READ_ONLY_BINARY 3
#define SUCCESS 0
#define NOTHING_TO_READ 0
#define FAILURE -1


#define CACHE ((Cache*) fuse_get_context()->private_data)

using namespace std;

struct fuse_operations caching_oper;


/**
 * Check if there exists a file or folder at given path
 * @param path
 * @return 0 if stat() succeed -> entry exists
 */
int isEntry(string path){


    struct stat fileStatBuf = {0};
    return stat(path.c_str(), &fileStatBuf);
}

/**
 * Check if the entry at given path is a dir
 * @param path
 * @return >0 if it is a dir
 */

int isDirectory(string path){

    struct stat fileStatBuf = {0};
    stat(path.c_str(), &fileStatBuf);
    return S_ISDIR(fileStatBuf.st_mode);
}

/**
 *
 */
int isLogFile(const char *path){

    if (string(path).compare(LOG_FILE) == SUCCESS){

        return -ENOENT;
    }

    return SUCCESS;
}

/**
 *
 */
int checkFullPath(string path){

    if (path.empty() == SUCCESS){

        return -ENAMETOOLONG;
    }

    return SUCCESS;
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *path, struct stat *statbuf){

    int response;
    response = CACHE->writeToLog("getattr", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }
    if (statbuf == NULL){

        return FAILURE;
    }

    // here need to get string fullPath
    string fullPath = CACHE->getFullPath(path);
    response = checkFullPath(fullPath);

    if(response != SUCCESS){

        return -ENAMETOOLONG;
    }

    response = lstat(fullPath.c_str(), statbuf);

    if (response < SUCCESS){

        response = -errno;
    }

    return response;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int caching_fgetattr(const char *path, struct stat *statbuf,
                     struct fuse_file_info *fi) {

    int response;
    response = CACHE->writeToLog("fgetattr", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    response = fstat(fi->fh, statbuf);
    return response;
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int caching_access(const char *path, int mask)
{
    int response;
    // writes to the log file.
    response = CACHE->writeToLog("fgetattr", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    string fullPath = CACHE->getFullPath(path);
    response = checkFullPath(fullPath);
    if(response != SUCCESS){

        return -ENAMETOOLONG;
    }

    response = access(fullPath.c_str(), mask);

    if (response != SUCCESS) {

        response = -errno;
    }

    return response;
}


/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * initialize an arbitrary filehandle (fh) in the fuse_file_info
 * structure, which will be passed to all file operations.

 * pay attention that the max allowed path is PATH_MAX (in limits.h).
 * if the path is longer, return error.

 * Changed in version 2.2
 */
int caching_open(const char *path, struct fuse_file_info *fi){

    int response;
    int fileDescriptor;
    // writs to the log file the function.
    response = CACHE->writeToLog("open", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    string fullPath = CACHE->getFullPath(path);
    response = checkFullPath(fullPath);

    if (response != SUCCESS){

        return -ENAMETOOLONG;
    }

    // checking the fuse info flags.
    if ((fi->flag & READ_ONLY_BINARY) != O_RDONLY){

        return -EACCES;
    }

    // getting the file descriptor of the openning file.
    fileDescriptor = open(fullPath.c_str(), O_RDONLY | O_DIRECT | O_SYNC);
    if (fileDescriptor < SUCCESS){

        response = -errno;
    } else {

        fi->fh = fileDescriptor;
    }

    return response;
}


/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error. For example, if you receive size=100, offest=0,
 * but the size of the file is 10, you will init only the first
   ten bytes in the buff and return the number 10.

   In order to read a file from the disk,
   we strongly advise you to use "pread" rather than "read".
   Pay attention, in pread the offset is valid as long it is
   a multipication of the block size.
   More specifically, pread returns 0 for negative offset
   and an offset after the end of the file
   (as long as the the rest of the requirements are fulfiiled).
   You are suppose to preserve this behavior also in your implementation.

 * Changed in version 2.2
 */
int caching_read(const char *path, char *buf, size_t size,
                 off_t offset, struct fuse_file_info *fi){

    struct stat statStruct;
    Block* curBlock;
    int response;
    int fileDescriptor;
    int readDataSize = NOTHING_TO_READ;
    // writs to the log file the function.
    response = CACHE->writeToLog("read", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    string fullPath = CACHE->getFullPath(path);
    response = checkFullPath(fullPath);

    if (response != SUCCESS){

        return -ENAMETOOLONG;
    }

    response = isEntry(fullPath);
    if (response != SUCCESS){

        return -ENOENT;
    }

    caching_fgetattr(path,&statStruct,fi);
    size_t sizeToRead = (size_t) statStruct.st_size;

    // checks the size of byets we want to read from the file.
    if (size > sizeToRead - (size_t) offset){

        size = sizeToRead - (size_t) offset;
    }

    if (size == NOTHING_TO_READ){

        return SUCCESS;
    }

    // checks how many bytes we need to cover decides from the offest,
    // the size and the blockSize of the OS.
    int startBlock = offset / (size_t)CACHE->blockSize;
    int startBlockOffset = offset % (size_t)CACHE->blockSize;
    int endBlock = (offset + size) / (size_t)CACHE->blockSize;
    int endBlockOffset = (offset + size) % (size_t)CACHE->blockSize;

    // now we should check of we want to read a full block
    if (endBlockOffset == 0){

        endBlock--;
        // complete it
    }

    // going over the blocks that can be found with to fix size the offset.
    for (int i = startBlock; i <= endBlock; i++){

        // checks either the block of the current path exiset and the
        // number of the block is fixed.
        blockIndex = findBlockInContainer(fullPath,i);

        // if we didn't find the block in cache, brings it from the disk.
        if (blockIndex == CACHE->cacheContainer.size()){

            curBlock = CACHE->readBlockFromDisk(fi->fh, fullPath, i);
            if (curBlock == NULL){

                return -errno;
            }

        } else {
            // in case that the block found in the cache.
            curBlock = CACHE->cacheContainer.at(blockIndex);
        }

        // in case we need to read from the last block, in other words
        // only from one block.
        if (startBlock == endBlock){

            strncpy(buf + readDataSize, curBlock->data + startBlockOffset, size);
            readDataSize = size;
            curBlock->usedTimes++;
        }

        // in case we need to read from more than one block.
        // the simplest case, when we stopped on the last block.
        if (i == endBlock){

            strncpy(buf + readDataSize, curBlock->data, endBlockOffset);
            curBlock->usedTimes++;
            readDataSize += endBlockOffset;

        } else if (i == startBlock){

            // in case we are in the first block, we can read until the
            // offset of the start block.
            strncpy(buf + readDataSize, startBlockOffset + curBlock->data,
                    CACHE->blockSize - startBlockOffset);
            curBlock->usedTimes++;
            readDataSize += (CACHE->blockSize - startBlockOffset);

        } else {

            strncpy(buf + readDataSize, curBlock->data ,CACHE->blockSize);
            curBlock->usedTimes++;
            readDataSize += CACHE->blockSize;
        }
    }

    return readDataSize;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int caching_flush(const char *path, struct fuse_file_info *fi)
{
    int response;
    // writs to the log file the function.
    response = CACHE->writeToLog("flush", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    return SUCCESS;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int caching_release(const char *path, struct fuse_file_info *fi){

    int response;
    response = CACHE->writeToLog("release", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    string fullPath = CACHE->getFullPath(path);
    response = checkFullPath(path);
    if(response != SUCCESS){

        return -ENAMETOOLONG;
    }

    response = close(fi->fh);
    return response;

}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi){

    int response;
    DIR *dp;
    response = CACHE->writeToLog("opendir", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    string fullPath = CACHE->getFullPath(path);
    response = checkFullPath(path);
    if(response != SUCCESS){

        return -ENAMETOOLONG;
    }

    dp = opendir(fullPath.c_str());

    if (dp == NULL){

        return -errno;
    }

    fi->fh = (intptr_t) dp;
    return SUCCESS;


}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * Introduced in version 2.3
 */
int caching_readdir(const char *path, void *buf,
                    fuse_fill_dir_t filler,
                    off_t offset, struct fuse_file_info *fi){

    int response = SUCCESS;
    DIR *dp;
    struct dirent *de;
    // write to log
    response = CACHE->writeToLog("readdir", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    dp = (DIR*) (uintptr_t) fi->fh;
    de = readdir(dp);
    if (de == 0){
        response = -errno;
        //return response;
    }

    do {
        // in case it's not the log file
        if ((strcmp(LOG_FILE.c_str(), de->d_name)) != SUCCESS){
            // fill the buffer and make sure it's not full
            if ((filler(buf, de->d_name, NULL, 0) != SUCCESS)) {
                return -ENOMEM;
            }
        }

    } while ((de = readdir(dp)) != NULL);

    return response;

}


/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi){

    int response;
    response = CACHE->writeToLog("releasedir", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    string fullPath = CACHE->getFullPath(path);
    response = checkFullPath(path);
    if(response != SUCCESS){

        return -ENAMETOOLONG;
    }

    closedir((DIR*)(uintptr_t)fi->fh);

    return SUCCESS;
}

/** Rename a file */
int caching_rename(const char *path, const char *newpath){

    int response;
    // here we need insert the log.
    response = CACHE->writeToLog("rename", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    string prevPath = CACHE->getFullPath(path);
    response = checkFullPath(prevPath);
    if(response != SUCCESS){

        return -ENAMETOOLONG;
    }

    string newPath = CACHE->getFullPath(newPath);
    response = checkFullPath(newPath);
    if(response != SUCCESS){

        return -ENAMETOOLONG;
    }

    response = rename(prevPath.c_str(), newPath.c_str());
    if (response < SUCCESS){

        return -errno;
    }

    if (isDirectory(prevPath) == SUCCESS){

        prevPath += "/";
        newPath += "/";
    }

    CACHE->renameBlockCache(prevPath,newPath);
    return response;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *

If a failure occurs in this function, do nothing (absorb the failure
and don't report it).
For your task, the function needs to return NULL always
(if you do something else, be sure to use the fuse_context correctly).
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *caching_init(struct fuse_conn_info *conn){

    int response;
    // here we need insert the log.
    response = CACHE->writeToLog("init", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    return CACHE;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.

If a failure occurs in this function, do nothing
(absorb the failure and don't report it).

 * Introduced in version 2.3
 */
void caching_destroy(void *userdata){

    int response;
    // here we need insert the log.
    response = CACHE->writeToLog("destroy", 1);
    if (response == FAILURE){

        return FAILURE;
    }

    response = isLogFile(path);
    if (response != SUCCESS){

        return -ENOENT;
    }

    delete CACHE;

}


/**
 * Ioctl from the FUSE sepc:
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * However, in our case, this function only needs to print
 cache table to the log file .
 *
 * Introduced in version 2.8
 */
int caching_ioctl (const char *, int cmd, void *arg,
                   struct fuse_file_info *, unsigned int flags, void *data){

    int response;
    // here we need insert the log.
    response = CACHE->writeToLog("ioctl",1);
    if (response == FAILURE){

        return FAILURE;
    }

    // open the filesystem log for updates it.
    response = CACHE->writeToLog("ioctl",2);
    if (response == FAILURE){

        response =  -ENOENT;
    }

    return response;
}


// Initialise the operations.
// You are not supposed to change this function.
void init_caching_oper()
{

    caching_oper.getattr = caching_getattr;
    caching_oper.access = caching_access;
    caching_oper.open = caching_open;
    caching_oper.read = caching_read;
    caching_oper.flush = caching_flush;
    caching_oper.release = caching_release;
    caching_oper.opendir = caching_opendir;
    caching_oper.readdir = caching_readdir;
    caching_oper.releasedir = caching_releasedir;
    caching_oper.rename = caching_rename;
    caching_oper.init = caching_init;
    caching_oper.destroy = caching_destroy;
    caching_oper.ioctl = caching_ioctl;
    caching_oper.fgetattr = caching_fgetattr;


    caching_oper.readlink = NULL;
    caching_oper.getdir = NULL;
    caching_oper.mknod = NULL;
    caching_oper.mkdir = NULL;
    caching_oper.unlink = NULL;
    caching_oper.rmdir = NULL;
    caching_oper.symlink = NULL;
    caching_oper.link = NULL;
    caching_oper.chmod = NULL;
    caching_oper.chown = NULL;
    caching_oper.truncate = NULL;
    caching_oper.utime = NULL;
    caching_oper.write = NULL;
    caching_oper.statfs = NULL;
    caching_oper.fsync = NULL;
    caching_oper.setxattr = NULL;
    caching_oper.getxattr = NULL;
    caching_oper.listxattr = NULL;
    caching_oper.removexattr = NULL;
    caching_oper.fsyncdir = NULL;
    caching_oper.create = NULL;
    caching_oper.ftruncate = NULL;
}

//basic main. You need to complete it.
int main(int argc, char* argv[]){

    init_caching_oper();
    argv[1] = argv[2];
    for (int i = 2; i< (argc - 1); i++){
        argv[i] = NULL;
    }
    argv[2] = (char*) "-s";
    argc = 3;

    int fuse_stat = fuse_main(argc, argv, &caching_oper, NULL);
    return fuse_stat;
}

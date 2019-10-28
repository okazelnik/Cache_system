//
// Created by omri1990 on 5/19/16.
//

#include "Cache.h"
#include <malloc.h>
#include <limits.h>
#include <string.h>
#include <fstream>
#include <time>
#include <linux/limits.h>
#include <unistd.h>

#define SUCCESS 0
#define FAILURE -1

#define EMPTY_PATH ""
#define DIR_SEPARATOR "/"
#define BLOCKNUM_SEPARATOR ":"
#define FIRST_STATE 1
#define SECOND_STATE 2

/**
 * getting  the canonicalized absolute pathname
 * @param path - the path to check.
 * @return the path after string it.
 */
string Cache::getRealPath(string path) {

    char* realPath = realpath(path.c_str(), NULL);

    if (realPath == NULL){

        return "NULL";

    }

    string retVal = realPath;
    free(realPath);
    return retVal;
}

/**
 * getting the full path of the file.
 * @param path  the path to check.
 * @return the full path of the file.
 */
string Cache::getFullPath(const char *path) {

    string fullPath = rootDir + string(path);
    if (fullPath.length() > PATH_MAX){

        return EMPTY_PATH;
    }

    return fullPath;
}


/**
 * default constructor of the cache
 */
Cache::Cache()
        : rootDir("/")
        , maxBlockSize(0)
        , blockSize(0)
        , logFile("/"){}

/**
 * parameters constructor for the cache.
 */
Cache::Cache(string root, string logFileVal,
             int blockSizeToSet,int numOfBlocksToSet, int oldSetLimit, int newSetLimit)
        : rootDir(getRealPath(root))
        , numOfBlocks(numOfBlocksToSet)
        , blockSize(blockSizeToSet)
        , oldLimit(oldSetLimit)
        , newLimit(newSetLimit)
        , logFile(logFileVal)
{
    if (rootDir == "NULL") {
        throw -errno;

    }

}

/**
 * destructor of the cache.
 */
Cache::~Cache() {

    for (int i = 0; i < cacheContainer.size(); i++){

        delete cacheContainer.at(i);

    }

    cacheContainer.clear();
}

/**
 * reading the data from the disk
 * @param fh the file handler (descriptor).
 *  path the path of the file.
 *  blockNum the number of the block read from the file.
 */
Block* Cache::readFromDisk(uint64_t fh, string path, int numOfBlock) {

    int response;
    // getting the buffer for the data.
    char* buffer = new char[blockSize + 1];

    if (buffer == NULL){

        return NULL;
    }

    // reads the data from the disk, gets the data to buffer in blocksize.
    response = pread(fh, buffer, blockSize, blockSize * numOfBlock);

    if (response < SUCCESS){

        delete[] (buffer);
        return NULL;

    } else{

        // in case of success, checks if the cache is full.
        if (cacheContainer.size() >= maxBlockSize){

            DeleteFromCache();
        }

        // if not full, insert the new block to the cache.
        Block* readBlock = new Block(buffer,path,numOfBlock);
        insertToCache(readBlock);
        return readBlock;
    }

}

/**
 * inserts the new data block to the cache.
 * @param block the block to insert.
 */
void Cache::insertToCache(Block *block) {

    cacheContainer.push_back(block);
    Block* popedBlock = cacheContainer.at(newLimit - 1);
    delete cacheContainer.at(newLimit - 1);
    cacheContainer.push_back(popedBlock);
}

/**
 * Delete the lfu block from the cache container.
 */
void Cache::DeleteFromCache() {

    int lfuBlockTime = cacheContainer.at(0)->usedTimes;
    int lfuIndex = 0;
    // going over the old inner vector and seeking after the LFU.
    for (int i = 1; i < oldLimit ; i++){

        if (cacheContainer.at(i)->usedTimes < lfuBlock){

            lfuBlockTime = cacheContainer.at(i)->usedTimes;
            lfuIndex = i;
        }
    }

    delete cacheContainer.at(lfuIndex);
}

/**
 * check this method!!!!
 */
void Cache::renameBlockCache(string prevPath, string newPath) {

    for (cacheContainer::iterator it = cacheContainer.begin();
         it != cacheContainer.end(); it++){

        // if we found the block in the cache, change the path.
        if ((*it)->path.compare(0,prevPath.length(), oldPath)){

            cacheContainer.erase((*it));
            (*it)->path.replace(0,prevPath.length(), newPath);
            cacheContainer.push_back(*it);
        }
    }
}

/**
 * writes the function the the operating time of it to the log.
 * @param func the name of the function.
 */
int Cache::writeToLog(string func, int state) {

    if (state == FIRST_STATE) {

        try {

            ofstream logFileStream;
            logFileStream.open(this->logFile, ios_base::app);
            time_t time = time(nullptr);
            if (time < SUCCESS) {

                // write here an error
            }

            logFileStream << time << " " << func << endl;
            logFileStream.close();
            return SUCCESS;

        } catch (exception &e) {

            cerr << "FAILURE in open the log file " << e.what() << endl;
            return FAILURE;
        }
    } else {

        try {

            ofstream logFileStream;
            logFileStream.open(this->logFile, ios_base::app);
            for (int i = 0; i < cacheContainer.size(); i++) {

                logFileStream << cacheContainer.at(i)->path << " " <<
                cacheContainer.at(i)->numOfBlock << " " << cacheContainer.at(i)->usedTimes <<
                endl;
            }

            logFileStream.close();
            return SUCCESS;

        } catch (exception &e) {

            cerr << "FAILURE in open the log file " << e.what() << endl;
            return FAILURE;
        }
    }
}

/**
 *
 */
int Cache::findBlockInContainer(Block* blockToFind, int numOfBlock) {

    // this function should run over the vector and find the block.
    int i;
    for (i = 0; i < cacheContainer.size(); i++){

        if (cacheContainer.at(i)->path == blockToFind->path &&
                cacheContainer.at(i)->num == i ){

            return i;
        }
    }

    return (i + 1);
}
//
// Created by omri1990 on 5/19/16.
//

#ifndef EX4_CACHE_H
#define EX4_CACHE_H

#include "Block.h"
#include <vector>
#include <string>

class Cache{
public:
    vector<Block*> cacheContainer;
    string rootDir;
    string logFile;
    int numOfBlocks;
    int blockSize;
    int oldLimit;
    int newLimit;
    Cache();
    Cache(string rootDir, string logFile,
          int blockSizeToSet, int numOfBlocksToSet, int oldSetLimit, int newSetLimit);
    ~Cache();
    string getRealPath(string path);
    string getFullPath(const char* path);
    void insertToCache(Block* block);
    Block* readFromDisk(uint64_t fh, string path, unsigned long blockNum);
    void DeleteFromCache();
    void renameBlockCache(string prevPath, string newPath);
    void writeToLog(string func, int state);
    int findBlockInContainer(Block* blockToFind, int numOfBlock);


};
#endif //EX4_CACHE_H

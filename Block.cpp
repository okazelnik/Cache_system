//
// Created by omri1990 on 5/19/16.
//

#include "Block.h"


Block::Block(char* newData, string path,
                      int numOfBlockToSet)
        : data(newData)
        , path(path)
        , numOfBlock(numOfBlockToSet)
{
}

/**
 *  Block Destructor
 */
Block::~Block()
{
    delete[](data);
}


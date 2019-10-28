//
// Created by omri1990 on 5/19/16.
//

#ifndef EX4_BLOCK_H
#define EX4_BLOCK_H

#include <string>
#include <iostream>

using namespace std;

#include <string>

using namespace std;

class Block {
public:

    Block(char *data, string path, int numOfBlockToSet);
    ~Block();
    char *data;
    string path;
    int numOfBlock;
    int usedTimes;
};

#endif //EX4_BLOCK_H



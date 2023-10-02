#include <stdio.h>
#include "udp.h"
#include "ufs.h"



int server_LookUp(int fd, int pinum, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable, char* buffer);
int server_Stat(int fd, int inumber, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable, MFS_Stat_t* buffer);
int server_Write(int fd, int inumber, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable, char* buffer, int offSet, int byteSize);
int server_Read(int fd, int inumber, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable, char* buffer, int offSet, int byteSize);
int server_Create(int fd, int pinum, bitmap_t* inodeBitmap, bitmap_t* dataBitMap, inode_block* inodeTable, int fileType, char* name);
int server_Unlink(int fd, int inumber, bitmap_t* inodeBitmap, bitmap_t* dataBitMap, inode_block* inodeTable, char* name); //0 on success -1 on failure

int validateInodeNumber(int inum, bitmap_t* bitmap);
int findSpace(bitmap_t* bitmap, int mapVersion);
int allocateBlock(bitmap_t* bitmap, int mapVersion);
void setBit(bitmap_t* bitmap, int val, int blockNumber, int mapVersion);
int checkBit(bitmap_t* bitmap, int blockNumber);
int sync_OnDiskStructures(int fd, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable);
void intHandler(int dummy);
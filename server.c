#include "server.h"

// server code
super_t superBlock;
int sd;
int fd;

int main(int argc, char *argv[]) {
    signal(SIGINT, intHandler);
    int closesd;

    if(argc != 3) 
    {
        perror("Missing Parameters...");
        exit(1);
    }

    int port = atoi(argv[1]);    //get the port number that is going to listen
    printf("Trying to listen on Port %d", port);
    char* filePath = argv[2];   // get the file path
     fd = open(filePath, O_RDWR);    //open the file, return the file descriptor, mode: read/write 
    if(fd == -1)
    {
        perror("image does not exist\n");
        exit(1);
    }

    



    
    inode_block inodeTable[32];
    
    lseek(fd, 0, SEEK_SET);
    if(-1 == read(fd, &superBlock, sizeof(super_t)) )   //read the super block
    {
        perror("Read SuperBlock Errors...");
        return -1;
    }

    bitmap_t inodeBitMap[superBlock.inode_bitmap_len];
    bitmap_t dataBitMap[superBlock.data_bitmap_len];

    
    lseek(fd, (superBlock.inode_bitmap_addr) * UFS_BLOCK_SIZE, SEEK_SET);
    if(-1 == read(fd, inodeBitMap, superBlock.inode_bitmap_len * UFS_BLOCK_SIZE) )   //read the inodeBitMap
    {
        perror("Read inodeBitMap Errors...");
        return -1;
    }
    printf("inode 1:  %d\n",  checkBit(inodeBitMap, 1));
    //printBitMap(inodeBitMap, 4);
    lseek(fd, (superBlock.data_bitmap_addr) * UFS_BLOCK_SIZE, SEEK_SET);

    if(-1 == read(fd, dataBitMap, superBlock.data_bitmap_len * UFS_BLOCK_SIZE) )   //read the dataBitMap
    {
        perror("Read data bitmap Errors...");
        return -1;
    }

    lseek(fd, (superBlock.inode_region_addr) * UFS_BLOCK_SIZE, SEEK_SET);
    if(-1 == read(fd, inodeTable, superBlock.inode_region_len * UFS_BLOCK_SIZE) )   //read the inode table
    {
        perror("Read inodeBitMap Errors...");
        return -1;
    }
    



    

    sd = UDP_Open(port);
    while(sd <= -1)
    {
        sd = UDP_Open(port);
    }

    closesd = sd;


    while (1) {
	struct sockaddr_in addr;
	msg_t message;
	printf("server:: waiting for command...\n");
	int rc = UDP_Read(sd, &addr, &message, sizeof(msg_t));
    while(rc < 0)
    {
        perror("server :: didn't receive any command and retry now\n");
        rc = UDP_Read(sd, &addr, &message, sizeof(msg_t));
        
    }
    printf("MESSAGE: %d\n", message.msgType);
    int status;

	if (rc > 0) {

        printf("server:: receive a command\n");
        msg_t response;
       /* if(message.msgType == 0)    //just make sure its connected (init)
        {
            //response.msgType = 0;
            //status = 666;
        }else*/ if(message.msgType == 1){ //return inode number of name
            status = server_LookUp(fd, message.inumber, inodeBitMap, dataBitMap, inodeTable, message.buffer);
        }else if (message.msgType == 2){    //stat
            status =  server_Stat(fd, message.inumber, inodeBitMap, dataBitMap, inodeTable, (MFS_Stat_t*) response.buffer);
        }else if(message.msgType == 3){ //write
            status = server_Write(fd, message.inumber, inodeBitMap, dataBitMap, inodeTable, message.buffer, message.offset, message.bytes);
        }else if(message.msgType == 4){ //read
            status = server_Read(fd, message.inumber, inodeBitMap, dataBitMap, inodeTable, response.buffer, message.offset, message.bytes);
        }else if(message.msgType == 5){ //create
            status = server_Create(fd, message.inumber, inodeBitMap, dataBitMap, inodeTable, message.fileType, message.buffer);
            
        }else if(message.msgType == 6){ //unlink
            status = server_Unlink(fd, message.inumber, inodeBitMap, dataBitMap, inodeTable, message.buffer);
        }else if(message.msgType == 7){ //shutdown
            fsync(fd);  //flush to the disk
            status = 444;
            close(fd);
            UDP_Close(closesd);
            exit(0);
        }else continue; //unknown command

        response.inumber = status;
            
        rc = UDP_Write(sd, &addr, (char*)&response, sizeof(msg_t));
	    printf("server:: Replied to the client\n");
        printf("response:  %d\n", response.inumber);


	} 

    }
    return 0; 
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//METHODS
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

int server_LookUp(int fd, int pinum, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable, char* buffer)
{
    int row = pinum / 32;
    int rowIndex = pinum % 32;

            if(validateInodeNumber(pinum, inodeBitMap) != 0 || inodeTable[row].inodes[rowIndex].type != 0) return -1;   //failure cases: not valid inode, is not a directory
            
            //access inode table
            for(int i = 0; i < DIRECT_PTRS; i++)
            {
                unsigned int blockNumber = inodeTable[row].inodes[rowIndex].direct[i];
                if(blockNumber == -1) continue;
                lseek(fd, blockNumber * UFS_BLOCK_SIZE, SEEK_SET);
                dir_block_t dirBlock;
                read(fd, &dirBlock, sizeof(dir_block_t));
                
                for(int j = 0; j < 128; j++)
                {
                    if( 0 == strcmp(dirBlock.entries[j].name, buffer)) return dirBlock.entries[j].inum; //found
                   // printf("entry name > %s\n", dirBlock.entries[j].name);
                }
            } 



            return -1; 
}



int server_Stat(int fd, int inumber, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable, MFS_Stat_t* buffer)
{
            MFS_Stat_t stat;
            int row = inumber / 32;
            int rowIndex = inumber % 32;
            if(checkBit(inodeBitMap, inumber) == 0) return -1;

            stat.size = inodeTable[row].inodes[rowIndex].size;
            stat.type = inodeTable[row].inodes[rowIndex].type;
            memcpy(buffer, &stat, sizeof(MFS_Stat_t));

            return 0;
}


int server_Write(int fd, int inumber, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable, char* buffer, int offSet, int byteSize)
{
            int row = inumber / 32;
            int rowIndex = inumber % 32;
           // printf("pre\n");
            printf("type: row-%d id-%d   --> type-%d\n", row, rowIndex, inodeTable[row].inodes[rowIndex].type);
            if(inumber < 0 || inumber >= superBlock.num_inodes || checkBit(inodeBitMap, inumber) == 0|| inodeTable[row].inodes[rowIndex].type == 0) return -1;

            int fileSize = inodeTable[row].inodes[rowIndex].size;
            //printf("prepre\n");

            if(offSet < 0 || offSet > fileSize || offSet + byteSize > 30 * UFS_BLOCK_SIZE || byteSize < 0 || byteSize > UFS_BLOCK_SIZE) return -1;

    
            int ptrNum = offSet / UFS_BLOCK_SIZE;
            int blockOffset = offSet % UFS_BLOCK_SIZE;
            //printf("a\n");

            int blockNumber = inodeTable[row].inodes[rowIndex].direct[ptrNum];

            if(inodeTable[row].inodes[rowIndex].direct[ptrNum] == -1 )  //need to allocate a new block
            {
                blockNumber = allocateBlock(dataBitMap, 1);    //allocate a new data block
              //  printf("b\n");
                if(blockNumber == -1) return -1;

                blockNumber += superBlock.data_region_addr; //plus the offset
                inodeTable[row].inodes[rowIndex].direct[ptrNum] = blockNumber;
                
            }

            lseek(fd, blockNumber * UFS_BLOCK_SIZE + blockOffset, SEEK_SET);

            if(blockOffset + byteSize > 4096)   //write in two times
            {
                int firstWriteBytes = 4096 - blockOffset;
                int remainWriteBytes = byteSize - firstWriteBytes;

                write(fd, buffer, firstWriteBytes);

                //find the next block
                if(inodeTable[row].inodes[rowIndex].direct[ptrNum+1] == -1) //need to allocate a new data block
                {
                    int newBlock = allocateBlock(dataBitMap, 1);
                    //printf("c\n");
                    if(newBlock == -1) return -1;

                    inodeTable[row].inodes[rowIndex].direct[ptrNum+1] = newBlock;
                }
                lseek(fd, inodeTable[row].inodes[rowIndex].direct[ptrNum+1] * UFS_BLOCK_SIZE , SEEK_SET);   //second write
                write(fd, buffer + firstWriteBytes, remainWriteBytes);

            }else write(fd, buffer, byteSize); //could write in one time
            

            if(offSet + byteSize > fileSize) inodeTable[row].inodes[rowIndex].size = offSet + byteSize;
            //printf("d\n");
           // if( -1 == sync_OnDiskStructures(fd, inodeBitMap, dataBitMap, inodeTable) ) return -1;
            //printf("e\n");


            //update the inodeTable to disk
            lseek(fd, (superBlock.inode_region_addr + row) * UFS_BLOCK_SIZE + rowIndex * sizeof(inode_t), SEEK_SET);
            write(fd, &inodeTable[row].inodes[rowIndex], sizeof(inode_t));




            return fsync(fd);
}

int server_Read(int fd, int inumber, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable, char* buffer, int offSet, int byteSize)
{

           // printf("a\n");
            if(inumber < 0 || inumber >= superBlock.num_inodes || checkBit(inodeBitMap, inumber) == 0 ) return -1;

            int row = inumber/32;
            int rowIndex = inumber % 32;

            int fileSize = inodeTable[row].inodes[rowIndex].size;
            int fileType = inodeTable[row].inodes[rowIndex].type;
            
            int ptrNum = offSet / UFS_BLOCK_SIZE;
            int blockOffset = offSet % UFS_BLOCK_SIZE;
            
            //printf("b\n");
            if(offSet < 0 || offSet > fileSize || offSet + byteSize > 30 * UFS_BLOCK_SIZE || inodeTable[row].inodes[rowIndex].direct[ptrNum] == -1 || byteSize < 0 || byteSize > UFS_BLOCK_SIZE) return -1;

                //printf("c\n");
            if(fileType == 0 && (offSet % sizeof(MFS_DirEnt_t) != 0 || byteSize % sizeof(MFS_DirEnt_t) != 0)) return -1;    //failure case: invalid read for directory

            
            


            int blockNumber = inodeTable[row].inodes[rowIndex].direct[ptrNum];

            lseek(fd, blockNumber * UFS_BLOCK_SIZE + blockOffset, SEEK_SET);
            
            if(blockOffset + byteSize > 4096)    //if its cross block, divide it in two times...
            {
                int firstReadByteSize = 4096 -  blockOffset;
                read(fd, buffer, firstReadByteSize);
                if(ptrNum + 1 >= DIRECT_PTRS || inodeTable[row].inodes[rowIndex].direct[ptrNum+1] == -1) return -1;    //failure case: unreachable read
                int secondBlockNumber = inodeTable[row].inodes[rowIndex].direct[ptrNum+1];
                lseek(fd, secondBlockNumber * UFS_BLOCK_SIZE , SEEK_SET);

                read(fd, buffer + firstReadByteSize, byteSize - firstReadByteSize);

            }else{  //could read in one time
                read(fd, buffer, byteSize);
            }

            return 0;
}


/*
int MFS_Creat(int pinum, int type, char *name): 
MFS_Creat() makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified by pinum of name [name]. 
Returns 0 on success, -1 on failure. 
Failure modes: pinum does not exist, or name is too long. If name already exists, return success.
*/


int server_Create(int fd, int pinum, bitmap_t* inodeBitmap, bitmap_t* dataBitMap, inode_block* inodeTable, int fileType, char* name)
{
            
            //check if pinum exist
            if(checkBit(inodeBitmap, pinum) == 0) return -1;    //failure case: pinum does not exist
            printf("a\n");

            int row = pinum / 32;
            int rowIndex = pinum % 32;

            if(inodeTable[row].inodes[rowIndex].type != 0) return -1;   //failure case: pinum is not a directory
            printf("b\n");
            //scan for whether NAME already exists in the parent directory
            
            for(int i = 0 ; i < DIRECT_PTRS; i++)
            {
                unsigned int tempDataPtr = inodeTable[row].inodes[rowIndex].direct[i];
                if(tempDataPtr == -1) continue; //not allocated for this block, just skip..
                
                dir_block_t currDirBlock;
                lseek(fd, tempDataPtr, SEEK_SET);
                read(fd, &currDirBlock, sizeof(dir_block_t));
                
                for(int j = 0; j < 128; j++)
                {
                    if( currDirBlock.entries[j].inum != -1 && 0 == strcmp(currDirBlock.entries[j].name, name)) return 0; //SPECIAL CASE: name already exists, return success
                }
            }
            
            //###>>> following codes are for creating a new file <<<###

            //find an inode for the new file
            //printBitMap(inodeBitmap, 4);
            int newInode = findSpace(inodeBitmap, 0);
            printf("new Inode ->%d\n", newInode);

            if(newInode == -1) return -1;   //failure case: inodes are full
            setBit(inodeBitmap, 1, newInode, 0);
            
            int created = 0;
            //printf("c\n");
            for(int i = 0 ; i < DIRECT_PTRS; i++)   //update directory entry
            {
                unsigned int blockNum = inodeTable[row].inodes[rowIndex].direct[i];
                if(blockNum == -1)  //there's no dirEntry block or we need one dirEntry block
                {
                    int newDataBlock = findSpace(dataBitMap, 1);
                    if(newDataBlock == -1) return -1;   //failure case: no free data blocks.

                    setBit(dataBitMap, 1, newDataBlock, 1);    //reflect the datablock is in use
                    inodeTable[row].inodes[rowIndex].direct[i] = newDataBlock + superBlock.data_region_addr;

                    lseek(fd, (newDataBlock + superBlock.data_region_addr) * UFS_BLOCK_SIZE, SEEK_SET);
                    
                    blockNum = newDataBlock + superBlock.data_region_addr;
                }

                
                

                dir_block_t temp;
                lseek(fd, blockNum * UFS_BLOCK_SIZE  , SEEK_SET);
                read(fd, &temp, sizeof(dir_block_t) );

                for(int i = 0; i < 128; i++)
                {
                    if(temp.entries[i].inum == -1)
                    {
                        temp.entries[i].inum = newInode;
                        strcpy(temp.entries[i].name, name);
                        created = 1;
                        break;
                    }
                }   

                if(created == 0) continue;
                lseek(fd, blockNum * UFS_BLOCK_SIZE  , SEEK_SET);
                write(fd, &temp, sizeof(dir_block_t));  //update the directory block

                if(created) break;
            }
            
            if(created == 0) return -1; //failure case: insert dirEntry unsuccessful

            created = 0;    //reset status

            //====finished insert dirEntry====

            //update inode table

            inodeTable[newInode/32].inodes[newInode % 32].type = fileType;
            inodeTable[newInode/32].inodes[newInode % 32].size = 0;

            for(int i = 0; i < DIRECT_PTRS; i++)    //set all the direct pointers to -1, since its an empty file
            {
                inodeTable[newInode/32].inodes[newInode % 32].direct[i] = -1;
            }

            if(fileType == 0)   //when the type is directory, it should have at least have two entries..
            {
                //find a datablock for directory entry block
                int dirEntryBlock = allocateBlock(dataBitMap, 1);
             //   printf("d");
                if(dirEntryBlock == -1) return -1;  //failure case: does not have space for dirEnt block
                inodeTable[newInode/32].inodes[newInode % 32].direct[0] = dirEntryBlock + superBlock.data_region_addr;

                for(int i = 1; i < DIRECT_PTRS; i++)
                {
                    inodeTable[newInode/32].inodes[newInode % 32].direct[i] = -1;
                }
                
                created = lseek(fd, (dirEntryBlock + superBlock.data_region_addr) * UFS_BLOCK_SIZE  , SEEK_SET);
                if(created == -1) return created;

                dir_block_t temp;
                strcpy(temp.entries[0].name, ".");
                temp.entries[0].inum = newInode;
                strcpy(temp.entries[1].name, "..");
                temp.entries[1].inum = pinum;

                for(int i = 2; i < 128; i++)
                {
                    temp.entries[i].inum = -1;
                }

                created = write(fd, &temp, sizeof(dir_block_t));
                if(created == -1) return created;
                inodeTable[newInode/32].inodes[newInode % 32].size = 2*sizeof(dir_ent_t);

                            //update the inodeTable to disk
                lseek(fd, (superBlock.inode_region_addr + (newInode/32)) * UFS_BLOCK_SIZE + (newInode % 32) * sizeof(inode_t), SEEK_SET);
                write(fd, &inodeTable[newInode/32].inodes[newInode % 32], sizeof(inode_t));

            }
           // printf("e");
            //if( -1 == sync_OnDiskStructures(fd, inodeBitmap, dataBitMap, inodeTable) ) return -1;


                        //update the inodeTable to disk
            lseek(fd, (superBlock.inode_region_addr + row) * UFS_BLOCK_SIZE + rowIndex * sizeof(inode_t), SEEK_SET);
            write(fd, &inodeTable[row].inodes[rowIndex], sizeof(inode_t));

            
            return fsync(fd);
}

int server_Unlink(int fd, int inumber, bitmap_t* inodeBitmap, bitmap_t* dataBitMap, inode_block* inodeTable, char* name) //0 on success -1 on failure
{
    //check whether the pinum exists
    if(inumber < 0 || inumber >= superBlock.num_inodes || checkBit(inodeBitmap, inumber) == 0 ) return -1;
    int row = inumber / 32;
    int rowIndex = inumber % 32;
    if(inodeTable[row].inodes[rowIndex].type != 0) return -1;
    int found = 0;
    
    for(int i = 0; i < DIRECT_PTRS; i++)    //perform a linear scan of the target name
    {
        unsigned int dirEntryBlockNum = inodeTable[row].inodes[rowIndex].direct[i];
        lseek(fd, dirEntryBlockNum * UFS_BLOCK_SIZE, SEEK_SET);
        dir_block_t tempDirBlock;
        read(fd, &tempDirBlock, sizeof(dir_block_t));
        
        for(int j = 0; j < 128; j++)
        {
            if(tempDirBlock.entries[j].inum != -1 && 0 == strcmp(tempDirBlock.entries[j].name, name))
            {
                int targetInum = tempDirBlock.entries[j].inum;
                int targetType = inodeTable[targetInum / 32].inodes[targetInum % 32].type;
                int targetSize = inodeTable[targetInum / 32].inodes[targetInum % 32].size;
                
                //check the targetNode if it's a directory
                if(targetType == 0 )   //test if the directory is empty
                {
                    for(int k = 0; k < DIRECT_PTRS; k++)
                    {
                        unsigned int scanBlock = inodeTable[targetInum / 32].inodes[targetInum % 32].direct[k];
                        if(scanBlock == -1) continue;
                        //access the block;
                        lseek(fd, scanBlock * UFS_BLOCK_SIZE, SEEK_SET);
                        dir_block_t tmpDir;
                        read(fd, &tmpDir, sizeof(dir_block_t));

                        for(int m = 0 ; m < 128; m++)
                        {
                            if(tmpDir.entries[m].inum != -1 && tmpDir.entries[m].inum != inumber && tmpDir.entries[m].inum != targetInum) return -1;    //non-empty dir
                        }


                    }
                }

                for(int k = 0; k < DIRECT_PTRS; k++)
                {
                    unsigned int blockNumToBeDeleted = inodeTable[targetInum / 32].inodes[targetInum % 32].direct[k];
                    inodeTable[targetInum / 32].inodes[targetInum % 32].direct[k] = -1;
                    setBit(dataBitMap, 0, blockNumToBeDeleted - superBlock.data_region_addr, 1);   //set curr dirPtr to free on data bitmap
                }

                setBit(inodeBitmap, 0, targetInum,0); //set the deleted inode to free
                tempDirBlock.entries[j].inum = -1;

                inodeTable[row].inodes[rowIndex].size -= sizeof(dir_ent_t); //reflect the deletion on size of directory
                
                found = 1;
                            //update the inodeTable to disk
                lseek(fd, (superBlock.inode_region_addr + (targetInum / 32)) * UFS_BLOCK_SIZE + (targetInum % 32) * sizeof(inode_t), SEEK_SET);
                write(fd, &inodeTable[targetInum / 32].inodes[targetInum % 32], sizeof(inode_t));
                break;

            } 
        }

        lseek(fd, dirEntryBlockNum * UFS_BLOCK_SIZE, SEEK_SET);
        write(fd, &tempDirBlock, sizeof(dir_block_t));

                    //update the inodeTable to disk
        lseek(fd, (superBlock.inode_region_addr + row) * UFS_BLOCK_SIZE + rowIndex * sizeof(inode_t), SEEK_SET);
        write(fd, &inodeTable[row].inodes[rowIndex], sizeof(inode_t));
        if(found) break;
        
    }

    //if( -1 == sync_OnDiskStructures(fd, inodeBitmap, dataBitMap, inodeTable) ) return -1;
    return fsync(fd);   //flush to the disk
}

//--------------------###################Utilities--############################
int sync_OnDiskStructures(int fd, bitmap_t* inodeBitMap, bitmap_t* dataBitMap, inode_block* inodeTable)
{
    int status;
    lseek(fd, superBlock.inode_bitmap_addr * UFS_BLOCK_SIZE, SEEK_SET); //write inode Bitmap
    status = write(fd, inodeBitMap, superBlock.inode_bitmap_len * UFS_BLOCK_SIZE);
    if(status == -1) return -1;

    lseek(fd, superBlock.data_bitmap_addr * UFS_BLOCK_SIZE, SEEK_SET); //write data Bitmap
    status = write(fd, dataBitMap, superBlock.data_bitmap_len * UFS_BLOCK_SIZE);
    if(status == -1) return -1;

    lseek(fd, superBlock.inode_region_addr * UFS_BLOCK_SIZE, SEEK_SET); //write inode table
    status = write(fd, inodeTable, superBlock.inode_region_len * UFS_BLOCK_SIZE);
    if(status == -1) return -1;    

    return 0;

}

int validateInodeNumber(int inum, bitmap_t* bitmap)
{
    if(inum < 0 || inum >= superBlock.num_inodes*32 || checkBit(bitmap, inum) == 0) return -1;
    return 0;
}

int findSpace(bitmap_t* bitmap, int mapVersion)  //return block number, if no space available return -1;
{
    int length = mapVersion == 0 ? superBlock.num_inodes : superBlock.num_data;
    int numPerBlock  = mapVersion == 0 ? 32: 1;
    length *= numPerBlock; 
    //printf("bitmap length: %d\n", length);
    for(int i = 0 ; i < length; i++)
    {
        //printf("find for bit: %d\n", i);
        if(checkBit(bitmap, i) == 0) return i;

    }

    return -1;

} 


int allocateBlock(bitmap_t* bitmap, int mapVersion)
{
    int emptyBlock = findSpace(bitmap, mapVersion);
    if(emptyBlock == -1) return -1;
    setBit(bitmap, 1, emptyBlock, mapVersion);  //update the bitmap for user
    return emptyBlock;
}

void setBit(bitmap_t* bitmap, int val, int blockNumber, int mapVersion)
{
    int rowNumber = blockNumber/32;
    int rowIndex = blockNumber % 32;
    int bitBlockNumber = rowNumber/1024;
    rowNumber = rowNumber % 1024;

    unsigned int originalVal = bitmap[bitBlockNumber].bits[rowNumber];
    unsigned int bitMask = 1;
    if(val == 1){
        
        bitMask = bitMask << (31-rowIndex);
        bitmap[bitBlockNumber].bits[rowNumber] = originalVal | bitMask;
    }else{
        bitMask = ~(bitMask << (31 - rowIndex));
        bitmap[bitBlockNumber].bits[rowNumber] = originalVal & bitMask;
    }

    //write to the file
    int status;
    if(mapVersion == 0) // inodeBitmap
    {
        lseek(fd, (superBlock.inode_bitmap_addr + bitBlockNumber) * UFS_BLOCK_SIZE + rowNumber * sizeof(unsigned int), SEEK_SET); //write inode Bitmap
        status = write(fd, &bitmap[bitBlockNumber].bits[rowNumber], sizeof(unsigned int));
        if(status == -1) return -1;
    }else{  //dataBitmap
        lseek(fd, (superBlock.data_bitmap_addr + bitBlockNumber) * UFS_BLOCK_SIZE + rowNumber * sizeof(unsigned int), SEEK_SET); //write inode Bitmap
        status = write(fd, &bitmap[bitBlockNumber].bits[rowNumber], sizeof(unsigned int));
        if(status == -1) return -1;
    }
}

int checkBit(bitmap_t* bitmap, int blockNumber)
{
    int rowNumber = blockNumber/32;
    int rowIndex = blockNumber % 32;
    int bitBlockNumber = rowNumber/1024;
    rowNumber = rowNumber % 1024;

    unsigned int originalVal = bitmap[bitBlockNumber].bits[rowNumber];
    //printf("row %d: %u\n", rowNumber, originalVal);
    //unsigned int bitMask = 1;

    //bitMask = bitMask << (31 - rowIndex);
    return (originalVal >> (31- rowIndex)) & 0x1;
}

void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

//debugging tools

void printBitMap(bitmap_t* bitmap, int rows)
{
    int bitBlockNumber = rows / 1024;
    printf("bit blocknumber %d\n", bitBlockNumber);
    
    for(int i = 0; i < rows; i++)
    {
        printf("index %d\n", i%1024);
        printf("row %d:  %u\n", i, bitmap[bitBlockNumber].bits[i%1024]);
    }
}
    


 
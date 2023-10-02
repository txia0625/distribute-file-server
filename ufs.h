#ifndef __ufs_h__
#define __ufs_h__

#define UFS_DIRECTORY (0)
#define UFS_REGULAR_FILE (1)

#define UFS_BLOCK_SIZE (4096)
#define BUFFER_SIZE (4096)

#define DIRECT_PTRS (30)

typedef struct {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    unsigned int direct[DIRECT_PTRS];
} inode_t;

typedef struct {
    char name[28];  // up to 28 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} dir_ent_t;

typedef struct{
    int msgType; //0- init,  1-lookup, 2-stat, 3-write, 4-read, 5-creat, 6-unlink, 7-shutdown
    char buffer[4096];  //msg content
    int inumber;    //a parameter for inumber when need it
    int offset;     //offset paramter when need it 
    int bytes;  //bytes write/read
    int fileType;   //indicate what file it is when create a file
}msg_t;

// presumed: block 0 is the super block
typedef struct __super {
    int inode_bitmap_addr; // block address (in blocks)
    int inode_bitmap_len;  // in blocks
    int data_bitmap_addr;  // block address (in blocks)
    int data_bitmap_len;   // in blocks
    int inode_region_addr; // block address (in blocks)
    int inode_region_len;  // in blocks
    int data_region_addr;  // block address (in blocks)
    int data_region_len;   // in blocks
    int num_inodes;        // just the number of inodes
    int num_data;          // and data blocks...
} super_t;

typedef struct {
unsigned int bits[UFS_BLOCK_SIZE / sizeof(unsigned int)];
} bitmap_t;

typedef struct {
dir_ent_t entries[128];
} dir_block_t;

typedef struct {
inode_t inodes[UFS_BLOCK_SIZE / sizeof(inode_t)];
} inode_block;

typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t {
    char name[28];  // up to 28 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;



#endif // __ufs_h__
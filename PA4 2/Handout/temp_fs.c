//                                  بِسْمِ اللهِ الرَّحْمٰنِ الرَّحِيْمِ

/**
 * @file fs.c
 * @author Sooms (24100180@lums.edu.pk)
 * @brief
 * @version 0.1
 * @date 2023-11-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <math.h>
#include <string.h>

#include "fs.h"

static int MOUNT_FLAG = 0;
static int NUM_OF_BLOCKS = 0;
static int NUM_OF_INODES = 0;
static int NUM_OF_DNODES = 0;

static union block SUPERBLOCK;
static union block BLOCK_BITMAP;
static union block INODE_BITMAP;

struct bitmapSearchReply
{
    int nodeNumber;
    int blockNumber;
};

// pass 1 for Inode ; 0 for Dnode ; != -1 => success else no available block
int getFirstFreeBlock(int choice)
{
    union block bitMapBlock;
    int bitMapResults = disk_read((choice + 1), &bitMapBlock);
    for (int i = 0; i < FLAGS_PER_BLOCK; i++)
    {
        if (bitMapBlock.bitmap[i] == 0)
        {
            return i;
        }
    }
    return -1;
}

// pass 1 for Inode ; 0 for Dnode
void updateBitMaps(int choice, int blockNumber)
{
    union block bitMapBlock;
    int bitmapOffset = 1;
    int bitMapResults = disk_read((choice + bitmapOffset), &bitMapBlock);
    bitMapBlock.bitmap[blockNumber] = 1;
    bitMapResults = disk_write((choice + bitmapOffset), &bitMapBlock);
}

// extract the relevant block number

int fs_format()
{

    NUM_OF_BLOCKS = disk_size();

    // initializing the disk :

    for (int i = 0; i < NUM_OF_BLOCKS; i++)
    {

        char block[BLOCK_SIZE];
        for (int j = 0; j < BLOCK_SIZE; i++)
        {
            block[i] = 0;
        }
        int result = disk_write(i, block);

        if (result != 0)
        {
            printf("Error in the initializing the disk :: \n");
            return;
        }
    }

    // the number of the INODes :
    NUM_OF_INODES = (NUM_OF_BLOCKS * INODE_SIZE) / BLOCK_SIZE;
    NUM_OF_DNODES = NUM_OF_BLOCKS - 1 - NUM_OF_INODES - 2;

    // Creating the superBlock :
    union block superblockUnion;
    superblockUnion.superblock.s_blocks_count = NUM_OF_BLOCKS;
    superblockUnion.superblock.s_inode_bitmap = 2;
    superblockUnion.superblock.s_block_bitmap = 1;
    superblockUnion.superblock.s_blocks_count = NUM_OF_DNODES;
    superblockUnion.superblock.s_inodes_count = NUM_OF_INODES;
    superblockUnion.superblock.s_inode_table_block_start = 3;

    disk_write(0, &superblockUnion.data);

    // initializing the bitmap blocks of DNODE and INODE :

    union block bitmapBlock;

    for (int j = 0; j < FLAGS_PER_BLOCK; j++)
    {
        bitmapBlock.bitmap[j] = 0;
    }

    disk_write(1, &bitmapBlock);
    disk_write(2, &bitmapBlock);

    // initialize the INODES in the block :

    struct inode initialINODE = {
        .i_size = 0,
        .i_is_directory = 0,
        .i_direct_pointers = {0}, // initializes all elements to 0 (NULL)
        .i_single_indirect_pointer = 0,
        .i_double_indirect_pointer = 0};

    for (int j = 0; j < NUM_OF_INODES; j++)
    {
        union block initialINODES;
        for (int i = 0; i < INODES_PER_BLOCK; i++)
        {
            initialINODES.inodes[i] = initialINODE;
        }
        disk_write((3 + j), &initialINODES);
    }

    // Setting up the root INODE :
    int firstFreeInode = getFirstFreeBlock(1);
    int firstFreeDnode = getFirstFreeBlock(0);

    int blockNumber = (firstFreeInode / INODES_PER_BLOCK) + 3;
    int blockOffset = (firstFreeInode % INODES_PER_BLOCK);
    union block iNodeBlock;
    int results = disk_read(blockNumber, &iNodeBlock);

    // creating the root iNode :

    iNodeBlock.inodes[blockOffset].i_is_directory = 1;
    iNodeBlock.inodes[blockOffset].i_size = 0;
    iNodeBlock.inodes[blockOffset].i_direct_pointers[0] = firstFreeDnode;

    // update the disk now after the Inode entry :
    results = disk_write(blockNumber, &iNodeBlock);

    // update the bitmaps correspondindly
    updateBitMaps(1, firstFreeInode);
    updateBitMaps(0, firstFreeDnode);
}

int fs_mount()
{
    // Initializing the variables :
    disk_read(0, &SUPERBLOCK);
    disk_read(1, &BLOCK_BITMAP);
    disk_read(2, &INODE_BITMAP);
    MOUNT_FLAG = 1;
    return 0;
}

// following functions are required for the creating a file/directory:

// will create directory or file as needed in the current directory and update the relevant info
int createFile(union block *parentDataBlock, char *dirName, int is_directory, int parentDataBlockNumber)
{
    int firstFreeInode = getFirstFreeBlock(1);
    int firstFreeDnode = getFirstFreeBlock(0);
    int blockNumber = (firstFreeInode / INODES_PER_BLOCK) + 3;
    int blockOffset = (firstFreeInode % INODES_PER_BLOCK);

    // reading the FreeInode :
    union block iNodeBlock;
    int results = disk_read(blockNumber, &iNodeBlock);

    // populating the iNode :
    iNodeBlock.inodes[blockOffset].i_is_directory = is_directory;
    iNodeBlock.inodes[blockOffset].i_size = 0;
    if (is_directory)
    {
        iNodeBlock.inodes[blockOffset].i_direct_pointers[0] = firstFreeDnode;
    }

    // updating the disk with the newly added values ::
    results = disk_write(blockNumber, &iNodeBlock);

    // updating the bitMaps :
    updateBitMaps(1, firstFreeInode);
    if (is_directory)
    {
        updateBitMaps(0, firstFreeDnode);
    }

    // updating the parent directory data node appropriately ::

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        // perform the strcmp here ::
        if (parentDataBlock->directory_block.entries[i].name == '0')
        {
            // this is the block we need:
            strcpy(parentDataBlock->directory_block.entries[i].name, dirName);
            parentDataBlock->directory_block.entries[i].inode_number = firstFreeInode;
        }
    }

    // update the disk
    results = disk_write(parentDataBlockNumber, parentDataBlock);

    // update the size somehow ::
}

// function to parse the string based on the folders :

char **parsePath(const char *path, int *count)
{
    char *copy = strdup(path); // Create a copy to avoid modifying the original string
    if (copy == NULL)
    {
        // Handle memory allocation failure
        return NULL;
    }

    // Count the number of folders
    *count = 0;
    char *token = strtok(copy, "/");
    while (token != NULL)
    {
        (*count)++;
        token = strtok(NULL, "/");
    }

    // Allocate memory for the array of strings
    char **folders = (char **)malloc(*count * sizeof(char *));
    if (folders == NULL)
    {
        // Handle memory allocation failure
        free(copy);
        return NULL;
    }

    // Copy each folder name into the array
    token = strtok(path, "/");
    for (int i = 0; i < *count; ++i)
    {
        folders[i] = strdup(token);
        if (folders[i] == NULL)
        {
            // Handle memory allocation failure
            for (int j = 0; j < i; ++j)
            {
                free(folders[j]);
            }
            free(folders);
            free(copy);
            return NULL;
        }
        token = strtok(NULL, "/");
    }

    free(copy); // Free the copied string, as it's no longer needed
    return folders;
}

void freePathArray(char **folders, int count)
{
    for (int i = 0; i < count; ++i)
    {
        free(folders[i]);
    }
    free(folders);
}

// returns -1 if not found else returns the block number :
int searchDirTable(char *dirName, int directoryTableBlockNumb)
{
    // read the directory table block:
    union block dirBlock;
    disk_read(directoryTableBlockNumb, &dirBlock);

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        // use strcmp here
        if (dirBlock.directory_block.entries[i].name == dirName)
        {
            return dirBlock.directory_block.entries[i].inode_number;
        }
    }
    return -1;
}

// function to recurse :

int recurseFileCreate(char **folders, int is_directory, union block *parentBlockNode, int count, int currentfolderIndex, int parentInodeNumber)
{
    // STOPPPPING CONDITION WHEN THE CURRENTFOLDER INDEX == COUNT !!!

    // search for file inside this iNode :
    // Looking at the direct tables :
    for (int i = 0; i < INODE_DIRECT_POINTERS; i++)
    {
        if (parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i] == NULL)
        {
            continue;
        }
        int result = searchDirTable(folders[currentfolderIndex], parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);

        if (result != -1)
        {
            // means the directory exists :
            // go deeper into the function :
            union block newParentBlock;

            int blockNumber = (result / INODES_PER_BLOCK) + 3;
            int blockOffset = (result % INODES_PER_BLOCK);
            disk_read(blockNumber, &newParentBlock);
            recurseFileCreate(folders, is_directory, &newParentBlock, count, currentfolderIndex + 1, blockOffset);
        }
    }

    // time to search in the first in-direct folder

    int singleInDirectBlockNumber = parentBlockNode->inodes[parentInodeNumber].i_single_indirect_pointer;

    // read the block

    union block singleInDirectBlock;
    disk_read(singleInDirectBlockNumber, &singleInDirectBlock);

    for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
    {
        if (singleInDirectBlock.pointers[i] > 1)
        {
            // then perform the normal search as singleInDirectBlock.pointers[i] has the appropriate block number for the table
            int result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[i]);
            if (result != -1)
            {
                union block newParentBlock;
                int blockNumber = (result / INODES_PER_BLOCK) + 3;
                int blockOffset = (result % INODES_PER_BLOCK);
                disk_read(blockNumber, &newParentBlock);
                recurseFileCreate(folders, is_directory, &newParentBlock, count, currentfolderIndex + 1, blockOffset);
            }
        }
    }

    // now performing the same for the diouble indirect block
    int doubleInDirectBlockNumber = parentBlockNode->inodes[parentInodeNumber].i_double_indirect_pointer;
    union block doubleInDirectBlock;
    disk_read(doubleInDirectBlockNumber, &doubleInDirectBlock);

    for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
    {
        singleInDirectBlockNumber = doubleInDirectBlock.pointers[i];
        singleInDirectBlock;
        disk_read(singleInDirectBlockNumber, &singleInDirectBlock);

        for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            if (singleInDirectBlock.pointers[i] > 1)
            {
                // then perform the normal search as singleInDirectBlock.pointers[i] has the appropriate block number for the table
                int result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[i]);
                if (result != -1)
                {
                    union block newParentBlock;
                    int blockNumber = (result / INODES_PER_BLOCK) + 3;
                    int blockOffset = (result % INODES_PER_BLOCK);
                    disk_read(blockNumber, &newParentBlock);
                    recurseFileCreate(folders, is_directory, &newParentBlock, count, currentfolderIndex + 1, blockOffset);
                }
            }
        }
    }
}

int fs_create(char *path, int is_directory)
{

    // parse the string to get the array of the folders :
    int count;
    char **folders = parsePath(path, &count);
    // extracting the root directory
    union block parentBlockNode;
    disk_read(0, &parentBlockNode);
    recurseFileCreate(folders, is_directory, &parentBlockNode, count, 0, 0);

    return 0;
}

int fs_remove(char *path)
{
    return 0;
}

int fs_read(char *path, void *buf, size_t count, off_t offset)
{
    return 0;
}

int fs_write(char *path, void *buf, size_t count, off_t offset)
{
    return 0;
}

int fs_list(char *path)
{

    return 0;
}

void fs_stat()
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return;
    }

    printf("Superblock:\n");
    printf("    Blocks: %d\n", SUPERBLOCK.superblock.s_blocks_count);
    printf("    Inodes: %d\n", SUPERBLOCK.superblock.s_inodes_count);
    printf("    Inode Table Block Start: %d\n", SUPERBLOCK.superblock.s_inode_table_block_start);
    printf("    Data Blocks Start: %d\n", SUPERBLOCK.superblock.s_data_blocks_start);
}

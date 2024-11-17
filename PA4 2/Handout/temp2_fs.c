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
            bitMapBlock.bitmap[i] = 1;
            disk_write((choice + 1), &bitMapBlock);
            if (choice == 1)
            {
                return i;
            }
            else
            {
                return i + 3 + NUM_OF_INODES;
            }
        }
    }
    return -1;
}

// just pass it the block numbers as given by the getFirstFreeBlock function
// pass 1 for Inode ; 0 for Dnode
void updateBitMaps(int choice, int blockNumber)
{

    printf("\n\t the choice is %d and Block Number is : %d", choice, blockNumber);
    blockNumber = blockNumber - (3 + NUM_OF_INODES);
    if (choice == 0)
    {
        blockNumber = blockNumber - 3 - NUM_OF_INODES;
    }
    union block bitMapBlock;
    int bitmapOffset = 1;
    printf("the blockNumber is : %d ", blockNumber);
    for (int i = 0; i < FLAGS_PER_BLOCK; i++)
    {
        // printf("value of i : %d \n", i);
        if (i == blockNumber)
        {
            printf("yesss!\n");
            int bitMapResults = disk_read((choice + bitmapOffset), &bitMapBlock.data);
            bitMapBlock.bitmap[i] = 1;
            bitMapResults = disk_write((choice + bitmapOffset), &bitMapBlock.data);
        }
    }

    // checking whether the updates are stored or not :
    union block bitMapBlock2;
    int results = disk_read((choice + bitmapOffset), &bitMapBlock2);
    for (int i = 0; i < FLAGS_PER_BLOCK; i++)
    {
        if (bitMapBlock2.bitmap[i] == 1)
        {
            printf("\n\t update is stored successfullly %d \n", i);
        }
    }
}

// extract the relevant block number

int fs_format()
{

    NUM_OF_BLOCKS = disk_size();

    // initializing the disk :
    printf("Yes done!\n");

    for (int i = 0; i < NUM_OF_BLOCKS; i++)
    {

        char block[BLOCK_SIZE];
        for (int j = 0; j < BLOCK_SIZE; j++)
        {
            block[j] = 0;
        }
        int result = disk_write(i, block);

        if (result == -1)
        {
            printf("Error in the initializing the disk :: \n");
            return;
        }
        // printf("Yes done! %d \n", i);
    }

    printf("Yes done!\n");
    // the number of the INODes full blocks (this is not the same as the number of iNodes):
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

    // initializing the datablock as of now :
    union block dataBlock;
    results = disk_read(firstFreeDnode, &dataBlock);
    initializeEntryTable(&dataBlock);
    results = disk_write(firstFreeDnode, &dataBlock);

    // update the bitmaps correspondindly
    // updateBitMaps(1, firstFreeInode);
    // updateBitMaps(0, firstFreeDnode);
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

// will create directory or file as needed in the current directory and update the relevant info & returns the iNode number of the current folder according to bitmap
// parentDataBlock = entry table , dirName = folder name , parentDataBlockNumber = entry table block number
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
    union block dNodeBlock;
    disk_read(firstFreeDnode, &dNodeBlock);
    if (is_directory)
    {
        initializeEntryTable(&dNodeBlock);
    }
    disk_write(firstFreeDnode, &dNodeBlock);

    // updating the bitMaps :
    // updateBitMaps(1, firstFreeInode);
    // if (is_directory)
    // {
    //     updateBitMaps(0, firstFreeDnode);
    // }

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
    printf("the first free Inode is : %d \n", firstFreeInode);
    return firstFreeInode;

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

// prints the entries in the table , for debugging
void listTheEntries(int directoryTableBlockNumb)
{
    union block dirBlock;
    disk_read(directoryTableBlockNumb, &dirBlock);
    printf("\n\t ** Table entries are : ** \n");

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        printf("\t %s : %d \n", dirBlock.directory_block.entries[i].name, dirBlock.directory_block.entries[i].inode_number);
    }
}

// returns -1 if not found else returns the block number :
int searchDirTable(char *dirName, int directoryTableBlockNumb)
{
    // read the directory table block:
    union block dirBlock;
    disk_read(directoryTableBlockNumb, &dirBlock);
    printf("Dirname is %s\n", dirName);
    printf("the directorytable block number is : %d \n", directoryTableBlockNumb);

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        // use strcmp here
        if (strcmp(dirBlock.directory_block.entries[i].name, dirName) == 0)
        {
            printf("Yes found it !\n");
            return dirBlock.directory_block.entries[i].inode_number;
        }
    }
    printf("Unsucessful search ;-; \n");
    return -1;
}

// returns -1 if not found else returns the block number that is passed in the argument :
int searchDirTableFreeEntry(int directoryTableBlockNumb)
{
    // read the directory table block:
    union block dirBlock;
    disk_read(directoryTableBlockNumb, &dirBlock);

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        // use strcmp here
        // use strcmp here
        if (strcmp(dirBlock.directory_block.entries[i].name, "0") == 0)
        {
            printf("the directory table now is : %d \n", directoryTableBlockNumb);
            return directoryTableBlockNumb;
        }
        else
        {
            printf("the directory name is : %s ", dirBlock.directory_block.entries[i].name);
        }
    }
    return -1;
}

// choice == 0 => data Entry block , choice == 1 => indirect pointer block
int allocateDataEntryBlock(int choice)
{

    if (choice == 0)
    {
        int firstFreeDnode = getFirstFreeBlock(0);
        union block dataBlock;

        for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
        {
            strcpy(dataBlock.directory_block.entries[i].name, "0");
            dataBlock.directory_block.entries[i].inode_number = 0;
        }
        // updateBitMaps(0, firstFreeDnode);
        disk_write(firstFreeDnode, &dataBlock);

        return firstFreeDnode;
    }
    else
    {
        int firstFreeDnode = getFirstFreeBlock(0);
        union block dataBlock;
        for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            dataBlock.pointers[i] = 0;
        }
        // updateBitMaps(0, firstFreeDnode);
        disk_write(firstFreeDnode, &dataBlock);
        return firstFreeDnode;
    }
}

// provides the blockNumber from the INode number :
int convertItoB(int iNodeNumber)
{
    return (iNodeNumber / INODES_PER_BLOCK) + 3;
}

// returns datablock number which has free entry else returns -1 :

int findFreeSlot(union block *parentBlockNode, int parentInodeNumber)
{

    int parentBlockNumber = convertItoB(parentInodeNumber);
    int result = 0;

    int singleInDirectBlockNumber = parentBlockNode->inodes[parentInodeNumber].i_single_indirect_pointer;
    union block singleInDirectBlock;

    int doubleInDirectBlockNumber = parentBlockNode->inodes[parentInodeNumber].i_double_indirect_pointer;
    union block doubleInDirectBlock;

    // Searching the free block in the direct Blocks :
    for (int i = 0; i < INODE_DIRECT_POINTERS; i++)
    {
        if (parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i] == 0)
        {
            // make a function to allocate a free table entry here
            result = allocateDataEntryBlock(0);
            parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i] = parentBlockNode;
            printf("Nope : the entry block is : %d\n", parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);
            disk_write(parentBlockNumber, parentBlockNode);
            break;
            // continue;
        }
        else
        {
            printf("Yeah : the entry block is : %d\n", parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);
            result = searchDirTableFreeEntry(parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);
            break;
        }
    }

    // Searching the free block in the in-direct blocks :

    if (result == -1)
    {
        // checking if singleInDirect is unInitialized or not ::

        if (singleInDirectBlockNumber == 0)
        {
            // allocate a block of indirect nodes ::
            singleInDirectBlockNumber = allocateDataEntryBlock(1);
            parentBlockNode->inodes[parentInodeNumber].i_single_indirect_pointer = singleInDirectBlockNumber;
            disk_write(parentBlockNumber, parentBlockNode);
        }

        disk_read(singleInDirectBlockNumber, &singleInDirectBlock);

        for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            if (singleInDirectBlock.pointers[i] != 0)
            {
                result = searchDirTableFreeEntry(singleInDirectBlock.pointers[i]);
                printf("yeah!\n");
                break;
            }
            else
            {
                singleInDirectBlock.pointers[i] = allocateDataEntryBlock(0);
                disk_write(singleInDirectBlock.pointers[i], &singleInDirectBlock);
                result = singleInDirectBlock.pointers[i];
                break;
            }
        }
    }

    // Searching for the free block in the in-indirect block :

    if (result == -1)
    {
        if (doubleInDirectBlockNumber == 0)
        {
            doubleInDirectBlockNumber = allocateDataEntryBlock(1);
            parentBlockNode->inodes[parentInodeNumber].i_double_indirect_pointer = doubleInDirectBlockNumber;
            disk_write(parentBlockNumber, parentBlockNode);
        }

        disk_read(doubleInDirectBlockNumber, &doubleInDirectBlock);

        for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            singleInDirectBlockNumber = doubleInDirectBlock.pointers[i];
            if (singleInDirectBlockNumber == 0)
            {
                // allocate a block of indirect nodes ::
                singleInDirectBlockNumber = allocateDataEntryBlock(1);
                doubleInDirectBlock.pointers[i] = singleInDirectBlockNumber;
                disk_write(doubleInDirectBlockNumber, &doubleInDirectBlock);
            }

            disk_read(singleInDirectBlockNumber, &singleInDirectBlock);

            for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
            {
                if (singleInDirectBlock.pointers[i] > 1)
                {
                    result = searchDirTableFreeEntry(singleInDirectBlock.pointers[i]);
                }
                else
                {
                    singleInDirectBlock.pointers[i] = allocateDataEntryBlock(0);
                    disk_write(singleInDirectBlock.pointers[i], &singleInDirectBlock);
                    result = singleInDirectBlock.pointers[i];
                    break;
                }
            }
        }
    }

    if (result == -1)
    {
        printf("NO FREE BLOCK HAS BEEN FOUND, SOMETHING IS INCORRECT IN THE LOGIC \n");
        return -1;
    }
    else
    {
        printf("The free slot for this directory is : %d", result);
        return result;
    }
}

// function to inialize the entries of the table entry data :
void initializeEntryTable(union block *dataEntryBlock)
{
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        strcpy(dataEntryBlock->directory_block.entries[i].name, "0");
        dataEntryBlock->directory_block.entries[i].inode_number = 0;
    }
}

// function to recurse :
// requires : folders = array of directories , parentBlockNode = parent block which has the iNode of the parent , count = max directories in path , totalParentInodeNumber according to bitmap
int recurseFileCreate(char **folders, int is_directory, union block *parentBlockNode, int count, int currentfolderIndex, int totalParentInodeNumber)
{
    printf("Parent Directory INODE number : %d \n", totalParentInodeNumber);
    int parentInodeNumber = (totalParentInodeNumber % INODES_PER_BLOCK);

    int result = -1;

    int singleInDirectBlockNumber = parentBlockNode->inodes[parentInodeNumber].i_single_indirect_pointer;
    union block singleInDirectBlock;

    int doubleInDirectBlockNumber = parentBlockNode->inodes[parentInodeNumber].i_double_indirect_pointer;
    union block doubleInDirectBlock;

    for (int i = 0; i < INODE_DIRECT_POINTERS; i++)
    {
        if (parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i] == 0) // checking if they are empty
        {
            continue;
        }
        else
        {
            printf("YES!\n");
            result = searchDirTable(folders[currentfolderIndex], parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);
        }
    }

    // time to search in the first in-direct folder

    if (result == -1)
    {
        if (singleInDirectBlockNumber != 0)
        {
            disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
            for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
            {
                if (singleInDirectBlock.pointers[i] > 1)
                {
                    printf("YES!\n");
                    result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[i]);
                }
            }
        }
    }

    // time to search the second in-direct folder
    if (result == -1)
    {

        // condition to see if the Second indirect node is initialised or not :
        disk_read(doubleInDirectBlockNumber, &doubleInDirectBlock);

        if (doubleInDirectBlockNumber != 0)
        {
            for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
            {
                singleInDirectBlockNumber = doubleInDirectBlock.pointers[i];

                if (singleInDirectBlockNumber != 0)
                {
                    printf("Yes ! : single block number : %d \n", singleInDirectBlockNumber);
                    disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
                    for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
                    {
                        if (singleInDirectBlock.pointers[i] > 1)
                        {
                            result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[i]);
                        }
                    }
                }
            }
        }
    }

    // after full iteration ::
    if (result != -1)
    {
        union block newParentBlock;
        int blockNumber = (result / INODES_PER_BLOCK) + 3;
        int blockOffset = (result % INODES_PER_BLOCK);
        disk_read(blockNumber, &newParentBlock);

        // Stopping condition :
        // --> check this condition please !
        if (currentfolderIndex + 1 == count)
        {
            printf("probably a logical mistake....\n");
            return;
        }

        recurseFileCreate(folders, is_directory, &newParentBlock, count, currentfolderIndex + 1, result);
    }
    else
    {
        printf("Yes in the second condition of the recursion\n");
        int freeBlock = findFreeSlot(parentBlockNode, parentInodeNumber);
        int currentFreeInode = createFile(parentBlockNode, folders[currentfolderIndex], is_directory, parentInodeNumber);

        // accessing the block :
        int currentBlockNumber = convertItoB(currentFreeInode);
        union block currentBlock;
        disk_read(currentBlockNumber, &currentBlock);
        // Stopping condition :
        // --> check this condition please !
        if (currentfolderIndex + 1 == count)
        {
            printf("probably a correct return....\n");
            return;
        }
        // performing the same recursion again ::
        recurseFileCreate(folders, is_directory, &currentBlock, count, currentfolderIndex + 1, currentFreeInode);
    }
}

// function to print the directories :
void printFolders(char **folders, int count)
{
    printf("Number of folders: %d\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("Folder %d: %s\n", i + 1, folders[i]);
    }
}

int fs_create(char *path, int is_directory)
{

    // parse the string to get the array of the folders :
    int count;
    char **folders = parsePath(path, &count);
    printFolders(folders, count);
    // extracting the root directory
    union block parentBlockNode;
    disk_read(3, &parentBlockNode);
    printf("Recursion 1 go!\n");
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
    fs_create(path, 1);

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

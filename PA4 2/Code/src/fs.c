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

    // printf("\n\t the choice is %d and Block Number is : %d", choice, blockNumber);
    blockNumber = blockNumber - (3 + NUM_OF_INODES);
    if (choice == 0)
    {
        blockNumber = blockNumber - 3 - NUM_OF_INODES;
    }
    union block bitMapBlock;
    int bitmapOffset = 1;
    // printf("the blockNumber is : %d ", blockNumber);
    for (int i = 0; i < FLAGS_PER_BLOCK; i++)
    {
        // printf("value of i : %d \n", i);
        if (i == blockNumber)
        {
            // printf("yesss!\n");
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
            continue;
            // printf("\n\t update is stored successfullly %d \n", i);
        }
    }
}

// extract the relevant block number

int fs_format()
{
    if (MOUNT_FLAG)
    {
        return -1;
    }
    NUM_OF_BLOCKS = disk_size();

    // initializing the disk :
    // printf("Yes done!\n");

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

    // printf("Yes done!\n");
    //  the number of the INODes full blocks (this is not the same as the number of iNodes):
    NUM_OF_INODES = ((NUM_OF_BLOCKS * INODE_SIZE) / BLOCK_SIZE);
    if (((NUM_OF_BLOCKS * INODE_SIZE) % BLOCK_SIZE) != 0)
    {
        NUM_OF_INODES = NUM_OF_INODES + 1;
    }
    NUM_OF_DNODES = NUM_OF_BLOCKS - 1 - NUM_OF_INODES - 2;

    // Creating the superBlock :
    union block superblockUnion;
    superblockUnion.superblock.s_blocks_count = NUM_OF_BLOCKS;
    superblockUnion.superblock.s_inode_bitmap = 2;
    superblockUnion.superblock.s_block_bitmap = 1;
    // superblockUnion.superblock.s_blocks_count = NUM_OF_DNODES;
    superblockUnion.superblock.s_inodes_count = NUM_OF_BLOCKS;
    superblockUnion.superblock.s_inode_table_block_start = 3;
    superblockUnion.superblock.s_data_blocks_start = 3 + NUM_OF_INODES;

    // printf("s_blocks_count: %d\n", superblockUnion.superblock.s_blocks_count);
    // printf("s_inode_bitmap: %d\n", superblockUnion.superblock.s_inode_bitmap);
    // printf("s_block_bitmap: %d\n", superblockUnion.superblock.s_block_bitmap);
    // printf("s_inodes_count: %d\n", superblockUnion.superblock.s_inodes_count);
    // printf("s_inode_table_block_start: %d\n", superblockUnion.superblock.s_inode_table_block_start);
    // printf("s_data_blocks_start: %d\n", superblockUnion.superblock.s_data_blocks_start);

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
    iNodeBlock.inodes[blockOffset].i_size = BLOCK_SIZE;
    iNodeBlock.inodes[blockOffset].i_direct_pointers[0] = firstFreeDnode;

    // printf("the first root datablock table that is given to me is %d ......... \n", firstFreeDnode);

    // update the disk now after the Inode entry :
    results = disk_write(blockNumber, &iNodeBlock);

    // initializing the datablock as of now :
    union block dataBlock;
    results = disk_read(firstFreeDnode, &dataBlock);
    initializeEntryTable(&dataBlock);
    results = disk_write(firstFreeDnode, &dataBlock);

    return 0;
    // update the bitmaps correspondindly
    // updateBitMaps(1, firstFreeInode);
    // updateBitMaps(0, firstFreeDnode);
}

int fs_mount()
{
    // Initializing the variables :
    if (MOUNT_FLAG)
    {
        return -1;
    }
    disk_read(0, &SUPERBLOCK);
    disk_read(1, &BLOCK_BITMAP);
    disk_read(2, &INODE_BITMAP);
    MOUNT_FLAG = 1;
    return 0;
}

void fs_unmount()
{
    if (MOUNT_FLAG == 0)
    {
        printf("Error: Disk is not mounted.\n");
        return;
    }
    // Set the mount flag to 0
    MOUNT_FLAG = 0;
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

    // populating the iNode according to the property:
    iNodeBlock.inodes[blockOffset].i_is_directory = is_directory;
    iNodeBlock.inodes[blockOffset].i_size = 0;
    // initializing the direct pointers :
    for (int i = 0; i < INODE_DIRECT_POINTERS; i++)
    {
        iNodeBlock.inodes[blockOffset].i_direct_pointers[i] = 0;
    }
    iNodeBlock.inodes[blockOffset].i_single_indirect_pointer = 0;
    iNodeBlock.inodes[blockOffset].i_double_indirect_pointer = 0;

    if (is_directory)
    {
        iNodeBlock.inodes[blockOffset].i_direct_pointers[0] = firstFreeDnode;
        iNodeBlock.inodes[blockOffset].i_size = BLOCK_SIZE;
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

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        // perform the strcmp here ::
        // perform the strcmp here
        if (strcmp(parentDataBlock->directory_block.entries[i].name, "0") == 0)
        {
            // this is the block we need:
            strcpy(parentDataBlock->directory_block.entries[i].name, dirName);
            parentDataBlock->directory_block.entries[i].inode_number = firstFreeInode;
            break;
        }
    }
    // update the disk
    results = disk_write(parentDataBlockNumber, parentDataBlock);
    // listTheEntries(parentDataBlockNumber);
    // printf("the first free Inode is : %d \n", firstFreeInode);
    return firstFreeInode;

    // update the size somehow ::
}

// function to parse the string based on the folders :
char **parsePath(const char *path, int *count)
{
    char *copy = strdup(path);
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
    // printf("Dirname is %s & directory table block number is : %d \n", dirName, directoryTableBlockNumb);
    //  printf("the directorytable block number is : %d \n", directoryTableBlockNumb);

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        // use strcmp here
        if (strcmp(dirBlock.directory_block.entries[i].name, dirName) == 0)
        {
            // printf("Yes found it %s =  %d \n", dirBlock.directory_block.entries[i].name, dirBlock.directory_block.entries[i].inode_number);
            return dirBlock.directory_block.entries[i].inode_number;
        }
    }
    // printf("Unsucessful search ;-; \n");
    return -1;
}

// this will check whether the type is same or not ?
int searchDirTable2(char *dirName, int directoryTableBlockNumb, int is_directory)
{
    // read the directory table block:
    union block dirBlock;
    disk_read(directoryTableBlockNumb, &dirBlock);
    // printf("Dirname is %s & directory table block number is : %d \n", dirName, directoryTableBlockNumb);
    //  printf("the directorytable block number is : %d \n", directoryTableBlockNumb);

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        // use strcmp here
        if (strcmp(dirBlock.directory_block.entries[i].name, dirName) == 0)
        {
            // printf("Yes found it %s =  %d \n", dirBlock.directory_block.entries[i].name, dirBlock.directory_block.entries[i].inode_number);
            // checking the types :
            union block tempInode;
            disk_read(convertItoB(dirBlock.directory_block.entries[i].inode_number), &tempInode);
            if (tempInode.inodes[convertItoBOffset(dirBlock.directory_block.entries[i].inode_number)].i_is_directory == is_directory)
            {
                // printf("Yes found it %s =  %d \n", dirBlock.directory_block.entries[i].name, dirBlock.directory_block.entries[i].inode_number);
                return dirBlock.directory_block.entries[i].inode_number;
            }
            else
            {
                continue;
            }
        }
    }
    // printf("Unsucessful search ;-; \n");
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
            // printf("the directory table now is : %d \n", directoryTableBlockNumb);
            return directoryTableBlockNumb;
        }
        else
        {
            continue;
            // printf("the directory name is : %s ", dirBlock.directory_block.entries[i].name);
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
        // printf("THE FIRST ALLOCATED FREE D-NODE %d", firstFreeDnode);
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
        // printf("THE FIRST ALLOCATED FREE D-NODE %d", firstFreeDnode);
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

// provides the offset of the raw INode number :
int convertItoBOffset(int iNodeNumber)
{
    return (iNodeNumber % INODES_PER_BLOCK);
}

// returns datablock number which has free entry else returns -1 :
// searches for a free slot in parent directory where the current node can be inserted. returns the table block which has the free entry slot.
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
            // printf("Nope : the entry block is : %d\n", parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);
            disk_write(parentBlockNumber, parentBlockNode);
            break;
            // continue;
        }
        else
        {
            // printf("Yeah : the entry block is : %d\n", parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);
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
        // printf("The free slot for this directory is : %d", result);
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

// the following function will just update the size of the file when created/written by fs_write
int updateSize(char **folders, union block *parentBlockNode, int count, int currentfolderIndex, int totalParentInodeNumber, int newSize)
{
    // printf("\t\n UPDATING THE SIZE FOR INODE %d : \n", totalParentInodeNumber);
    // printf("FOLDER TO HAVE SIZE INCREASED %s /%d \n", folders[currentfolderIndex], count);

    // this is offset
    int parentInodeNumber = (totalParentInodeNumber % INODES_PER_BLOCK);

    int targetDataEntryBlock = 0;
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
            // printf("YES!\n");
            result = searchDirTable(folders[currentfolderIndex], parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);
            // printf("RESULT ACHIEVED ARE %d: \n", result);
            if (result != -1)
            {
                targetDataEntryBlock = parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i];
            }
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
                    // printf("YES!\n");
                    result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[i]);
                    if (result != -1)
                    {
                        targetDataEntryBlock = singleInDirectBlock.pointers[i];
                    }
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
                    disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
                    for (int j = 0; j < INODE_INDIRECT_POINTERS_PER_BLOCK; j++)
                    {
                        if (singleInDirectBlock.pointers[j] > 1)
                        {
                            result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[j]);
                            if (result != -1)
                            {
                                targetDataEntryBlock = singleInDirectBlock.pointers[j];
                            }
                        }
                    }
                }
            }
        }
    }

    // after full iteration ::
    if (result != -1)
    {

        // Stopping condition :
        // --> check this condition please !

        union block locatedParentDataBlock;
        int blockNumber = (result / INODES_PER_BLOCK) + 3;
        int blockOffset = (result % INODES_PER_BLOCK);
        disk_read(blockNumber, &locatedParentDataBlock);
        parentBlockNode->inodes[parentInodeNumber].i_size = parentBlockNode->inodes[parentInodeNumber].i_size + newSize;

        // printf("\t\n UPDATED THE SIZE OF THE NODE AS %d  for BLOCK  : %d \n", parentBlockNode->inodes[parentInodeNumber].i_size, parentInodeNumber);
        //  saving this into the disk :

        // printf("SIZE has been updated to the dir : %s :%d : %ld \n", folders[currentfolderIndex], currentfolderIndex, parentBlockNode->inodes[parentInodeNumber].i_size);
        disk_write(convertItoB(totalParentInodeNumber), parentBlockNode);

        // fs_list("/");

        if (currentfolderIndex + 1 == count)
        {

            // printf("Successfully updated all the parent folders !\n");
            return 1;
        }
        // printf("passing down the line !\n");

        // updating the size of the parent node :

        updateSize(folders, &locatedParentDataBlock, count, currentfolderIndex + 1, result, newSize);
    }
    else
    {
        return 0;
    }
}

// function to recurse :
// requires : folders = array of directories , parentBlockNode = parent block which has the iNode of the parent , count = max directories in path , totalParentInodeNumber according to bitmap
int recurseFileCreate(char **folders, int is_directory, union block *parentBlockNode, int count, int currentfolderIndex, int totalParentInodeNumber)
{
    // printf("Parent Directory INODE number : %d \n", totalParentInodeNumber);
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
            // printf("YES!\n");
            int tempDir = 0;
            if ((currentfolderIndex + 1) != count)
            {
                tempDir = 1;
            }
            else
            {
                tempDir = is_directory;
            }
            result = searchDirTable2(folders[currentfolderIndex], parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i], tempDir);
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
                    // printf("YES!\n");
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
                    disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
                    for (int j = 0; j < INODE_INDIRECT_POINTERS_PER_BLOCK; j++)
                    {
                        if (singleInDirectBlock.pointers[j] > 1)
                        {
                            result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[j]);
                        }
                    }
                }
            }
        }
    }

    // after full iteration ::
    if (result != -1)
    {
        union block locatedParentDataBlock;
        int blockNumber = (result / INODES_PER_BLOCK) + 3;
        int blockOffset = (result % INODES_PER_BLOCK);
        disk_read(blockNumber, &locatedParentDataBlock);

        // Stopping condition :
        // --> check this condition please !
        if (currentfolderIndex + 1 == count)
        {
            printf("End directory reached....\n");
            return result;
        }
        recurseFileCreate(folders, is_directory, &locatedParentDataBlock, count, currentfolderIndex + 1, result);
    }
    else
    {
        // printf("Yes in the second condition of the recursion\n");
        int freeBlockNumber = findFreeSlot(parentBlockNode, parentInodeNumber);
        // printf("\t\n CURRENT FOLDER INDEX : %d \n", currentfolderIndex);
        union block freeBlock;
        disk_read(freeBlockNumber, &freeBlock);
        int currentFreeInode = 0;
        if (currentfolderIndex + 1 == count)
        {
            currentFreeInode = createFile(&freeBlock, folders[currentfolderIndex], is_directory, freeBlockNumber);
            if (is_directory)
            {
                union block rootBlock;
                disk_read(3, &rootBlock);
                // updateSize(folders, &rootBlock, currentfolderIndex + 1, 0, 0, 4096);
                //  fs_list("/");
            }
        }
        else
        {
            currentFreeInode = createFile(&freeBlock, folders[currentfolderIndex], 1, freeBlockNumber);
            union block rootBlock;
            disk_read(3, &rootBlock);
            // updateSize(folders, &rootBlock, currentfolderIndex + 1, 0, 0, 4096);
            //  fs_list("/");
        }

        // accessing the block :
        int currentBlockNumber = convertItoB(currentFreeInode);
        union block currentBlock;
        disk_read(currentBlockNumber, &currentBlock);
        // Stopping condition :
        // --> check this condition please !
        if (currentfolderIndex + 1 == count)
        {
            // printf("probably a correct return....\n");
            return currentFreeInode;
        }
        // performing the same recursion again ::
        // printf("ABOUT TO PERFORM NEW RECURSIONS \n");
        // fs_list("/");
        recurseFileCreate(folders, is_directory, &currentBlock, count, currentfolderIndex + 1, currentFreeInode);
    }
}

// function to print the directories :
void printFolders(char **folders, int count)
{
    // printf("Number of folders: %d\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("Folder %d: %s\n", i + 1, folders[i]);
    }
}

int fs_create(char *path, int is_directory)
{
    if (MOUNT_FLAG == 0)
    {
        return -1;
    }
    // parse the string to get the array of the folders :
    int count;
    char **folders = parsePath(path, &count);
    // printFolders(folders, count);
    //  extracting the root directory
    union block parentBlockNode;
    disk_read(3, &parentBlockNode);
    // printf("Recursion 1 go!\n");
    int iNodeCreated = recurseFileCreate(folders, is_directory, &parentBlockNode, count, 0, 0);
    // fs_list("/");
    // printf("-----\n");
    // fs_list("/foo");
    // printf("-----\n");
    return iNodeCreated;
}

// following is a search function that will use the path given to provide :
// 1) last directory/file iNodeNumber , 2) TableEntry Data block's number , 3) Parent's iNodeNumber

struct searchNodesReply searchFile(char **folders, union block *parentBlockNode, int count, int currentfolderIndex, int totalParentInodeNumber)
{

    int parentInodeNumber = (totalParentInodeNumber % INODES_PER_BLOCK);

    int targetDataEntryBlock = 0;
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
            // printf("YES!\n");
            result = searchDirTable(folders[currentfolderIndex], parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i]);
            if (result != -1)
            {
                targetDataEntryBlock = parentBlockNode->inodes[parentInodeNumber].i_direct_pointers[i];
            }
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
                    // printf("YES!\n");
                    result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[i]);
                    if (result != -1)
                    {
                        targetDataEntryBlock = singleInDirectBlock.pointers[i];
                    }
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
                    disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
                    for (int j = 0; j < INODE_INDIRECT_POINTERS_PER_BLOCK; j++)
                    {
                        if (singleInDirectBlock.pointers[j] > 1)
                        {
                            result = searchDirTable(folders[currentfolderIndex], singleInDirectBlock.pointers[j]);
                            if (result != -1)
                            {
                                targetDataEntryBlock = singleInDirectBlock.pointers[j];
                            }
                        }
                    }
                }
            }
        }
    }

    // after full iteration ::
    if (result != -1)
    {

        // Stopping condition :
        // --> check this condition please !

        union block locatedParentDataBlock;
        int blockNumber = (result / INODES_PER_BLOCK) + 3;
        int blockOffset = (result % INODES_PER_BLOCK);
        disk_read(blockNumber, &locatedParentDataBlock);

        if (currentfolderIndex + 1 == count)
        {

            struct searchNodesReply finalReply;
            finalReply.currentInodeNumber = result;
            finalReply.parentInodeNumber = totalParentInodeNumber;
            finalReply.tableEntryNumber = targetDataEntryBlock;

            return finalReply;
        }
        // recurseFileCreate(folders, is_directory, &locatedParentDataBlock, count, currentfolderIndex + 1, result);
        printf("passing down the line !\n");
        searchFile(folders, &locatedParentDataBlock, count, currentfolderIndex + 1, result);
    }
    else
    {
        struct searchNodesReply finalReply;
        finalReply.currentInodeNumber = result;
        finalReply.parentInodeNumber = -1;
        finalReply.tableEntryNumber = -1;
        return finalReply;
    }
}

// FOllowing functions are to search and remove a folder from directory :

// will get the block number and the choice & according to the choice it will update the bitmap as 0
// choice = 0 -> dNode ; choice 1 -> iNode ; iNodeNumber which is RAW
int removeBitmap(int choice, int NodeNumber)
{
    union block bitMapBlock;
    int bitMapResults = disk_read((choice + 1), &bitMapBlock);
    bitMapBlock.bitmap[NodeNumber] = 0;
    disk_write((choice + 1), &bitMapBlock);
}

// give it the raw(aka bitmap dNode and it will remove the folders and files in it)
int removeEntriesFromTable(int tableEntryNumber)
{
    union block currentEntryBlock;
    disk_read(tableEntryNumber, &currentEntryBlock);

    for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++)
    {
        // checking if there is entry in the table or not
        if (strcmp(currentEntryBlock.directory_block.entries[j].name, "0") == 0 || strcmp(currentEntryBlock.directory_block.entries[j].name, "") == 0)
        {
            continue;
        }
        else
        {
            // calling the function recursively ....
            removeRecursively(currentEntryBlock.directory_block.entries[j].inode_number);
            strcpy(currentEntryBlock.directory_block.entries[j].name, "0");
        }
    }

    // delete this table block in the bitMap :
    removeBitmap(0, tableEntryNumber);
}

// following functions will remove the datablocks in the file's singleIndirect block:
int removeDataPointersFromTable(int dataBlockNumber)
{
    union block currentDataBlock;
    disk_read(dataBlockNumber, &currentDataBlock);

    for (int j = 0; j < INODE_INDIRECT_POINTERS_PER_BLOCK; j++)
    {
        // checking if there is entry in the table or not
        if (currentDataBlock.pointers[j] <= 0)
        {
            continue;
        }
        else
        {
            // calling the function recursively ....
            removeBitmap(0, currentDataBlock.pointers[j]);
        }
    }

    // delete this table block in the bitMap :
    removeBitmap(0, dataBlockNumber);
}

// this function will remove files in the folder and will recurse if there is another folder in this folder :

// pass it the iNodeNumber according to bitmap position
int removeRecursively(int iNodeNumber)
{

    // CORRECT THE INDEXING
    // CHECK THE LAST BITMAP
    // MAKE THE BITMAP FUNCTION
    // CHECK THE LOGIC

    // read this iNodeNumber block
    int blockNumber = convertItoB(iNodeNumber);
    int blockOffset = (iNodeNumber % INODES_PER_BLOCK);

    // reading the block and extract the iNode out of it  :
    union block currentBlock;
    disk_read(blockNumber, &currentBlock);

    // checking if the block is directory or not :
    if (currentBlock.inodes[blockOffset].i_is_directory) // means that its directory
    {
        // iterate over the direct block entry tables :
        // printf("YES it is DIRECTORY\n");
        for (int i = 0; i < INODE_DIRECT_POINTERS; i++)
        {
            if (currentBlock.inodes[blockOffset].i_direct_pointers[i] == 0) // checking if they are empty
            {
                continue;
            }
            else
            {
                // go to the table entry block -> iterate and if found any iNode , call this function recursively
                removeEntriesFromTable(currentBlock.inodes[blockOffset].i_direct_pointers[i]);
                currentBlock.inodes[blockOffset].i_direct_pointers[i] = 0;
            }
        }

        // itrate over the first indirect entry tables :

        int singleInDirectBlockNumber = currentBlock.inodes[blockOffset].i_single_indirect_pointer;
        union block singleInDirectBlock;
        disk_read(singleInDirectBlockNumber, &singleInDirectBlock);

        if (singleInDirectBlockNumber != 0)
        {
            disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
            for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
            {
                if (singleInDirectBlock.pointers[i] > 0)
                {
                    // go to the entries of this table block now
                    removeEntriesFromTable(singleInDirectBlock.pointers[i]);
                    singleInDirectBlock.pointers[i] = 0;
                }
            }
            removeBitmap(0, singleInDirectBlockNumber);
        }

        // iterate over the second indirect entries tables :

        int doubleInDirectBlockNumber = currentBlock.inodes[blockOffset].i_double_indirect_pointer;
        union block doubleInDirectBlock;

        if (doubleInDirectBlockNumber != 0)
        {
            disk_read(doubleInDirectBlockNumber, &doubleInDirectBlock);
            for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
            {
                singleInDirectBlockNumber = doubleInDirectBlock.pointers[i];
                if (singleInDirectBlockNumber != 0)
                {
                    disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
                    for (int j = 0; j < INODE_INDIRECT_POINTERS_PER_BLOCK; j++)
                    {
                        if (singleInDirectBlock.pointers[j] > 1)
                        {
                            removeEntriesFromTable(singleInDirectBlock.pointers[j]);
                        }
                    }
                    removeBitmap(0, singleInDirectBlockNumber);
                }
            }
            removeBitmap(0, doubleInDirectBlockNumber);
        }
    }

    else
    {
        // loop over the direct data blocks and update the bitmaps :

        for (int i = 0; i < INODE_DIRECT_POINTERS; i++)
        {
            if (currentBlock.inodes[blockOffset].i_direct_pointers[i] == 0) // checking if they are empty
            {
                continue;
            }
            else
            {
                // go to the table entry block -> iterate and if found any iNode , call this function recursively
                removeBitmap(0, currentBlock.inodes[blockOffset].i_direct_pointers[i]);
            }
        }

        // loops over the single indirect node :
        int singleInDirectBlockNumber = currentBlock.inodes[blockOffset].i_single_indirect_pointer;
        union block singleInDirectBlock;
        disk_read(singleInDirectBlockNumber, &singleInDirectBlock);

        if (singleInDirectBlockNumber != 0)
        {
            // disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
            removeDataPointersFromTable(singleInDirectBlockNumber);
        }

        // loop over the double indirect node :

        int doubleInDirectBlockNumber = currentBlock.inodes[blockOffset].i_double_indirect_pointer;
        union block doubleInDirectBlock;
        if (doubleInDirectBlockNumber != 0)
        {
            disk_read(doubleInDirectBlockNumber, &doubleInDirectBlock);
            for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
            {
                singleInDirectBlockNumber = doubleInDirectBlock.pointers[i];
                if (singleInDirectBlockNumber != 0)
                {
                    removeDataPointersFromTable(singleInDirectBlockNumber);
                }
            }
            removeBitmap(0, doubleInDirectBlockNumber);
        }
    }

    // removing the original block
    removeBitmap(1, iNodeNumber);
}

// this function will return the iNode number for the deleted directory in the path ::
int fs_remove(char *path)
{
    if (MOUNT_FLAG == 0)
    {
        return -1;
    }
    // getting the iNode of the block :
    int count;
    char **folders = parsePath(path, &count);
    printFolders(folders, count);
    // extracting the root directory
    union block parentBlockNode;
    disk_read(3, &parentBlockNode);

    //

    // 1) search if the iNode is present or not -> yes -> remove all of its folders etc -> update the entry in the table -> (for future, update the parentSize)

    struct searchNodesReply replyRecieved = searchFile(folders, &parentBlockNode, count, 0, 0);
    if (replyRecieved.currentInodeNumber != -1)
    {
        // printf("I got the reply as %d \n", replyRecieved.currentInodeNumber);
        removeRecursively(replyRecieved.currentInodeNumber);
        // updating the table entry block :
        union block tempTableBlock;
        disk_read(replyRecieved.tableEntryNumber, &tempTableBlock);
        for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {

            if (tempTableBlock.directory_block.entries[i].inode_number == replyRecieved.currentInodeNumber)
            {
                tempTableBlock.directory_block.entries[i].inode_number = 0;
                strcpy(tempTableBlock.directory_block.entries[i].name, "0");
                // saving this into the disk :
                disk_write(replyRecieved.tableEntryNumber, &tempTableBlock);
                break;
            }
        }
    }
    else
    {
        printf("Coulnt find the file in the directory !!! \n");
        return -1;
    }

    return 0;
}

// the following function will return the block number to start reading from in the file ...
int giveStartingBlock(int offset)
{
    return (offset / 4096);
}

// this will return the starting single Indirect block when calling the doubleIndirect block. Offset is the raw offset:
int giveStartingSingleIndirectBlock(int offset)
{
    return (((offset / 4096) - INODE_DIRECT_POINTERS - INODE_INDIRECT_POINTERS_PER_BLOCK) / INODE_INDIRECT_POINTERS_PER_BLOCK);
}

// this will return the current index for the SingleIndirect Block given the current Data block we are reading
int giveCurrentStartingIndirectBlock(int currentDataBlockNumber)
{
    return ((currentDataBlockNumber - INODE_DIRECT_POINTERS - INODE_INDIRECT_POINTERS_PER_BLOCK) / INODE_INDIRECT_POINTERS_PER_BLOCK);
}

// this will return the starting data block in the single Indirect Block, offset is the raw offset :
int giveStartingDataBlock(int offset)
{
    return (((offset / 4096) - INODE_DIRECT_POINTERS - INODE_INDIRECT_POINTERS_PER_BLOCK) % INODE_INDIRECT_POINTERS_PER_BLOCK);
}

// the following function will just update the size of the file

int fs_read(char *path, void *buf, size_t count, off_t offset)
{
    if (MOUNT_FLAG == 0)
    {
        return -1;
    }

    // gettting the iNode number of the block that we need :
    char *path2 = strdup(path);
    int count1;
    char **folders = parsePath(path2, &count1);

    union block parentBlockNode;
    disk_read(3, &parentBlockNode);

    union block currentBlock;
    struct searchNodesReply replyRecieved = searchFile(folders, &parentBlockNode, count1, 0, 0);
    // printf("the returned iNODE is : %d !!!\n", replyRecieved.currentInodeNumber);
    int iNodeOffset = convertItoBOffset(replyRecieved.currentInodeNumber);
    disk_read(convertItoB(replyRecieved.currentInodeNumber), &currentBlock);

    // we need to find the starting block to start reading from ...
    // assuming the node exists

    // 1) getting the starting block to start reading from :
    int startingBlock = giveStartingBlock(offset);
    int startByte = offset % 4096;
    uint8_t *dataArray = (uint8_t *)malloc(count * sizeof(uint8_t));
    int startIndexArray = 0;
    int x = startingBlock;
    int y = startByte;
    // printf("the value of Y = %d , X = %d \n", y, x);
    int limitFlag = 0; // 0 means data has not been fully copied

    // starting the loop now : x is the block from which we will read the data
    while (startIndexArray < count && (((x * 4096) + y) < currentBlock.inodes[iNodeOffset].i_size))
    {
        if (x < INODE_DIRECT_POINTERS)
        {
            union block tempDataBlock;
            disk_read(currentBlock.inodes[iNodeOffset].i_direct_pointers[x], &tempDataBlock);

            // now iterating over the block to copy each byte in its place :
            for (int i = y; i < BLOCK_SIZE; i++)
            {
                dataArray[startIndexArray] = tempDataBlock.data[i];
                startIndexArray++;
                if ((startIndexArray == count) || (((x * 4096) + startIndexArray + y) >= currentBlock.inodes[iNodeOffset].i_size))
                {
                    limitFlag = 1;
                    break;
                }
            }
            if (limitFlag)
            {
                break;
            }
        }
        // condition for checking the datablock is to be extracted from the single indirction block
        else if (x < (INODE_DIRECT_POINTERS + INODE_INDIRECT_POINTERS_PER_BLOCK))
        {
            union block tempDataBlock;
            union block tempSingleIndirectBlock;
            disk_read(currentBlock.inodes[iNodeOffset].i_single_indirect_pointer, &tempSingleIndirectBlock);

            // extracting the datablock to read :
            disk_read(tempSingleIndirectBlock.pointers[x - INODE_DIRECT_POINTERS], &tempDataBlock);
            // now iterating over the block to copy each byte in its place :
            for (int i = y; i < BLOCK_SIZE; i++)
            {
                dataArray[startIndexArray] = tempDataBlock.data[i];
                startIndexArray++;

                if (startIndexArray == count || (((x * 4096) + y + startIndexArray) == currentBlock.inodes[iNodeOffset].i_size))
                {
                    limitFlag = 1;
                    break;
                }
            }
            if (limitFlag)
            {
                break;
            }
        }
        // condition for checking the dataBlock to be extracted from the double indirect block
        else
        {
            union block tempDoubleIndirectBlock;
            union block tempSingleIndirectBlock;
            union block tempDataBlock;
            disk_read(currentBlock.inodes[iNodeOffset].i_double_indirect_pointer, &tempDoubleIndirectBlock);
            int indexForSingleIndirectBlock = giveCurrentStartingIndirectBlock(x);
            int indexForBlockInSingleIndirectBlock = (x - (INODE_DIRECT_POINTERS + INODE_INDIRECT_POINTERS_PER_BLOCK)) % INODE_INDIRECT_POINTERS_PER_BLOCK;
            // reading the singleIndirectBlock:
            disk_read(tempDoubleIndirectBlock.pointers[indexForSingleIndirectBlock], &tempSingleIndirectBlock);
            // extracting the datablock to read :
            disk_read(tempSingleIndirectBlock.pointers[indexForBlockInSingleIndirectBlock], &tempDataBlock);
            // now iterating over the block to copy each byte in its place :
            for (int i = y; i < BLOCK_SIZE; i++)
            {
                dataArray[startIndexArray] = tempDataBlock.data[i];
                startIndexArray++;

                if (startIndexArray == count || (((x * 4096) + y + startIndexArray) == currentBlock.inodes[iNodeOffset].i_size))
                {
                    limitFlag = 1;
                    break;
                }
            }
            if (limitFlag)
            {
                break;
            }
        }

        // updating the states of x & y
        x = x + 1;
        y = 0;
    }
    // sending the data back to the file buffer :
    memcpy(buf, dataArray, startIndexArray);
    // printf("bytes read are equal to %d \n", startIndexArray);
    return startIndexArray;
}

int fs_write(char *path, void *buf, size_t count, off_t offset)
{
    // char *path1 = strdup(path);
    // char *path2 = strdup(path);
    // int iNodeOfBlock = fs_create(path, 1);
    // // now deleting the file lol
    // printf("\n\t DELETING THE FILE NOW \n ");
    // fs_remove(path1);
    // printf("\n\t TRYING TO DELETE THE FILE AGAIN !!! \n ");
    // fs_remove(path2);

    // searching for the iNode if it exists or not :

    if (MOUNT_FLAG == 0)
    {
        return -1;
    }

    int count1;
    char *path2 = strdup(path);
    char *path3 = strdup(path);
    char **folders = parsePath(path, &count1);
    union block parentBlockNode;
    disk_read(3, &parentBlockNode);
    union block currentBlock;
    struct searchNodesReply replyRecieved = searchFile(folders, &parentBlockNode, count1, 0, 0);

    if (replyRecieved.currentInodeNumber == -1)
    {
        int iNodeOfBlock = fs_create(path2, 0);
    }
    // printf("YESSSS!!!!!\n");
    replyRecieved = searchFile(folders, &parentBlockNode, count1, 0, 0);
    int iNodeOffset = convertItoBOffset(replyRecieved.currentInodeNumber);
    disk_read(convertItoB(replyRecieved.currentInodeNumber), &currentBlock);

    // initializing the starting Blocks :
    int startingBlock = giveStartingBlock(offset);
    int startByte = offset % 4096;
    uint8_t *dataArray = (uint8_t *)malloc(count * sizeof(uint8_t));
    int startIndexArray = 0;
    int x = startingBlock;
    int y = startByte;
    int limitFlag = 0;

    // copying the data in the buf to the dataArray :
    memcpy(dataArray, buf, count);

    // looping and writing process now continues :
    while (startIndexArray < count)
    {
        if (x < INODE_DIRECT_POINTERS)
        {
            // check if this datablock is initialized or not :
            if (currentBlock.inodes[iNodeOffset].i_direct_pointers[x] == 0)
            {
                currentBlock.inodes[iNodeOffset].i_direct_pointers[x] = allocateDataEntryBlock(0);
                // saving this iNode :
                disk_write(convertItoB(replyRecieved.currentInodeNumber), &currentBlock);
            }

            union block tempDataBlock;
            disk_read(currentBlock.inodes[iNodeOffset].i_direct_pointers[x], &tempDataBlock);
            // now iterating over the block to copy each byte in its place :
            for (int i = y; i < BLOCK_SIZE; i++)
            {
                // dataArray[startIndexArray] = tempDataBlock.data[i];
                tempDataBlock.data[i] = dataArray[startIndexArray];
                startIndexArray++;

                if (startIndexArray == count)
                {
                    limitFlag = 1;
                    disk_write(currentBlock.inodes[iNodeOffset].i_direct_pointers[x], &tempDataBlock);
                    break;
                }
            }
            disk_write(currentBlock.inodes[iNodeOffset].i_direct_pointers[x], &tempDataBlock);
            if (limitFlag)
            {
                break;
            }
        }
        else if (x < (INODE_DIRECT_POINTERS + INODE_INDIRECT_POINTERS_PER_BLOCK))
        {
            // printf("YESSSSSSS\n");
            union block tempDataBlock;
            union block tempSingleIndirectBlock;
            // checking if the single_indirect_pointer is initialized or not :
            if (currentBlock.inodes[iNodeOffset].i_single_indirect_pointer == 0)
            {
                // printf("YESSSSSSS1\n");
                currentBlock.inodes[iNodeOffset].i_single_indirect_pointer = allocateDataEntryBlock(1);
                disk_write(convertItoB(replyRecieved.currentInodeNumber), &currentBlock);
            }

            disk_read(currentBlock.inodes[iNodeOffset].i_single_indirect_pointer, &tempSingleIndirectBlock);

            if (tempSingleIndirectBlock.pointers[x - INODE_DIRECT_POINTERS] == 0)
            {
                tempSingleIndirectBlock.pointers[x - INODE_DIRECT_POINTERS] = allocateDataEntryBlock(0);
                disk_write(currentBlock.inodes[iNodeOffset].i_single_indirect_pointer, &tempSingleIndirectBlock);
            }

            disk_read(tempSingleIndirectBlock.pointers[x - INODE_DIRECT_POINTERS], &tempDataBlock);

            // now copying the data into the block as needed :

            for (int i = y; i < BLOCK_SIZE; i++)
            {
                // dataArray[startIndexArray] = tempDataBlock.data[i];
                tempDataBlock.data[i] = dataArray[startIndexArray];
                startIndexArray++;

                if (startIndexArray == count)
                {
                    limitFlag = 1;
                    disk_write(tempSingleIndirectBlock.pointers[x - INODE_DIRECT_POINTERS], &tempDataBlock);
                    break;
                }
            }
            disk_write(tempSingleIndirectBlock.pointers[x - INODE_DIRECT_POINTERS], &tempDataBlock);
            if (limitFlag)
            {
                break;
            }
        }
        else
        {
            union block tempDoubleIndirectBlock;
            union block tempSingleIndirectBlock;
            union block tempDataBlock;

            // 1) ensuring i have the Double Indirect block initialized :
            if (currentBlock.inodes[iNodeOffset].i_double_indirect_pointer == 0)
            {
                currentBlock.inodes[iNodeOffset].i_double_indirect_pointer = allocateDataEntryBlock(1);
                disk_write(convertItoB(replyRecieved.currentInodeNumber), &currentBlock);
            }

            // 2) reading the Double Indirect block :
            disk_read(currentBlock.inodes[iNodeOffset].i_double_indirect_pointer, &tempDoubleIndirectBlock);

            // 3) getting the index for the single Indirect block whhich will be used for writing :
            int indexForSingleIndirectBlock = giveCurrentStartingIndirectBlock(x);

            // 4) checking if this block is allocated or not
            if (tempDoubleIndirectBlock.pointers[indexForSingleIndirectBlock] == 0)
            {
                tempDoubleIndirectBlock.pointers[indexForSingleIndirectBlock] = allocateDataEntryBlock(1);
                disk_write(tempDoubleIndirectBlock.pointers[indexForSingleIndirectBlock], &tempDoubleIndirectBlock);
            }

            // 5) extracting the block of single from double indirect
            disk_read(tempDoubleIndirectBlock.pointers[indexForSingleIndirectBlock], &tempSingleIndirectBlock);

            int indexForBlockInSingleIndirectBlock = (x - (INODE_DIRECT_POINTERS + INODE_INDIRECT_POINTERS_PER_BLOCK)) % INODE_INDIRECT_POINTERS_PER_BLOCK;

            // 6) checking if the block on whcih we will write the data is allocated or not :
            if (tempSingleIndirectBlock.pointers[indexForBlockInSingleIndirectBlock] == 0)
            {
                tempSingleIndirectBlock.pointers[indexForBlockInSingleIndirectBlock] = allocateDataEntryBlock(0);
                disk_write(tempSingleIndirectBlock.pointers[indexForBlockInSingleIndirectBlock], &tempDataBlock);
            }

            // 7) time to copy & save data into this pointer :
            disk_read(tempSingleIndirectBlock.pointers[indexForBlockInSingleIndirectBlock], &tempDataBlock);

            for (int i = y; i < BLOCK_SIZE; i++)
            {
                // dataArray[startIndexArray] = tempDataBlock.data[i];
                tempDataBlock.data[i] = dataArray[startIndexArray];
                startIndexArray++;

                if (startIndexArray == count)
                {
                    limitFlag = 1;
                    disk_write(tempSingleIndirectBlock.pointers[indexForBlockInSingleIndirectBlock], &tempDataBlock);
                    break;
                }
            }
            disk_write(tempSingleIndirectBlock.pointers[indexForBlockInSingleIndirectBlock], &tempDataBlock);
            if (limitFlag)
            {
                break;
            }
        }
        x = x + 1;
        y = 0;
    }

    // printf("the bytes copied are equal to : %d \n", startIndexArray);
    //  saving the size now :
    currentBlock.inodes[convertItoBOffset(replyRecieved.currentInodeNumber)].i_size = currentBlock.inodes[convertItoBOffset(replyRecieved.currentInodeNumber)].i_size + startIndexArray;
    disk_write(convertItoB(replyRecieved.currentInodeNumber), &currentBlock);

    // updating the parents :
    union block rootBlock;
    disk_read(3, &rootBlock);
    // updateSize(folders, &rootBlock, count1, 0, 0, startIndexArray);
    //  fs_list(path3);
    //  printf("the bytes copied are equal to : %d \n", startIndexArray);
    return startIndexArray;
}

void printEntryTable(int directoryTableBlockNumb)
{
    union block dirBlock;
    disk_read(directoryTableBlockNumb, &dirBlock);
    // printf("the directorytable block number is : %d \n", directoryTableBlockNumb);

    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        // use strcmp here
        if ((strcmp(dirBlock.directory_block.entries[i].name, "0") == 0) || strcmp(dirBlock.directory_block.entries[i].name, "") == 0)
        {
            continue;
        }
        else
        {
            union block tempInode;
            disk_read(convertItoB(dirBlock.directory_block.entries[i].inode_number), &tempInode);
            printf("%s %ld\n", dirBlock.directory_block.entries[i].name, tempInode.inodes[convertItoBOffset(dirBlock.directory_block.entries[i].inode_number)].i_size);
        }
    }
    return -1;
}

int fs_list(char *path)
{

    if (MOUNT_FLAG == 0)
    {
        return -1;
    }
    // getting the iNode of the last item :
    int count1;
    char *path2 = strdup(path);
    char **folders = parsePath(path, &count1);
    union block parentBlockNode;
    disk_read(3, &parentBlockNode);
    union block currentBlock;
    if (strcmp(path, "/") == 0)
    {
        count1 = count1 - 1;
    }

    struct searchNodesReply replyRecieved;
    if (count1 <= 0)
    {
        replyRecieved.currentInodeNumber = 0;
        replyRecieved.parentInodeNumber = -1;
        replyRecieved.tableEntryNumber = -1;
    }
    else
    {
        replyRecieved = searchFile(folders, &parentBlockNode, count1, 0, 0);
    }

    // printf("the results are : %d \n", replyRecieved.currentInodeNumber);
    //  cater the case where count1 = 1 i.e root;
    disk_read(convertItoB(replyRecieved.currentInodeNumber), &currentBlock);

    int currentBlockOffset = convertItoBOffset(replyRecieved.currentInodeNumber);

    int singleInDirectBlockNumber = currentBlock.inodes[currentBlockOffset].i_single_indirect_pointer;
    union block singleInDirectBlock;

    int doubleInDirectBlockNumber = currentBlock.inodes[currentBlockOffset].i_double_indirect_pointer;
    union block doubleInDirectBlock;

    for (int i = 0; i < INODE_DIRECT_POINTERS; i++)
    {
        if (currentBlock.inodes[convertItoBOffset(replyRecieved.currentInodeNumber)].i_direct_pointers[i] == 0)
        {
            continue;
        }
        else
        {
            printEntryTable(currentBlock.inodes[convertItoBOffset(replyRecieved.currentInodeNumber)].i_direct_pointers[i]);
        }
    }

    if (singleInDirectBlockNumber != 0)
    {
        disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
        for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            if (singleInDirectBlock.pointers[i] > 1)
            {
                printEntryTable(singleInDirectBlock.pointers[i]);
            }
        }
    }

    if (doubleInDirectBlockNumber != 0)
    {
        disk_read(doubleInDirectBlockNumber, &doubleInDirectBlock);
        for (int i = 0; i < INODE_INDIRECT_POINTERS_PER_BLOCK; i++)
        {
            singleInDirectBlockNumber = doubleInDirectBlock.pointers[i];
            if (singleInDirectBlockNumber != 0)
            {
                disk_read(singleInDirectBlockNumber, &singleInDirectBlock);
                for (int j = 0; j < INODE_INDIRECT_POINTERS_PER_BLOCK; j++)
                {
                    if (singleInDirectBlock.pointers[j] > 1)
                    {
                        printEntryTable(singleInDirectBlock.pointers[i]);
                    }
                }
            }
        }
    }

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

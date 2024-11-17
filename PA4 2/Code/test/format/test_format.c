#include "fs.h"
#include "disk.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int small_test()
{
    union block test_block;
    test_block.superblock.s_blocks_count = 16;
    test_block.superblock.s_inodes_count = 16;
    test_block.superblock.s_block_bitmap = 1;
    test_block.superblock.s_inode_bitmap = 2;
    test_block.superblock.s_inode_table_block_start = 3;
    test_block.superblock.s_data_blocks_start = 4;

    if (disk_init("test/images/user/small_format.img", 16) == -1)
    {
        printf("ERROR: Could not initialize disk.\n");
        return -1;
    }

    // Format the disk.
    if (fs_format() == -1)
    {
        printf("ERROR: Could not format disk.\n");
        return -1;
    }

    union block block;

    // Read the superblock.
    if (disk_read(0, block.data) == -1)
    {
        printf("ERROR: Could not read superblock.\n");
        return -1;
    }

    // Close STDOUT.
    close(1);

    // Close the disk.
    if (disk_close() == -1)
    {
        printf("ERROR: Could not close disk.\n");
        return -1;
    }

    // Open STDOUT.
    open("/dev/tty", O_WRONLY);
}

int medium_test()
{

    union block test_block;
    test_block.superblock.s_blocks_count = 100;
    test_block.superblock.s_inodes_count = 100;
    test_block.superblock.s_block_bitmap = 1;
    test_block.superblock.s_inode_bitmap = 2;
    test_block.superblock.s_inode_table_block_start = 3;
    test_block.superblock.s_data_blocks_start = 5;

    if (disk_init("test/images/user/medium_format.img", 100) == -1)
    {
        printf("ERROR: Could not initialize disk.\n");
        return -1;
    }

    // Format the disk.
    if (fs_format() == -1)
    {
        printf("ERROR: Could not format disk.\n");
        return -1;
    }

    union block block;

    // Read the superblock.
    if (disk_read(0, block.data) == -1)
    {
        printf("ERROR: Could not read superblock.\n");
        return -1;
    }

    // Close STDOUT.
    close(1);

    // Close the disk.
    if (disk_close() == -1)
    {
        printf("ERROR: Could not close disk.\n");
        return -1;
    }

    // Open STDOUT.
    open("/dev/tty", O_WRONLY);
}

int large_test()
{

    union block test_block;
    test_block.superblock.s_blocks_count = 1000;
    test_block.superblock.s_inodes_count = 1000;
    test_block.superblock.s_block_bitmap = 1;
    test_block.superblock.s_inode_bitmap = 2;
    test_block.superblock.s_inode_table_block_start = 3;
    test_block.superblock.s_data_blocks_start = 19;

    if (disk_init("test/images/user/large_format.img", 1000) == -1)
    {
        printf("ERROR: Could not initialize disk.\n");
        return -1;
    }

    // Format the disk.
    if (fs_format() == -1)
    {
        printf("ERROR: Could not format disk.\n");
        return -1;
    }

    union block block;

    // Read the superblock.
    if (disk_read(0, block.data) == -1)
    {
        printf("ERROR: Could not read superblock.\n");
        return -1;
    }

    // Close STDOUT.
    close(1);

    // Close the disk.
    if (disk_close() == -1)
    {
        printf("ERROR: Could not close disk.\n");
        return -1;
    }

    // Open STDOUT.
    open("/dev/tty", O_WRONLY);
}

int main()
{
    int total = 3;

    printf("Testing fs_format()...\n");
    if (small_test() == -1)
    {
        printf("Test Failed: Small Format.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Small Format.\n");
    }

    if (medium_test() == -1)
    {
        printf("Test Failed: Medium Format.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Medium Format.\n");
    }

    if (large_test() == -1)
    {
        printf("Test Failed: Large Format.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Large Format.\n");
    }

    printf("%d/3 Format test(s) passed.\n", total);

    return 0;
}
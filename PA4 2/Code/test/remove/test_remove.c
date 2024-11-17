#include "fs.h"
#include "disk.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int rm_test_one()
{
    // Initialize the disk.
    if (disk_init("test/images/sys/rm_1.img", 700) == -1)
    {
        printf("ERROR: Could not initialize disk.\n");
        return -1;
    }

    // Mount the disk.
    if (fs_mount() == -1)
    {
        printf("ERROR: Could not mount disk.\n");
        return -1;
    }

    // Remove the file.
    if (fs_remove("/bar") == -1)
    {
        printf("ERROR: Could not remove file.\n");
        return -1;
    }

    // Redirect STDOUT.
    int fd = open("test/dump/rm_1.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd == -1)
    {
        printf("ERROR: Could not open file.\n");
        return -1;
    }

    if (dup2(fd, 1) == -1)
    {
        printf("ERROR: Could not redirect STDOUT.\n");
        return -1;
    }

    // List the root directory.
    if (fs_list("/") == -1)
    {
        printf("ERROR: Could not list directory.\n");
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

    fs_unmount();

    // // Compare two disks.
    // if (system("cmp test/data/rm_1.txt test/dump/rm_1.txt") != 0)
    // {
    //     printf("ERROR: Outputs do not match.\n");
    //     return -1;
    // }

    return 0;
}

int rm_test_two()
{
    // Initialize the disk.
    if (disk_init("test/images/sys/rm_2.img", 700) == -1)
    {
        printf("ERROR: Could not initialize disk.\n");
        return -1;
    }

    // Mount the disk.
    if (fs_mount() == -1)
    {
        printf("ERROR: Could not mount disk.\n");
        return -1;
    }

    // Remove the file.
    if (fs_remove("/a") == -1)
    {
        printf("ERROR: Could not remove file.\n");
        return -1;
    }

    // Redirect STDOUT.
    int fd = open("test/dump/rm_2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd == -1)
    {
        printf("ERROR: Could not open file.\n");
        return -1;
    }

    if (dup2(fd, 1) == -1)
    {
        printf("ERROR: Could not redirect STDOUT.\n");
        return -1;
    }

    // List the root directory.
    if (fs_list("/") == -1)
    {
        printf("ERROR: Could not list directory.\n");
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

    fs_unmount();

    // // Compare two disks.
    // if (system("cmp test/data/rm_2.txt test/dump/rm_2.txt") != 0)
    // {
    //     printf("ERROR: Outputs do not match.\n");
    //     return -1;
    // }

    return 0;
}

int main()
{
    printf("Testing fs_remove()...\n");

    int total = 0;

    if (rm_test_one())
    {
        printf("ERROR: Remove test one failed.\n");
        total -= 1;
    }

    total += 1;

    if (rm_test_two())
    {
        printf("ERROR: Remove test two failed.\n");
        total -= 1;
    }

    total += 1;

    printf("%d/2 Remove test(s) passed.\n", total);

    // system("rm test/dump/*");

    return 0;
}
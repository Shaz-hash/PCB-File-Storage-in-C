#include "fs.h"
#include "disk.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int create_file_test()
{
    if (disk_init("test/images/user/create_1.img", 16) == -1)
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

    // Mount the disk.
    if (fs_mount() == -1)
    {
        printf("ERROR: Could not mount disk.\n");
        return -1;
    }

    char *path = "/foo/bar/baz";
    char *deep_copy = strdup(path);

    // Create a file.
    if (fs_create(deep_copy, 0) == -1)
    {
        printf("ERROR: Could not create file.\n");
        return -1;
    }

    char *path2 = "/foo/bar/bat";
    char *deep_copy2 = strdup(path2);

    // Create a file.
    if (fs_create(deep_copy2, 0) == -1)
    {
        printf("ERROR: Could not create file.\n");
        return -1;
    }

    char *path3 = "/foo/bag";
    char *deep_copy3 = strdup(path3);

    // Create a file.
    if (fs_create(deep_copy3, 0) == -1)
    {
        printf("ERROR: Could not create file.\n");
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

    // fs_list("/");
    // Unmount
    fs_unmount();

    return 0;
}

int create_dir_test()
{
    if (disk_init("test/images/user/create_2.img", 36) == -1)
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

    // Mount the disk.
    if (fs_mount() == -1)
    {
        printf("ERROR: Could not mount disk.\n");
        return -1;
    }

    char *path = "/foo/bar/baz";
    char *deep_copy = strdup(path);

    // Create a file.
    if (fs_create(deep_copy, 1) == -1)
    {
        printf("ERROR: Could not create file.\n");
        return -1;
    }

    char *path2 = "/foo/bar/bat";
    char *deep_copy2 = strdup(path2);

    // Create a file.
    if (fs_create(deep_copy2, 1) == -1)
    {
        printf("ERROR: Could not create file.\n");
        return -1;
    }

    char *path3 = "/foo/bag";
    char *deep_copy3 = strdup(path3);

    // Create a file.
    if (fs_create(deep_copy3, 1) == -1)
    {
        printf("ERROR: Could not create file.\n");
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
    // fs_list("/");
    // Unmount
    fs_unmount();

    return 0;
}

int main()
{
    int total = 2;
    printf("Testing fs_create()...\n");

    if (create_file_test() == -1)
    {
        printf("Test Failed: Create File.\n");
        total -= 1;
    }
    else
    {
        // printf("wowwowow");
        //  fs_list("/");
        printf("Test Passed: Create File.\n");
    }

    if (create_dir_test() == -1)
    {
        printf("Test Failed: Create Directory.\n");
        total -= 1;
    }
    else
    {
        // printf("wowww\n");
        //  fs_list("/");
        printf("Test Passed: Create Directory.\n");
    }

    printf("%d/2 Create test(s) passed.\n", total);

    return 0;
}
#include "fs.h"
#include "disk.h"

#include <string.h>
#include <stdlib.h>

int main()
{
    // Initialize the disk.
    if (disk_init("disk.img", 16) == -1)
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
    char* deep_copy = strdup(path);

    // Create a file.
    if (fs_create(deep_copy, 0) == -1)
    {
        printf("ERROR: Could not create file.\n");
        return -1;
    }

    char *path2 = "/foo/bar/bat";
    char* deep_copy2 = strdup(path2);

    // Create a file.
    if (fs_create(deep_copy2, 1) == -1)
    {
        printf("ERROR: Could not create file.\n");
        return -1;
    }

    char *path3 = "/foo/bag";
    char* deep_copy3 = strdup(path3);

    // Create a file.
    if (fs_create(deep_copy3, 0) == -1)
    {
        printf("ERROR: Could not create file.\n");
        return -1;
    }

    char * list_path = "/foo/bar/baz";
    char* list_deep_copy = strdup(list_path);

    char* buf = malloc(100);

    // List the directory.
    if (fs_write(list_deep_copy, buf, 0, 0) == -1)
    {
        printf("ERROR: Could not list directory.\n");
        return -1;
    }
   

    // Close the disk.
    if (disk_close() == -1)
    {
        printf("ERROR: Could not close disk.\n");
        return -1;
    }

    return 0;
}
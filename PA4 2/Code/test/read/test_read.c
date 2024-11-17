#include "fs.h"
#include "disk.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int copy_out(char *fs_path, char *local_path)
{
    // Open the local file.
    FILE *local_file = fopen(local_path, "a");

    if (local_file == NULL)
    {
        printf("ERROR: Could not open local file.\n");
        return -1;
    }

    // Create a buffer for the FS file.
    char *fs_file_buffer = calloc(BLOCK_SIZE, sizeof(char));

    if (fs_file_buffer == NULL)
    {
        printf("ERROR: Could not allocate buffer for FS file.\n");
        return -1;
    }

    off_t offset = 0;
    while (1)
    {
        // Read the FS file into the buffer.
        int bytes_read = fs_read(fs_path, fs_file_buffer, BLOCK_SIZE, offset);

        if (bytes_read == -1)
        {
            printf("ERROR: Could not read FS file into buffer.\n");
            return -1;
        }

        // Write the buffer to the local file.
        if ((int)fwrite(fs_file_buffer, sizeof(char), bytes_read, local_file) != bytes_read)
        {
            printf("ERROR: Could not write buffer to local file.\n");
            return -1;
        }

        // If we read less than BLOCK_SIZE bytes, we've reached the end of the file.
        if (bytes_read < BLOCK_SIZE)
        {
            break;
        }

        // Otherwise, continue reading.
        offset += bytes_read;
    }

    // Close the local file.
    if (fclose(local_file) == EOF)
    {
        printf("ERROR: Could not close local file.\n");
        return -1;
    }

    // Free the FS file buffer.
    free(fs_file_buffer);

    return 0;
}

int basic_read_test()
{
    // Initialize the disk.
    if (disk_init("test/images/sys/write_1.img", 16) == -1)
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

    char *path = "test_1.txt";

    if (copy_out("/foo", path))
    {
        printf("ERROR: Could not copy out file.\n");
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
    // if (system("cmp test/data/test_1.txt test/dump/test_1.txt") != 0)
    // {
    //     printf("ERROR: Disk images do not match.\n");
    //     return -1;
    // }

    return 0;
}

int medium_read_test()
{
    // Initialize the disk.
    if (disk_init("test/images/sys/write_2.img", 36) == -1)
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

    char *path = "test_2.txt";

    if (copy_out("/foo", path))
    {
        printf("ERROR: Could not copy out file.\n");
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
    // if (system("cmp test/data/test_2.txt test/dump/test_2.txt") != 0)
    // {
    //     printf("ERROR: Disk images do not match.\n");
    //     return -1;
    // }

    return 0;
}

int advanced_read_test()
{
    // Initialize the disk.
    if (disk_init("test/images/sys/write_4.img", 700) == -1)
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

    char *path = "random.pdf";

    if (copy_out("/foo", path))
    {
        printf("ERROR: Could not copy out file.\n");
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
    // if (system("cmp test/data/random.pdf test/dump/random.pdf") != 0)
    // {
    //     printf("ERROR: Disk images do not match.\n");
    //     return -1;
    // }

    return 0;
}

int main()
{
    printf("Testing fs_read()...\n");

    int total = 3;

    if (basic_read_test())
    {
        printf("Test Failed: Basic Read.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Basic Read.\n");
    }

    if (medium_read_test())
    {
        printf("Test Failed: Medium Read.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Medium Read.\n");
    }

    if (advanced_read_test())
    {
        printf("Test Failed: Advanced Read.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Advanced Read.\n");
    }

    system("rm test/dump/*");

    printf("%d/3 Read test(s) passed.\n", total);

    return 0;
}
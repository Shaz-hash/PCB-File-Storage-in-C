#include "fs.h"
#include "disk.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int copy_in(char *local_path, char *fs_path)
{
    // Open the local file.
    FILE *local_file = fopen(local_path, "r");

    if (local_file == NULL)
    {
        printf("ERROR: Could not open local file.\n");
        return -1;
    }

    // Get the size of the local file.
    fseek(local_file, 0, SEEK_END);
    int local_file_size = ftell(local_file);
    fseek(local_file, 0, SEEK_SET);

    // Allocate a buffer for the local file.
    char *local_file_buffer = calloc(local_file_size, sizeof(char));

    if (local_file_buffer == NULL)
    {
        printf("ERROR: Could not allocate buffer for local file.\n");
        return -1;
    }
    // Read the local file into the buffer.
    if ((int)fread(local_file_buffer, sizeof(char), local_file_size, local_file) != local_file_size)
    {
        printf("ERROR: Could not read local file into buffer.\n");
        return -1;
    }

    // Close the local file.
    if (fclose(local_file) == EOF)
    {
        printf("ERROR: Could not close local file.\n");
        return -1;
    }

    // Write the local file buffer to the FS file.
    if (fs_write(fs_path, local_file_buffer, local_file_size, 0) == -1)
    {
        printf("ERROR: Could not write local file buffer to FS file.\n");
        return -1;
    }

    // Free the local file buffer.
    free(local_file_buffer);

    return 0;
}

int basic_write_test()
{
    if (disk_init("test/images/user/write_1.img", 16) == -1)
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

    char *path = "test/data/test_1.txt";

    if (copy_in(path, "/foo"))
    {
        printf("ERROR: Could not copy in file.\n");
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
    // if (system("cmp test/images/user/write_1.img test/images/sys/write_1.img") != 0)
    // {
    //     printf("ERROR: Disk images do not match.\n");
    //     return -1;
    // }

    return 0;
}

int large_write_test()
{
    if (disk_init("test/images/user/write_2.img", 36) == -1)
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

    char *path = "test/data/test_2.txt";

    if (copy_in(path, "/foo"))
    {
        printf("ERROR: Could not copy in file.\n");
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
    // if (system("cmp test/images/user/write_2.img test/images/sys/write_2.img") != 0)
    // {
    //     printf("ERROR: Disk images do not match.\n");
    //     return -1;
    // }

    return 0;
}

int append_test()
{
    if (disk_init("test/images/user/write_3.img", 10) == -1)
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

    char *text = "Hello, world!";
    char *text2 = " This is appended data.";

    if (fs_write("/foo", text, strlen(text), 0) == -1)
    {
        printf("ERROR: Could not write to file.\n");
        return -1;
    }

    if (fs_write("/foo", text2, strlen(text2), strlen(text)) == -1)
    {
        printf("ERROR: Could not write to file.\n");
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
    // if (system("cmp test/images/user/write_3.img test/images/sys/write_3.img") != 0)
    // {
    //     printf("ERROR: Disk images do not match.\n");
    //     return -1;
    // }

    return 0;
}

int huge_write_test()
{
    if (disk_init("test/images/user/write_4.img", 700) == -1)
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

    char *path = "test/data/random.pdf";

    if (copy_in(path, "/foo"))
    {
        printf("ERROR: Could not copy in file.\n");
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
    // if (system("cmp test/images/user/write_4.img test/images/sys/write_4.img") != 0)
    // {
    //     printf("ERROR: Disk images do not match.\n");
    //     return -1;
    // }

    return 0;
}

int main()
{
    int total = 4;
    printf("Testing fs_write()...\n");
    if (basic_write_test() == -1)
    {
        printf("Test Failed: Basic Write.\n");
        total -= 1;
    }
    else
    {

        printf("Test Passed: Basic Write.\n");
    }

    if (large_write_test() == -1)
    {
        printf("Test Failed: Large Write.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Large Write.\n");
    }

    if (append_test() == -1)
    {
        printf("Test Failed: Append.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Append.\n");
    }

    if (huge_write_test() == -1)
    {
        printf("Test Failed: Huge Write.\n");
        total -= 1;
    }
    else
    {
        printf("Test Passed: Huge Write.\n");
    }

    printf("%d/4 Write test(s) passed.\n", total);

    return 0;
}
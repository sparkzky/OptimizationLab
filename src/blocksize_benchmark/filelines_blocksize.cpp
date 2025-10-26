#include "filelines_blocksize.h"
#include "find_most_freq.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

void filelines_with_blocksize(char* filepath, uint32_t* total_line_num,
                               uint32_t* line_num, size_t block_size) {
    int handle;
    if ((handle = open(filepath, O_RDONLY)) < 0)
        return;

    char* bp = (char*)malloc(block_size);
    if (bp == NULL) {
        close(handle);
        return;
    }

    int cur_len = 0;
    while (1) {
        ssize_t bytes_read = read(handle, bp, block_size);
        if (bytes_read <= 0)
            break;

        for (ssize_t i = 0; i < bytes_read; i++) {
            if (bp[i] == '\n') {
                (*total_line_num)++;
                if (cur_len >= MAX_LEN)
                    line_num[MAX_LEN - 1]++;
                else
                    line_num[cur_len]++;
                cur_len = 0;
            } else {
                cur_len++;
            }
        }
    }

    free(bp);
    close(handle);
}

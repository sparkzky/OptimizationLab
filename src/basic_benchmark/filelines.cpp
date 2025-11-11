
#include "filelines_mt.h"
#include "find_most_freq.h"

#include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s filepath", argv[0]);
        return -1;
    }
    uint32_t line_num[MAX_LEN];
    for (int i = 0; i < MAX_LEN; i++)
        line_num[i] = 0;
    uint32_t total_line_num = 0;

    filelines_mt(argv[1], &total_line_num, line_num);

    uint32_t most_freq_len, most_freq_len_linenum;
    find_most_freq_line(line_num, &most_freq_len, &most_freq_len_linenum);
    printf("%d %d %d\n", total_line_num, most_freq_len, most_freq_len_linenum);
}

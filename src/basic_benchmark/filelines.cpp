/*
* 基准测试程序，这里使用256KB作为一个基本的块大小进行文件行分析
*/

#include "filelines_baseline.h"
#include "find_most_freq.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t line_num[MAX_LEN];
uint32_t total_line_num;
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s filepath", argv[0]);
        return -1;
    }
    for (int i = 0; i < MAX_LEN; i++)
        line_num[i] = 0;
    total_line_num = 0;
    filelines_baseline(argv[1], &total_line_num, line_num);
    uint32_t most_freq_len, most_freq_len_linenum;
    find_most_freq_line(line_num, &most_freq_len, &most_freq_len_linenum);
    printf("%d %d %d\n", total_line_num, most_freq_len, most_freq_len_linenum);
}

#include "filelines_simd_opt.h"

#include "find_most_freq.h"

#include <fcntl.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLOCK_SIZE 256 << 10 // 256KB - 与basic_benchmark一致

// 优化的SIMD处理函数
static inline void
process_block_simd_opt(const char* buffer, ssize_t size, uint32_t* total_line_num, uint32_t* line_num, int* cur_len) {
    ssize_t i = 0;
    const __m256i newline = _mm256_set1_epi8('\n');

    // SIMD处理：每次处理32字节
    while (i + 32 <= size) {
        __m256i chunk = _mm256_loadu_si256((__m256i*)(buffer + i));
        __m256i cmp = _mm256_cmpeq_epi8(chunk, newline);
        uint32_t mask = _mm256_movemask_epi8(cmp);

        i += 32;
        if (__builtin_expect(mask == 0, 0)) {
            // 快速路径：没有换行符
            *cur_len += 32;
        } else {
            int newlines_in_chunk = __builtin_popcount(mask);
            *total_line_num += newlines_in_chunk;
            int last_pos = -1;
            while (mask != 0) {
                int pos = __builtin_ctz(mask);
                int line_length = *cur_len + (pos - last_pos - 1);
                if (line_length < MAX_LEN) {
                    ++line_num[line_length];
                } else {
                    ++line_num[MAX_LEN - 1];
                }
                *cur_len = 0;
                last_pos = pos;
                // 高效清除这个已经处理过的 '1'
                mask &= (mask - 1);
            }
            *cur_len = 31 - last_pos;
        }
    }
    // 处理剩余字节
    while (i < size) {
        if (buffer[i] == '\n') {
            ++(*total_line_num);
            if (*cur_len >= MAX_LEN)
                ++line_num[MAX_LEN - 1];
            else
                ++line_num[*cur_len];
            *cur_len = 0;
        } else {
            ++(*cur_len);
        }
        ++i;
    }
}

void filelines_simd(char* filepath, uint32_t* total_line_num, uint32_t* line_num) {
    int handle;
    if ((handle = open(filepath, O_RDONLY)) < 0)
        return;

    // 对齐分配以提升SIMD性能
    char* bp = (char*)aligned_alloc(32, BLOCK_SIZE);
    if (bp == NULL) {
        close(handle);
        return;
    }

    int cur_len = 0;
    while (1) {
        ssize_t bytes_read = read(handle, bp, BLOCK_SIZE);
        if (bytes_read <= 0)
            break;

        process_block_simd_opt(bp, bytes_read, total_line_num, line_num, &cur_len);
    }

    free(bp);
    close(handle);
}

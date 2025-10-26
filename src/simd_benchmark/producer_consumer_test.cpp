#include "filelines_producer_consumer.h"
#include "filelines_simd_opt.h"
#include "find_most_freq.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

// 获取当前时间（微秒）
static inline uint64_t get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// 获取文件大小
static inline uint64_t get_file_size(const char* filepath) {
    struct stat st;
    if (stat(filepath, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("用法: %s <文件路径>\n", argv[0]);
        return 1;
    }

    char* filepath = argv[1];

    // 获取文件大小
    uint64_t file_size = get_file_size(filepath);
    if (file_size == 0) {
        printf("无法获取文件大小或文件为空\n");
        return 1;
    }

    // 初始化统计数组
    uint32_t total_line_num_simd = 0;
    uint32_t total_line_num_pc = 0;
    uint32_t* line_num_simd = (uint32_t*)calloc(MAX_LEN, sizeof(uint32_t));
    uint32_t* line_num_pc = (uint32_t*)calloc(MAX_LEN, sizeof(uint32_t));

    if (!line_num_simd || !line_num_pc) {
        printf("内存分配失败\n");
        return 1;
    }

    printf("=== 文件行分析性能测试 ===\n");
    printf("文件路径: %s\n", filepath);
    printf("文件大小: %.2f MB (%.2f GB)\n\n",
           file_size / 1024.0 / 1024.0,
           file_size / 1024.0 / 1024.0 / 1024.0);

    // 测试1: 原始SIMD版本
    printf("测试1: 原始SIMD版本\n");
    uint64_t start_simd = get_time_us();
    filelines_simd(filepath, &total_line_num_simd, line_num_simd);
    uint64_t end_simd = get_time_us();
    double time_simd_ms = (end_simd - start_simd) / 1000.0;
    double time_simd_s = time_simd_ms / 1000.0;
    double throughput_simd = (file_size / 1024.0 / 1024.0) / time_simd_s;  // MB/s

    uint32_t most_freq_len_simd, most_freq_len_linenum_simd;
    find_most_freq_line(line_num_simd, &most_freq_len_simd, &most_freq_len_linenum_simd);

    printf("  总行数: %u\n", total_line_num_simd);
    printf("  最常见的行长度: %u (出现 %u 次)\n", most_freq_len_simd, most_freq_len_linenum_simd);
    printf("  运行时间: %.2f ms (%.3f s)\n", time_simd_ms, time_simd_s);
    printf("  硬盘吞吐量: %.2f MB/s (%.2f GB/s)\n\n", throughput_simd, throughput_simd / 1024.0);

    // 测试2: 生产者-消费者模型版本
    printf("测试2: 生产者-消费者模型版本\n");
    uint64_t start_pc = get_time_us();
    filelines_producer_consumer(filepath, &total_line_num_pc, line_num_pc);
    uint64_t end_pc = get_time_us();
    double time_pc_ms = (end_pc - start_pc) / 1000.0;
    double time_pc_s = time_pc_ms / 1000.0;
    double throughput_pc = (file_size / 1024.0 / 1024.0) / time_pc_s;  // MB/s

    uint32_t most_freq_len_pc, most_freq_len_linenum_pc;
    find_most_freq_line(line_num_pc, &most_freq_len_pc, &most_freq_len_linenum_pc);

    printf("  总行数: %u\n", total_line_num_pc);
    printf("  最常见的行长度: %u (出现 %u 次)\n", most_freq_len_pc, most_freq_len_linenum_pc);
    printf("  运行时间: %.2f ms (%.3f s)\n", time_pc_ms, time_pc_s);
    printf("  硬盘吞吐量: %.2f MB/s (%.2f GB/s)\n\n", throughput_pc, throughput_pc / 1024.0);

    // 验证结果一致性
    printf("=== 结果验证 ===\n");
    bool correct = (total_line_num_simd == total_line_num_pc);
    if (correct) {
        for (int i = 0; i < MAX_LEN; i++) {
            if (line_num_simd[i] != line_num_pc[i]) {
                correct = false;
                printf("  错误: line_num[%d] 不一致: SIMD=%u, PC=%u\n", i, line_num_simd[i], line_num_pc[i]);
                break;
            }
        }
    } else {
        printf("  错误: 总行数不一致: SIMD=%u, PC=%u\n", total_line_num_simd, total_line_num_pc);
    }

    if (correct) {
        printf("  ✓ 结果一致\n\n");
    } else {
        printf("  ✗ 结果不一致\n\n");
    }

    // 性能比较
    printf("=== 性能对比 ===\n");
    printf("  原始SIMD:\n");
    printf("    运行时间: %.2f ms (%.3f s)\n", time_simd_ms, time_simd_s);
    printf("    吞吐量:   %.2f MB/s (%.2f GB/s)\n", throughput_simd, throughput_simd / 1024.0);
    printf("  生产者-消费者:\n");
    printf("    运行时间: %.2f ms (%.3f s)\n", time_pc_ms, time_pc_s);
    printf("    吞吐量:   %.2f MB/s (%.2f GB/s)\n", throughput_pc, throughput_pc / 1024.0);
    printf("\n");

    if (time_pc_ms < time_simd_ms) {
        printf("  时间加速比:      %.2fx (快 %.2f%%)\n",
               time_simd_ms / time_pc_ms,
               (time_simd_ms - time_pc_ms) / time_simd_ms * 100.0);
        printf("  吞吐量提升:      %.2f%% (从 %.2f MB/s 到 %.2f MB/s)\n",
               (throughput_pc - throughput_simd) / throughput_simd * 100.0,
               throughput_simd, throughput_pc);
    } else {
        printf("  时间加速比:      %.2fx (慢 %.2f%%)\n",
               time_simd_ms / time_pc_ms,
               (time_pc_ms - time_simd_ms) / time_pc_ms * 100.0);
        printf("  吞吐量降低:      %.2f%% (从 %.2f MB/s 到 %.2f MB/s)\n",
               (throughput_simd - throughput_pc) / throughput_simd * 100.0,
               throughput_simd, throughput_pc);
    }

    // 清理
    free(line_num_simd);
    free(line_num_pc);

    return 0;
}

/*
 * 块大小性能测试程序
 * 测试不同块大小对文件读取性能的影响
 */

#include "filelines_blocksize.h"
#include "find_most_freq.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

// 测试的块大小配置
struct BlockSizeConfig {
    size_t size;
    const char* name;
};

// 定义要测试的块大小
BlockSizeConfig block_sizes[] = {
    {64, "64B"},
    {256, "256B"},
    {1024, "1KB"},
    {4096, "4KB"},
    {16384, "16KB"},
    {65536, "64KB"},
    {262144, "256KB"},
    {1048576, "1MB"},
    {4194304, "4MB"},
};

const int NUM_BLOCK_SIZES = sizeof(block_sizes) / sizeof(block_sizes[0]);

void print_header() {
    cout << "\n========================================" << endl;
    cout << "    文件块大小性能测试" << endl;
    cout << "========================================\n" << endl;
}

void print_result_header() {
    cout << left << setw(12) << "块大小" << setw(18) << "执行时间(秒)" << setw(15) << "吞吐量(MB/s)" << setw(12)
         << "总行数" << setw(15) << "最频繁长度" << setw(15) << "出现次数" << endl;
    cout << string(85, '-') << endl;
}

void test_block_size(char* filepath, size_t block_size, const char* size_name) {
    uint32_t line_num[MAX_LEN];
    uint32_t total_line_num = 0;

    // 初始化数组
    for (int i = 0; i < MAX_LEN; i++)
        line_num[i] = 0;

    // 计时开始
    auto start = chrono::high_resolution_clock::now();

    // 执行文件分析
    filelines_with_blocksize(filepath, &total_line_num, line_num, block_size);

    // 计时结束
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;

    // 查找最频繁的行长度
    uint32_t most_freq_len, most_freq_len_linenum;
    find_most_freq_line(line_num, &most_freq_len, &most_freq_len_linenum);

    // 获取文件大小（用于计算吞吐量）
    FILE* fp = fopen(filepath, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fclose(fp);

        double throughput = (file_size / (1024.0 * 1024.0)) / duration.count();

        // 输出结果
        cout << left << setw(12) << size_name << fixed << setprecision(4) << setw(18) << duration.count()
             << setprecision(2) << setw(15) << throughput << setw(12) << total_line_num << setw(15) << most_freq_len
             << most_freq_len_linenum << endl;
    } else {
        // 如果无法获取文件大小，只输出时间和统计信息
        cout << left << setw(12) << size_name << fixed << setprecision(4) << setw(18) << duration.count() << setw(15)
             << "N/A" << setw(12) << total_line_num << setw(15) << most_freq_len << most_freq_len_linenum << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <filepath>\n", argv[0]);
        fprintf(stderr, "示例: %s test_2gb.txt\n", argv[0]);
        return 1;
    }

    char* filepath = argv[1];

    // 检查文件是否存在
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "错误: 无法打开文件 '%s'\n", filepath);
        return 1;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fclose(fp);

    print_header();
    cout << "测试文件: " << filepath << endl;
    cout << "文件大小: " << fixed << setprecision(2) << (file_size / (1024.0 * 1024.0 * 1024.0)) << " GB (" << file_size
         << " 字节)" << endl;
    cout << "测试块大小数量: " << NUM_BLOCK_SIZES << endl;
    cout << "\n开始测试...\n" << endl;

    print_result_header();

    // 测试每个块大小
    for (int i = 0; i < NUM_BLOCK_SIZES; i++) {
        test_block_size(filepath, block_sizes[i].size, block_sizes[i].name);
        cout.flush();
    }

    cout << "\n测试完成！" << endl;
    cout << "========================================\n" << endl;

    return 0;
}

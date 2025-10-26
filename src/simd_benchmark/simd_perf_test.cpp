/*
 * SIMD优化性能测试程序
 * 对比：标量版本（basic_benchmark） vs SIMD优化版本
 * 块大小：256KB（与basic_benchmark一致）
 */

#include "filelines_baseline.h"
#include "filelines_simd_opt.h"
#include "find_most_freq.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

using namespace std;

struct TestResult {
    double time_seconds;
    double throughput_mb_s;
    uint32_t total_lines;
    uint32_t most_freq_len;
    uint32_t most_freq_count;
};

TestResult run_test(char* filepath,
                   void (*test_func)(char*, uint32_t*, uint32_t*),
                   const char* version_name) {
    uint32_t line_num[MAX_LEN];
    uint32_t total_line_num = 0;

    // 初始化
    memset(line_num, 0, sizeof(line_num));

    cout << "  运行 " << version_name << "..." << flush;

    // 计时
    auto start = chrono::high_resolution_clock::now();
    test_func(filepath, &total_line_num, line_num);
    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> duration = end - start;

    // 查找最频繁的行长度
    uint32_t most_freq_len, most_freq_len_linenum;
    find_most_freq_line(line_num, &most_freq_len, &most_freq_len_linenum);

    // 获取文件大小
    FILE* fp = fopen(filepath, "rb");
    long file_size = 0;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fclose(fp);
    }

    TestResult result;
    result.time_seconds = duration.count();
    result.throughput_mb_s = (file_size / (1024.0 * 1024.0)) / duration.count();
    result.total_lines = total_line_num;
    result.most_freq_len = most_freq_len;
    result.most_freq_count = most_freq_len_linenum;

    cout << " 完成 (" << fixed << setprecision(3) << duration.count() << "s)" << endl;

    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <filepath>\n", argv[0]);
        fprintf(stderr, "示例: %s test_2gb.txt\n", argv[0]);
        return 1;
    }

    char* filepath = argv[1];

    // 检查文件
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "错误: 无法打开文件 '%s'\n", filepath);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fclose(fp);

    cout << "\n========================================" << endl;
    cout << "      SIMD优化性能测试" << endl;
    cout << "========================================\n" << endl;

    cout << "测试文件: " << filepath << endl;
    cout << "文件大小: " << fixed << setprecision(2)
         << (file_size / (1024.0 * 1024.0 * 1024.0)) << " GB" << endl;
    cout << "块大小: 256 KB\n" << endl;

    cout << "运行测试（每个版本测试3次取平均）...\n" << endl;

    // 测试3次取平均
    TestResult baseline_results[3], simd_results[3];

    for (int i = 0; i < 3; i++) {
        cout << "[测试轮次 " << (i + 1) << "/3]" << endl;
        baseline_results[i] = run_test(filepath, filelines_baseline, "标量版本");
        simd_results[i] = run_test(filepath, filelines_simd, "SIMD版本");
        cout << endl;
    }

    // 计算平均值
    TestResult baseline_avg = {0}, simd_avg = {0};

    for (int i = 0; i < 3; i++) {
        baseline_avg.time_seconds += baseline_results[i].time_seconds / 3.0;
        baseline_avg.throughput_mb_s += baseline_results[i].throughput_mb_s / 3.0;
        simd_avg.time_seconds += simd_results[i].time_seconds / 3.0;
        simd_avg.throughput_mb_s += simd_results[i].throughput_mb_s / 3.0;
    }

    baseline_avg.total_lines = baseline_results[0].total_lines;
    baseline_avg.most_freq_len = baseline_results[0].most_freq_len;
    baseline_avg.most_freq_count = baseline_results[0].most_freq_count;
    simd_avg.total_lines = simd_results[0].total_lines;
    simd_avg.most_freq_len = simd_results[0].most_freq_len;
    simd_avg.most_freq_count = simd_results[0].most_freq_count;

    cout << "========================================" << endl;
    cout << "          平均测试结果" << endl;
    cout << "========================================\n" << endl;

    cout << left << setw(20) << "版本"
         << setw(15) << "时间(秒)"
         << setw(18) << "吞吐量(MB/s)"
         << setw(12) << "总行数" << endl;
    cout << string(65, '-') << endl;

    cout << left << setw(20) << "标量版本"
         << fixed << setprecision(4) << setw(15) << baseline_avg.time_seconds
         << setprecision(2) << setw(18) << baseline_avg.throughput_mb_s
         << setw(12) << baseline_avg.total_lines << endl;

    cout << left << setw(20) << "SIMD版本 (AVX2)"
         << fixed << setprecision(4) << setw(15) << simd_avg.time_seconds
         << setprecision(2) << setw(18) << simd_avg.throughput_mb_s
         << setw(12) << simd_avg.total_lines << endl;

    cout << string(65, '-') << endl;

    // 计算性能提升
    double speedup = baseline_avg.time_seconds / simd_avg.time_seconds;
    double throughput_improvement =
        (simd_avg.throughput_mb_s - baseline_avg.throughput_mb_s) / baseline_avg.throughput_mb_s * 100.0;

    cout << "\n性能分析:" << endl;
    cout << "  加速比: " << fixed << setprecision(2) << speedup << "x";
    if (speedup > 1.0) {
        cout << " (SIMD更快 " << setprecision(1) << (speedup - 1.0) * 100 << "%)" << endl;
    } else {
        cout << " (标量更快 " << setprecision(1) << (1.0 / speedup - 1.0) * 100 << "%)" << endl;
    }
    cout << "  吞吐量提升: " << setprecision(1) << throughput_improvement << "%" << endl;
    cout << "  时间节省: " << setprecision(3)
         << (baseline_avg.time_seconds - simd_avg.time_seconds) << " 秒" << endl;

    // 验证结果一致性
    bool results_match = (baseline_avg.total_lines == simd_avg.total_lines) &&
                        (baseline_avg.most_freq_len == simd_avg.most_freq_len) &&
                        (baseline_avg.most_freq_count == simd_avg.most_freq_count);

    cout << "\n结果验证:" << endl;
    cout << "  总行数: " << baseline_avg.total_lines
         << (results_match ? " ✓" : " ✗") << endl;
    cout << "  最频繁长度: " << baseline_avg.most_freq_len
         << " (出现 " << baseline_avg.most_freq_count << " 次)"
         << (results_match ? " ✓" : " ✗") << endl;
    cout << "  数据一致性: " << (results_match ? "通过 ✓" : "失败 ✗") << endl;

    cout << "\n========================================\n" << endl;

    return results_match ? 0 : 1;
}

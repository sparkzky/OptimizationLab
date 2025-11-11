/*
 * 多线程性能对比测试程序
 * 对比：标量 vs 单线程SIMD vs 多线程SIMD（生产者-消费者）
 */

#include "filelines_baseline.h"
#include "filelines_mt.h"
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

TestResult run_test(char* filepath, void (*test_func)(char*, uint32_t*, uint32_t*), const char* version_name) {
    uint32_t line_num[MAX_LEN];
    uint32_t total_line_num = 0;

    memset(line_num, 0, sizeof(line_num));

    cout << "  测试 " << version_name << "..." << flush;

    auto start = chrono::high_resolution_clock::now();
    test_func(filepath, &total_line_num, line_num);
    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> duration = end - start;

    uint32_t most_freq_len, most_freq_len_linenum;
    find_most_freq_line(line_num, &most_freq_len, &most_freq_len_linenum);

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

    // 这个地方也非常神奇，你要是改用下面的cout，统计出来的执行时间就会变长:)
    cout << " 完成 (" << fixed << setprecision(3) << duration.count() << "s)" << endl;
    //    cout << " 完成 (" << fixed << setprecision(3) << duration.count() << "s, " << setprecision(1)
    //         << result.throughput_mb_s << " MB/s)" << endl;

    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <filepath>\n", argv[0]);
        fprintf(stderr, "示例: %s test_2gb.txt\n", argv[0]);
        return 1;
    }

    char* filepath = argv[1];

    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "错误: 无法打开文件 '%s'\n", filepath);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fclose(fp);

    cout << "\n========================================" << endl;
    cout << "    多线程优化性能对比测试" << endl;
    cout << "========================================\n" << endl;

    cout << "测试文件: " << filepath << endl;
    cout << "文件大小: " << fixed << setprecision(2) << (file_size / (1024.0 * 1024.0 * 1024.0)) << " GB" << endl;
    cout << "块大小: 256 KB" << endl;
    cout << "缓冲队列: 4 块\n" << endl;

    cout << "运行测试（每个版本测试3次取平均）...\n" << endl;

    // 测试3次取平均
    TestResult baseline_results[3], simd_results[3], mt_results[3];

    for (int i = 0; i < 3; i++) {
        cout << "[轮次 " << (i + 1) << "/3]" << endl;
        baseline_results[i] = run_test(filepath, filelines_baseline, "标量版本        ");
        simd_results[i] = run_test(filepath, filelines_simd, "单线程SIMD版本  ");
        mt_results[i] = run_test(filepath, filelines_mt, "多线程SIMD版本  ");
        cout << endl;
    }

    // 计算平均值
    TestResult baseline_avg = {0}, simd_avg = {0}, mt_avg = {0};

    for (int i = 0; i < 3; i++) {
        baseline_avg.time_seconds += baseline_results[i].time_seconds / 3.0;
        baseline_avg.throughput_mb_s += baseline_results[i].throughput_mb_s / 3.0;
        simd_avg.time_seconds += simd_results[i].time_seconds / 3.0;
        simd_avg.throughput_mb_s += simd_results[i].throughput_mb_s / 3.0;
        mt_avg.time_seconds += mt_results[i].time_seconds / 3.0;
        mt_avg.throughput_mb_s += mt_results[i].throughput_mb_s / 3.0;
    }

    baseline_avg.total_lines = baseline_results[0].total_lines;
    baseline_avg.most_freq_len = baseline_results[0].most_freq_len;
    baseline_avg.most_freq_count = baseline_results[0].most_freq_count;
    simd_avg.total_lines = simd_results[0].total_lines;
    simd_avg.most_freq_len = simd_results[0].most_freq_len;
    simd_avg.most_freq_count = simd_results[0].most_freq_count;
    mt_avg.total_lines = mt_results[0].total_lines;
    mt_avg.most_freq_len = mt_results[0].most_freq_len;
    mt_avg.most_freq_count = mt_results[0].most_freq_count;

    cout << "========================================" << endl;
    cout << "          平均测试结果" << endl;
    cout << "========================================\n" << endl;

    cout << left << setw(25) << "版本" << setw(15) << "时间(秒)" << setw(18) << "吞吐量(MB/s)" << setw(12) << "加速比"
         << endl;
    cout << string(70, '-') << endl;

    cout << left << setw(25) << "标量版本" << fixed << setprecision(4) << setw(15) << baseline_avg.time_seconds
         << setprecision(2) << setw(18) << baseline_avg.throughput_mb_s << "1.00x" << endl;

    double simd_speedup = baseline_avg.time_seconds / simd_avg.time_seconds;
    cout << left << setw(25) << "单线程SIMD版本" << fixed << setprecision(4) << setw(15) << simd_avg.time_seconds
         << setprecision(2) << setw(18) << simd_avg.throughput_mb_s << setprecision(2) << simd_speedup << "x" << endl;

    double mt_speedup = baseline_avg.time_seconds / mt_avg.time_seconds;
    cout << left << setw(25) << "多线程SIMD版本" << fixed << setprecision(4) << setw(15) << mt_avg.time_seconds
         << setprecision(2) << setw(18) << mt_avg.throughput_mb_s << setprecision(2) << mt_speedup << "x" << endl;

    cout << string(70, '-') << endl;

    // 详细性能分析
    cout << "\n性能提升分析:" << endl;
    cout << "  标量 → 单线程SIMD:" << endl;
    cout << "    加速比: " << fixed << setprecision(2) << simd_speedup << "x (" << setprecision(1)
         << (simd_speedup - 1.0) * 100 << "% 提升)" << endl;
    cout << "    吞吐量: " << setprecision(2) << baseline_avg.throughput_mb_s << " → " << simd_avg.throughput_mb_s
         << " MB/s" << endl;

    double mt_vs_simd = simd_avg.time_seconds / mt_avg.time_seconds;
    cout << "\n  单线程SIMD → 多线程SIMD:" << endl;
    cout << "    加速比: " << fixed << setprecision(2) << mt_vs_simd << "x (" << setprecision(1)
         << (mt_vs_simd - 1.0) * 100 << "% 提升)" << endl;
    cout << "    吞吐量: " << setprecision(2) << simd_avg.throughput_mb_s << " → " << mt_avg.throughput_mb_s << " MB/s"
         << endl;

    cout << "\n  标量 → 多线程SIMD (总提升):" << endl;
    cout << "    加速比: " << fixed << setprecision(2) << mt_speedup << "x (" << setprecision(1)
         << (mt_speedup - 1.0) * 100 << "% 提升)" << endl;
    cout << "    吞吐量: " << setprecision(2) << baseline_avg.throughput_mb_s << " → " << mt_avg.throughput_mb_s
         << " MB/s" << endl;

    // 验证结果一致性
    bool results_match = (baseline_avg.total_lines == simd_avg.total_lines) &&
                         (simd_avg.total_lines == mt_avg.total_lines) &&
                         (baseline_avg.most_freq_len == mt_avg.most_freq_len);

    cout << "\n结果验证:" << endl;
    cout << "  总行数: " << baseline_avg.total_lines << (results_match ? " ✓" : " ✗") << endl;
    cout << "  最频繁长度: " << baseline_avg.most_freq_len << " (出现 " << baseline_avg.most_freq_count << " 次)"
         << (results_match ? " ✓" : " ✗") << endl;
    cout << "  数据一致性: " << (results_match ? "通过 ✓" : "失败 ✗") << endl;

    cout << "\n========================================\n" << endl;

    return results_match ? 0 : 1;
}

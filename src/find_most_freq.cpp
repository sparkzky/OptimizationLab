#include "find_most_freq.h"
void find_most_freq_line(uint32_t* line_num, uint32_t* most_freq_len, uint32_t* most_freq_len_linenum) {
    int t_linenum = 0;
    int t_len = 0;
    for (int i = 0; i < MAX_LEN; i++) {
        if (line_num[i] > t_linenum) {
            t_linenum = line_num[i];
            t_len = i;
        }
    }
    *most_freq_len = t_len;
    *most_freq_len_linenum = t_linenum;
}


// TestResult run_test(char* filepath, void (*test_func)(char*, uint32_t*, uint32_t*), const char* version_name) {

//     uint32_t line_num[MAX_LEN];
//     uint32_t total_line_num = 0;

//     memset(line_num, 0, sizeof(line_num));

//     cout << "  测试 " << version_name << "..." << flush;

//     auto start = chrono::high_resolution_clock::now();
//     test_func(filepath, &total_line_num, line_num);
//     auto end = chrono::high_resolution_clock::now();

//     chrono::duration<double> duration = end - start;

//     uint32_t most_freq_len, most_freq_len_linenum;
//     find_most_freq_line(line_num, &most_freq_len, &most_freq_len_linenum);

//     FILE* fp = fopen(filepath, "rb");
//     long file_size = 0;
//     if (fp) {
//         fseek(fp, 0, SEEK_END);
//         file_size = ftell(fp);
//         fclose(fp);
//     }

//     TestResult result;
//     result.time_seconds = duration.count();
//     result.throughput_mb_s = (file_size / (1024.0 * 1024.0)) / duration.count();
//     result.total_lines = total_line_num;
//     result.most_freq_len = most_freq_len;
//     result.most_freq_count = most_freq_len_linenum;

//     // cout << " 完成 (" << fixed << setprecision(3) << duration.count() << "s)" << endl;
//         cout << " 完成 (" << fixed << setprecision(3) << duration.count() << "s, " << setprecision(1)
//              << result.throughput_mb_s << " MB/s)" << endl;

//     return result;
// }
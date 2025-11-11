#ifndef _FIND_MOST_FREQ_H
#define _FIND_MOST_FREQ_H

#include <stdint.h>
#define MAX_LEN 1024
void find_most_freq_line(uint32_t* line_num, uint32_t* most_freq_len, uint32_t* most_freq_len_linenum);
#endif

// #include <chrono>
// #include <iomanip>
// #include <iostream>
// #include <stdint.h>
// #include <stdio.h>
// #include <string.h>
// using namespace std;

// struct TestResult {
//     double time_seconds;
//     double throughput_mb_s;
//     uint32_t total_lines;
//     uint32_t most_freq_len;
//     uint32_t most_freq_count;
// };

// TestResult run_test(char* filepath, void (*test_func)(char*, uint32_t*, uint32_t*), const char* version_name) ;
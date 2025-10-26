#ifndef _FILELINES_SIMD_H
#define _FILELINES_SIMD_H

#include <stdint.h>

/**
 * SIMD优化版本的文件行分析函数（使用AVX2指令集）
 * 使用256KB块大小（与basic_benchmark一致）
 *
 * @param filepath 文件路径
 * @param total_line_num 输出：总行数
 * @param line_num 输出：各长度行的数量统计数组
 */
void filelines_simd(char* filepath, uint32_t* total_line_num, uint32_t* line_num);

#endif

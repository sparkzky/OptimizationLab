#ifndef _FILELINES_MT_H
#define _FILELINES_MT_H

#include <stdint.h>

/**
 * 多线程版本的文件行分析函数（生产者-消费者模型）
 * 使用256KB块大小
 *
 * 生产者线程：读取文件块
 * 消费者线程：使用SIMD方法统计
 *
 * @param filepath 文件路径
 * @param total_line_num 输出：总行数
 * @param line_num 输出：各长度行的数量统计数组
 */
void filelines_mt(char* filepath, uint32_t* total_line_num, uint32_t* line_num);

#endif

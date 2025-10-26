#ifndef _FILELINES_PRODUCER_CONSUMER_H
#define _FILELINES_PRODUCER_CONSUMER_H

#include <stdint.h>

/**
 * 使用生产者-消费者模型的文件行分析函数
 * 一个线程负责读取文件块（生产者）
 * 一个线程负责使用SIMD进行统计（消费者）
 *
 * @param filepath 文件路径
 * @param total_line_num 输出：总行数
 * @param line_num 输出：各长度行的数量统计数组
 */
void filelines_producer_consumer(char* filepath, uint32_t* total_line_num, uint32_t* line_num);

#endif

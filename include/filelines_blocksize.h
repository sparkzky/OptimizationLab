#ifndef _FILELINES_BLOCKSIZE_H
#define _FILELINES_BLOCKSIZE_H

#include <stdint.h>
#include <stddef.h>

/**
 * 支持可配置块大小的文件行分析函数
 *
 * @param filepath 文件路径
 * @param total_line_num 输出：总行数
 * @param line_num 输出：各长度行的数量统计数组
 * @param block_size 块大小（字节）
 */
void filelines_with_blocksize(char* filepath, uint32_t* total_line_num,
                               uint32_t* line_num, size_t block_size);

#endif

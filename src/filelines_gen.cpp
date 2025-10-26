/*
 * 测试文件生成器
 * 用法: filelines_gen <filepath> <size_in_gb>
 * 功能: 生成指定大小(GB)的文本文件，每行长度随机(1-100字符)
 */

#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MIN_LINE_LEN 1
#define MAX_LINE_LEN 100
#define BUFFER_SIZE (1024 * 1024) // 1MB 缓冲区

// 字符集：数字 + 大小写字母
const char CHARSET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const int CHARSET_SIZE = sizeof(CHARSET) - 1;

// 生成随机长度（1-100之间）
inline int random_line_length() { return MIN_LINE_LEN + rand() % MAX_LINE_LEN; }

// 生成随机字符
inline char random_char() { return CHARSET[rand() % CHARSET_SIZE]; }

int main(int argc, char* argv[]) {
    // 参数检查
    if (argc != 3) {
        fprintf(stderr, "用法: %s <filepath> <size_in_gb>\n", argv[0]);
        fprintf(stderr, "示例: %s test.txt 1.5\n", argv[0]);
        return 1;
    }

    char* filepath = argv[1];
    double size_in_gb = atof(argv[2]);

    if (size_in_gb <= 0) {
        fprintf(stderr, "错误: 文件大小必须为正数\n");
        return 1;
    }

    // 计算目标字节数
    uint64_t target_bytes = (uint64_t)(size_in_gb * 1024 * 1024 * 1024);

    printf("开始生成测试文件...\n");
    printf("文件路径: %s\n", filepath);
    printf("目标大小: %.2f GB (%" PRIu64 " 字节)\n", size_in_gb, target_bytes);

    // 打开文件
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("打开文件失败");
        return 1;
    }

    // 初始化随机数种子
    srand((unsigned int)time(NULL));

    // 分配缓冲区
    char* buffer = (char*)malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "内存分配失败\n");
        close(fd);
        return 1;
    }

    uint64_t bytes_written = 0;
    uint64_t lines_written = 0;
    int buffer_pos = 0;

    // 进度显示
    uint64_t progress_step = target_bytes / 100; // 每1%显示一次
    uint64_t next_progress = progress_step;
    int last_percent = 0;

    while (bytes_written < target_bytes) {
        // 生成一行
        int line_len = random_line_length();

        // 检查缓冲区空间
        if (buffer_pos + line_len + 1 > BUFFER_SIZE) {
            // 缓冲区满，写入文件
            ssize_t written = write(fd, buffer, buffer_pos);
            if (written < 0) {
                perror("写入文件失败");
                free(buffer);
                close(fd);
                return 1;
            }
            bytes_written += written;
            buffer_pos = 0;
        }

        // 生成随机字符到缓冲区
        for (int i = 0; i < line_len; i++) {
            buffer[buffer_pos++] = random_char();
        }
        // 添加换行符
        buffer[buffer_pos++] = '\n';
        lines_written++;

        // 显示进度
        if (bytes_written >= next_progress) {
            int percent = (int)(bytes_written * 100 / target_bytes);
            if (percent > last_percent) {
                printf("\r进度: %d%% (%.2f MB / %.2f MB)", percent, bytes_written / (1024.0 * 1024),
                       target_bytes / (1024.0 * 1024));
                fflush(stdout);
                last_percent = percent;
            }
            next_progress += progress_step;
        }
    }

    // 写入剩余缓冲区内容
    if (buffer_pos > 0) {
        ssize_t written = write(fd, buffer, buffer_pos);
        if (written < 0) {
            perror("写入文件失败");
            free(buffer);
            close(fd);
            return 1;
        }
        bytes_written += written;
    }

    // 清理资源
    free(buffer);
    close(fd);

    printf("\n\n生成完成！\n");
    printf("实际大小: %.2f GB (%" PRIu64 " 字节)\n", bytes_written / (1024.0 * 1024 * 1024), bytes_written);
    printf("总行数: %" PRIu64 "\n", lines_written);
    printf("平均行长度: %.2f 字符\n", (double)bytes_written / lines_written - 1);

    return 0;
}

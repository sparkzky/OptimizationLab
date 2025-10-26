#include "filelines_mt.h"
#include "find_most_freq.h"

#include <fcntl.h>
#include <immintrin.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLOCK_SIZE 262144  // 256KB
#define BUFFER_QUEUE_SIZE 4  // 缓冲队列大小

// 数据块结构
struct DataBlock {
    char* data;
    ssize_t size;
    bool valid;
};

// 生产者-消费者共享数据结构
struct SharedData {
    DataBlock queue[BUFFER_QUEUE_SIZE];
    int write_pos;
    int read_pos;
    int count;
    bool producer_done;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

    // 统计数据
    uint32_t* total_line_num;
    uint32_t* line_num;
    int cur_len;

    // 文件路径
    char* filepath;
};

// SIMD处理函数（与之前相同）
static inline void process_block_simd_opt(const char* buffer, ssize_t size,
                                          uint32_t* total_line_num, uint32_t* line_num,
                                          int* cur_len) {
    ssize_t i = 0;
    const __m256i newline = _mm256_set1_epi8('\n');

    while (i + 32 <= size) {
        __m256i chunk = _mm256_loadu_si256((__m256i*)(buffer + i));
        __m256i cmp = _mm256_cmpeq_epi8(chunk, newline);
        uint32_t mask = _mm256_movemask_epi8(cmp);

        if (__builtin_expect(mask == 0, 0)) {
            *cur_len += 32;
            i += 32;
        } else {
            for (int j = 0; j < 32; j++) {
                if (buffer[i + j] == '\n') {
                    (*total_line_num)++;
                    if (*cur_len >= MAX_LEN)
                        line_num[MAX_LEN - 1]++;
                    else
                        line_num[*cur_len]++;
                    *cur_len = 0;
                } else {
                    (*cur_len)++;
                }
            }
            i += 32;
        }
    }

    while (i < size) {
        if (buffer[i] == '\n') {
            (*total_line_num)++;
            if (*cur_len >= MAX_LEN)
                line_num[MAX_LEN - 1]++;
            else
                line_num[*cur_len]++;
            *cur_len = 0;
        } else {
            (*cur_len)++;
        }
        i++;
    }
}

// 生产者线程：读取文件块
void* producer_thread(void* arg) {
    SharedData* shared = (SharedData*)arg;
    char* filepath = shared->filepath;

    int handle = open(filepath, O_RDONLY);
    if (handle < 0) {
        pthread_mutex_lock(&shared->mutex);
        shared->producer_done = true;
        pthread_cond_signal(&shared->not_empty);
        pthread_mutex_unlock(&shared->mutex);
        return NULL;
    }

    while (1) {
        // 分配缓冲区
        char* buffer = (char*)aligned_alloc(32, BLOCK_SIZE);
        if (!buffer) break;

        // 读取数据
        ssize_t bytes_read = read(handle, buffer, BLOCK_SIZE);
        if (bytes_read <= 0) {
            free(buffer);
            break;
        }

        // 等待队列有空间
        pthread_mutex_lock(&shared->mutex);
        while (shared->count >= BUFFER_QUEUE_SIZE) {
            pthread_cond_wait(&shared->not_full, &shared->mutex);
        }

        // 将数据块放入队列
        shared->queue[shared->write_pos].data = buffer;
        shared->queue[shared->write_pos].size = bytes_read;
        shared->queue[shared->write_pos].valid = true;

        shared->write_pos = (shared->write_pos + 1) % BUFFER_QUEUE_SIZE;
        shared->count++;

        pthread_cond_signal(&shared->not_empty);
        pthread_mutex_unlock(&shared->mutex);
    }

    // 标记生产者完成
    pthread_mutex_lock(&shared->mutex);
    shared->producer_done = true;
    pthread_cond_signal(&shared->not_empty);
    pthread_mutex_unlock(&shared->mutex);

    close(handle);
    return NULL;
}

// 消费者线程：SIMD统计
void* consumer_thread(void* arg) {
    SharedData* shared = (SharedData*)arg;

    while (1) {
        pthread_mutex_lock(&shared->mutex);

        // 等待数据可用或生产者完成
        while (shared->count == 0 && !shared->producer_done) {
            pthread_cond_wait(&shared->not_empty, &shared->mutex);
        }

        // 检查是否已完成
        if (shared->count == 0 && shared->producer_done) {
            pthread_mutex_unlock(&shared->mutex);
            break;
        }

        // 从队列取出数据块
        DataBlock block = shared->queue[shared->read_pos];
        shared->queue[shared->read_pos].valid = false;
        shared->read_pos = (shared->read_pos + 1) % BUFFER_QUEUE_SIZE;
        shared->count--;

        pthread_cond_signal(&shared->not_full);
        pthread_mutex_unlock(&shared->mutex);

        // 处理数据块（不需要持有锁）
        process_block_simd_opt(block.data, block.size,
                              shared->total_line_num, shared->line_num,
                              &shared->cur_len);

        // 释放缓冲区
        free(block.data);
    }

    return NULL;
}

void filelines_mt(char* filepath, uint32_t* total_line_num, uint32_t* line_num) {
    // 初始化共享数据
    SharedData shared;
    memset(&shared, 0, sizeof(SharedData));

    shared.filepath = filepath;
    shared.total_line_num = total_line_num;
    shared.line_num = line_num;
    shared.cur_len = 0;
    shared.producer_done = false;
    shared.write_pos = 0;
    shared.read_pos = 0;
    shared.count = 0;

    pthread_mutex_init(&shared.mutex, NULL);
    pthread_cond_init(&shared.not_empty, NULL);
    pthread_cond_init(&shared.not_full, NULL);

    // 创建线程
    pthread_t producer, consumer;

    pthread_create(&producer, NULL, producer_thread, &shared);
    pthread_create(&consumer, NULL, consumer_thread, &shared);

    // 等待线程完成
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    // 清理
    pthread_mutex_destroy(&shared.mutex);
    pthread_cond_destroy(&shared.not_empty);
    pthread_cond_destroy(&shared.not_full);
}

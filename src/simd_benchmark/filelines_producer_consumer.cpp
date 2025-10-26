#include "filelines_producer_consumer.h"

#include "find_most_freq.h"

#include <fcntl.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#define BLOCK_SIZE 262144    // 256KB - 与basic_benchmark一致
#define QUEUE_MAX_SIZE 4     // 队列最大容量，避免生产者读取过快占用过多内存

// 数据块结构
struct DataBlock {
    char* data;              // 数据指针（对齐分配）
    ssize_t size;            // 实际数据大小
    bool is_end;             // 是否为结束标志

    DataBlock() : data(nullptr), size(0), is_end(false) {}
    DataBlock(char* d, ssize_t s, bool end = false)
        : data(d), size(s), is_end(end) {}
};

// 线程安全的阻塞队列
class BlockQueue {
private:
    std::queue<DataBlock> queue_;
    std::mutex mutex_;
    std::condition_variable cv_producer_;  // 生产者条件变量
    std::condition_variable cv_consumer_;  // 消费者条件变量
    size_t max_size_;
    bool finished_;  // 生产者是否完成

public:
    BlockQueue(size_t max_size) : max_size_(max_size), finished_(false) {}

    // 生产者：添加数据块到队列
    void push(const DataBlock& block) {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待队列有空间
        cv_producer_.wait(lock, [this] { return queue_.size() < max_size_; });
        queue_.push(block);
        cv_consumer_.notify_one();  // 通知消费者
    }

    // 消费者：从队列取出数据块
    bool pop(DataBlock& block) {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待队列非空或生产者完成
        cv_consumer_.wait(lock, [this] { return !queue_.empty() || finished_; });

        if (queue_.empty()) {
            return false;  // 队列空且生产者已完成
        }

        block = queue_.front();
        queue_.pop();
        cv_producer_.notify_one();  // 通知生产者
        return true;
    }

    // 标记生产者已完成
    void set_finished() {
        std::unique_lock<std::mutex> lock(mutex_);
        finished_ = true;
        cv_consumer_.notify_all();  // 通知所有等待的消费者
    }
};

// 优化的SIMD处理函数（与原版相同）
static inline void
process_block_simd_opt(const char* buffer, ssize_t size, uint32_t* total_line_num, uint32_t* line_num, int* cur_len) {
    ssize_t i = 0;
    const __m256i newline = _mm256_set1_epi8('\n');

    // SIMD处理：每次处理32字节
    while (i + 32 <= size) {
        __m256i chunk = _mm256_loadu_si256((__m256i*)(buffer + i));
        __m256i cmp = _mm256_cmpeq_epi8(chunk, newline);
        uint32_t mask = _mm256_movemask_epi8(cmp);

        if (__builtin_expect(mask == 0, 0)) {
            // 快速路径：没有换行符
            *cur_len += 32;
            i += 32;
        } else {
            // 有换行符，逐字节处理这32字节
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

    // 处理剩余字节
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

// 生产者线程函数：读取文件块
void producer_thread(const char* filepath, BlockQueue* queue) {
    int handle;
    if ((handle = open(filepath, O_RDONLY)) < 0) {
        queue->set_finished();
        return;
    }

    while (true) {
        // 对齐分配以提升SIMD性能
        char* buffer = (char*)aligned_alloc(32, BLOCK_SIZE);
        if (buffer == NULL) {
            break;
        }

        ssize_t bytes_read = read(handle, buffer, BLOCK_SIZE);
        if (bytes_read <= 0) {
            free(buffer);
            break;
        }

        // 将数据块添加到队列
        DataBlock block(buffer, bytes_read, false);
        queue->push(block);
    }

    close(handle);
    queue->set_finished();  // 通知消费者生产已完成
}

// 消费者线程函数：SIMD统计
void consumer_thread(BlockQueue* queue, uint32_t* total_line_num, uint32_t* line_num) {
    int cur_len = 0;

    while (true) {
        DataBlock block;
        if (!queue->pop(block)) {
            break;  // 队列已空且生产者已完成
        }

        if (block.is_end) {
            break;
        }

        // 使用SIMD处理数据块
        process_block_simd_opt(block.data, block.size, total_line_num, line_num, &cur_len);

        // 释放数据块内存
        free(block.data);
    }
}

// 主函数：使用生产者-消费者模型
void filelines_producer_consumer(char* filepath, uint32_t* total_line_num, uint32_t* line_num) {
    // 创建线程安全队列
    BlockQueue queue(QUEUE_MAX_SIZE);

    // 创建生产者和消费者线程
    std::thread producer(producer_thread, filepath, &queue);
    std::thread consumer(consumer_thread, &queue, total_line_num, line_num);

    // 等待线程完成
    producer.join();
    consumer.join();
}

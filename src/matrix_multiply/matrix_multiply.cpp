/*
矩阵乘法性能测试程序
包含基本矩阵乘法、分块矩阵乘法、SSE/AVX优化、以及多线程版本的实现和测试
*/

#include <chrono>
#include <cstdlib>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#define BLOCK_SIZE 64
#define THREAD_NUM 32

float rand_float(float s) { return 4.0f * s * (1.0f - s); }

void matrix_gen(float* a, float* b, int N, float seed) {
    float s = seed;
    // 使用 long long 避免当 N*N 很大时整数溢出
    long long size = (long long)N * N;
    for (long long i = 0; i < size; ++i) {
        s = rand_float(s);
        a[i] = s;
        s = rand_float(s);
        b[i] = s;
    }
}

float calculate_trace(const float* c, int N) {
    float trace = 0.0f;
    for (int i = 0; i < N; ++i) {
        trace += c[i * N + i];
    }
    return trace;
}

void clear_matrix(float* c, int N) {
    for (long long i = 0; i < (long long)N * N; ++i) {
        c[i] = 0.0f;
    }
}

template <typename MultiplyFunc>
void run_matrix_multiply_test(const std::string& name, int N, float seed, MultiplyFunc multiply_func) {
    std::cout << name << " N=" << N << " seed=" << seed << std::endl;

    std::vector<float> a((long long)N * N);
    std::vector<float> b((long long)N * N);
    std::vector<float> c((long long)N * N);

    matrix_gen(a.data(), b.data(), N, seed);

    auto start = std::chrono::high_resolution_clock::now();
    multiply_func(a.data(), b.data(), c.data(), N);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    float trace = calculate_trace(c.data(), N);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Trace: " << trace << std::endl;
    std::cout << "计算时间(s): " << duration.count() << std::endl;
}

void matrix_multiply(float* a, float* b, float* c, int N) {
    // for (int i = 0; i < N; ++i) {
    //     for (int j = 0; j < N; ++j) {
    //         float sum = 0.0f;
    //         for (int k = 0; k < N; ++k) {
    //             // 访问 A 的第 i 行第 k 列 和 B 的第 k 行第 j 列
    //             // 内存地址为 a[i*N + k] 和 b[k*N + j]
    //             sum += a[i * N + k] * b[k * N + j];
    //         }
    //         c[i * N + j] = sum;
    //     }
    // }
    for (int i = 0; i < N; ++i) {
        for (int k = 0; k < N; ++k) {
            // 将 A[i][k] 加载到寄存器中
            float a_val = a[i * N + k];
            for (int j = 0; j < N; ++j) {
                // 现在 B[k][j] 和 C[i][j] 都是连续访问
                c[i * N + j] += a_val * b[k * N + j];
            }
        }
    }
}

int basic_multiply(int n, float seed) {
    if (n <= 0 || seed <= 0.0f || seed >= 1.0f) {
        std::cerr << "错误: 参数无效。请确保 N > 0 且 0 < seed < 1。" << std::endl;
        return 1;
    }

    run_matrix_multiply_test(
        "Matrix_mul", n, seed, [](float* a, float* b, float* c, int N) { matrix_multiply(a, b, c, N); });

    return 0;
}

// 基本矩阵乘法测试
void test_basic_multiply(float seed = 0.12345f) {
    basic_multiply(512, seed);
    basic_multiply(1024, seed);
    basic_multiply(2048, seed);
    basic_multiply(4096, seed);
}

void matrix_multiply_blocked(float* a, float* b, float* c, int N, int m) {
    // clear_matrix(c, N);

    for (int i0 = 0; i0 < N; i0 += m) {
        for (int j0 = 0; j0 < N; j0 += m) {
            for (int k0 = 0; k0 < N; k0 += m) {
                int i_limit = std::min(i0 + m, N);
                for (int i = i0; i < i_limit; ++i) {
                    int k_limit = std::min(k0 + m, N);
                    for (int k = k0; k < k_limit; ++k) {
                        // 将a[i*N+k]加载到寄存器中
                        float a_val = a[i * N + k];
                        int j_limit = std::min(j0 + m, N);
                        for (int j = j0; j < j_limit; ++j) {
                            // a_val 在最内层循环中是常量
                            // b[k * N + j] 现在是顺序访问！
                            c[i * N + j] += a_val * b[k * N + j];
                        }
                    }
                }
            }
        }
    }
}

// 分块矩阵乘法测试
void blocked_multiply(int block_size, int N = 4096, float seed = 0.12345f) {
    run_matrix_multiply_test(
        "Blocked_multiply (block=" + std::to_string(block_size) + ")",
        N,
        seed,
        [block_size](float* a, float* b, float* c, int N) { matrix_multiply_blocked(a, b, c, N, block_size); });
}

void test_blocked_multiply() {
    blocked_multiply(8);
    blocked_multiply(16);
    blocked_multiply(32);
    blocked_multiply(64);
}

void matrix_multiply_blocked_sse(float* a, float* b, float* c, int N, int m) {
    // clear_matrix(c, N);

    for (int i0 = 0; i0 < N; i0 += m) {
        for (int j0 = 0; j0 < N; j0 += m) {
            for (int k0 = 0; k0 < N; k0 += m) {
                int i_limit = std::min(i0 + m, N);
                // i 以 2 为步长
                for (int i = i0; i < i_limit; i += 2) {

                    int j_limit = std::min(j0 + m, N);
                    // j 以 4 为步长
                    for (int j = j0; j < j_limit; j += 4) {

                        __m128 c_vec_0 = _mm_loadu_ps(&c[(i + 0) * N + j]);
                        __m128 c_vec_1 = _mm_loadu_ps(&c[(i + 1) * N + j]);

                        int k_limit = std::min(k0 + m, N);
                        for (int k = k0; k < k_limit; ++k) {
                            __m128 b_vec = _mm_loadu_ps(&b[k * N + j]);

                            __m128 a_val_0 = _mm_set1_ps(a[(i + 0) * N + k]);
                            __m128 a_val_1 = _mm_set1_ps(a[(i + 1) * N + k]);

                            c_vec_0 = _mm_add_ps(c_vec_0, _mm_mul_ps(a_val_0, b_vec));
                            c_vec_1 = _mm_add_ps(c_vec_1, _mm_mul_ps(a_val_1, b_vec));
                        }

                        _mm_storeu_ps(&c[(i + 0) * N + j], c_vec_0);
                        _mm_storeu_ps(&c[(i + 1) * N + j], c_vec_1);
                    }
                }
            }
        }
    }
}

void matrix_multiply_blocked_avx(float* a, float* b, float* c, int N, int m) {
    // clear_matrix(c, N);

    for (int i0 = 0; i0 < N; i0 += m) {
        for (int j0 = 0; j0 < N; j0 += m) {
            for (int k0 = 0; k0 < N; k0 += m) {
                int i_limit = std::min(i0 + m, N);
                // i 以 4 为步长，因为我们一次处理4行
                for (int i = i0; i < i_limit; i += 4) {

                    int j_limit = std::min(j0 + m, N);
                    // j 以 8 为步长，因为AVX一次处理8个float
                    for (int j = j0; j < j_limit; j += 8) {

                        // 1. 定义4个AVX寄存器作为 C[i:i+3][j:j+7] 的累加器
                        //    并加载C的当前值
                        __m256 c_vec_0 = _mm256_loadu_ps(&c[(i + 0) * N + j]);
                        __m256 c_vec_1 = _mm256_loadu_ps(&c[(i + 1) * N + j]);
                        __m256 c_vec_2 = _mm256_loadu_ps(&c[(i + 2) * N + j]);
                        __m256 c_vec_3 = _mm256_loadu_ps(&c[(i + 3) * N + j]);

                        int k_limit = std::min(k0 + m, N);
                        for (int k = k0; k < k_limit; ++k) {
                            // 2. 从B加载一个向量，这4行都会用到它
                            __m256 b_vec = _mm256_loadu_ps(&b[k * N + j]);

                            // 3. 从A加载4个值，并分别广播
                            __m256 a_val_0 = _mm256_set1_ps(a[(i + 0) * N + k]);
                            __m256 a_val_1 = _mm256_set1_ps(a[(i + 1) * N + k]);
                            __m256 a_val_2 = _mm256_set1_ps(a[(i + 2) * N + k]);
                            __m256 a_val_3 = _mm256_set1_ps(a[(i + 3) * N + k]);

                            // 4. 在寄存器中进行计算和累加
                            c_vec_0 = _mm256_add_ps(c_vec_0, _mm256_mul_ps(a_val_0, b_vec));
                            c_vec_1 = _mm256_add_ps(c_vec_1, _mm256_mul_ps(a_val_1, b_vec));
                            c_vec_2 = _mm256_add_ps(c_vec_2, _mm256_mul_ps(a_val_2, b_vec));
                            c_vec_3 = _mm256_add_ps(c_vec_3, _mm256_mul_ps(a_val_3, b_vec));
                        }

                        // 5. k循环结束后，将寄存器的结果写回内存
                        _mm256_storeu_ps(&c[(i + 0) * N + j], c_vec_0);
                        _mm256_storeu_ps(&c[(i + 1) * N + j], c_vec_1);
                        _mm256_storeu_ps(&c[(i + 2) * N + j], c_vec_2);
                        _mm256_storeu_ps(&c[(i + 3) * N + j], c_vec_3);
                    }
                }
            }
        }
    }
}

// 分块矩阵乘法测试 - SSE
void blocked_multiply_sse(int N = 4096, float seed = 0.12345f) {
    run_matrix_multiply_test("Blocked_multiply_SSE", N, seed, [](float* a, float* b, float* c, int N) {
        matrix_multiply_blocked_sse(a, b, c, N, BLOCK_SIZE);
    });
}

// 分块矩阵乘法测试 - AVX
void blocked_multiply_avx(int N = 4096, float seed = 0.12345f) {
    run_matrix_multiply_test("Blocked_multiply_AVX", N, seed, [](float* a, float* b, float* c, int N) {
        matrix_multiply_blocked_avx(a, b, c, N, BLOCK_SIZE);
    });
}

// 多线程版本的分块矩阵乘法 (AVX，不使用FMA)
void matrix_multiply_blocked_avx_mt_worker(float* a, float* b, float* c, int N, int m, int start_row, int end_row) {
    // // 每个线程处理从 start_row 到 end_row 的行
    // for (int i0 = start_row; i0 < end_row; i0 += m) {
    //     for (int j0 = 0; j0 < N; j0 += m) {
    //         for (int k0 = 0; k0 < N; k0 += m) {
    //             for (int i = i0; i < std::min(i0 + m, std::min(end_row, N)); ++i) {
    //                 for (int k = k0; k < std::min(k0 + m, N); ++k) {
    //                     __m256 a_val = _mm256_set1_ps(a[i * N + k]);
    //                     for (int j = j0; j < std::min(j0 + m, N); j += 8) {
    //                         __m256 b_vec = _mm256_loadu_ps(&b[k * N + j]);
    //                         __m256 c_vec = _mm256_loadu_ps(&c[i * N + j]);
    //                         c_vec = _mm256_add_ps(c_vec, _mm256_mul_ps(a_val, b_vec));
    //                         _mm256_storeu_ps(&c[i * N + j], c_vec);
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }

    // 外层 j0, k0 循环遍历整个矩阵
    for (int j0 = 0; j0 < N; j0 += m) {
        for (int k0 = 0; k0 < N; k0 += m) {

            // i0 循环只在线程负责的行范围内进行
            for (int i0 = start_row; i0 < end_row; i0 += m) {

                // --- matrix_multiply_blocked_avx 核心代码 ---
                int i_limit = std::min(i0 + m, end_row);
                int j_limit = std::min(j0 + m, N);
                int k_limit = std::min(k0 + m, N);

                // 计算能被4整除的安全边界
                int i_limit_safe = i_limit - (i_limit - i0) % 4;
                for (int i = i0; i < i_limit_safe; i += 4) {

                    for (int j = j0; j < j_limit; j += 8) {

                        __m256 c_vec_0 = _mm256_loadu_ps(&c[(i + 0) * N + j]);
                        __m256 c_vec_1 = _mm256_loadu_ps(&c[(i + 1) * N + j]);
                        __m256 c_vec_2 = _mm256_loadu_ps(&c[(i + 2) * N + j]);
                        __m256 c_vec_3 = _mm256_loadu_ps(&c[(i + 3) * N + j]);

                        for (int k = k0; k < k_limit; ++k) {
                            __m256 b_vec = _mm256_loadu_ps(&b[k * N + j]);

                            __m256 a_val_0 = _mm256_set1_ps(a[(i + 0) * N + k]);
                            __m256 a_val_1 = _mm256_set1_ps(a[(i + 1) * N + k]);
                            __m256 a_val_2 = _mm256_set1_ps(a[(i + 2) * N + k]);
                            __m256 a_val_3 = _mm256_set1_ps(a[(i + 3) * N + k]);

                            c_vec_0 = _mm256_add_ps(c_vec_0, _mm256_mul_ps(a_val_0, b_vec));
                            c_vec_1 = _mm256_add_ps(c_vec_1, _mm256_mul_ps(a_val_1, b_vec));
                            c_vec_2 = _mm256_add_ps(c_vec_2, _mm256_mul_ps(a_val_2, b_vec));
                            c_vec_3 = _mm256_add_ps(c_vec_3, _mm256_mul_ps(a_val_3, b_vec));
                        }

                        _mm256_storeu_ps(&c[(i + 0) * N + j], c_vec_0);
                        _mm256_storeu_ps(&c[(i + 1) * N + j], c_vec_1);
                        _mm256_storeu_ps(&c[(i + 2) * N + j], c_vec_2);
                        _mm256_storeu_ps(&c[(i + 3) * N + j], c_vec_3);
                    }
                }
                for (int i = i_limit_safe; i < i_limit; ++i) {
                    for (int j = j0; j < j_limit; j += 8) {
                        __m256 c_vec = _mm256_loadu_ps(&c[i * N + j]);
                        for (int k = k0; k < k_limit; ++k) {
                            __m256 a_val = _mm256_set1_ps(a[i * N + k]);
                            __m256 b_vec = _mm256_loadu_ps(&b[k * N + j]);
                            c_vec = _mm256_add_ps(c_vec, _mm256_mul_ps(a_val, b_vec));
                        }
                        _mm256_storeu_ps(&c[i * N + j], c_vec);
                    }
                }
            }
        }
    }
}

void matrix_multiply_blocked_avx_mt(float* a, float* b, float* c, int N, int m, int num_threads) {
    // clear_matrix(c, N);

    std::vector<std::thread> threads;
    int rows_per_thread = (N + num_threads - 1) / num_threads; // 向上取整

    // for (int t = 0; t < num_threads; ++t) {
    //     int start_row = t * rows_per_thread;
    //     int end_row = (t == num_threads - 1) ? N : (t + 1) * rows_per_thread;

    //     threads.emplace_back(matrix_multiply_blocked_avx_mt_worker, a, b, c, N, m, start_row, end_row);
    // }

    for (int t = 0; t < num_threads; ++t) {
        int start_row = t * rows_per_thread;
        int end_row = std::min((t + 1) * rows_per_thread, N);
        if (start_row >= end_row)
            continue;

        // 调用新的、高性能的 worker 函数！
        threads.emplace_back(matrix_multiply_blocked_avx_mt_worker, a, b, c, N, m, start_row, end_row);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

// 多线程分块矩阵乘法测试 - AVX
void blocked_multiply_avx_mt(int num_threads, int N = 4096, float seed = 0.12345f) {
    run_matrix_multiply_test("Blocked_multiply_AVX_MT (threads=" + std::to_string(num_threads) + ")",
                             N,
                             seed,
                             [num_threads](float* a, float* b, float* c, int N) {
                                 matrix_multiply_blocked_avx_mt(a, b, c, N, BLOCK_SIZE, num_threads);
                             });
}

// 测试不同线程数的性能
void test_multithreaded_performance(int N = 4096, float seed = 0.12345f) {
    std::cout << "\n========== 多线程性能对比测试 ==========" << std::endl;

    // 单线程基准
    std::cout << "--- 单线程（基准） ---" << std::endl;
    blocked_multiply_avx_mt(1, N, seed);

    // 测试不同线程数
    for (int threads : {2, 4, 6, 8, 12, 16, 32, 64}) {
        std::cout << "\n--- " << threads << " 线程 ---" << std::endl;
        blocked_multiply_avx_mt(threads, N, seed);
    }

    std::cout << "\n======================================" << std::endl;
}

void run_with_best(int N = 4096, float seed = 0.12345f) { blocked_multiply_avx_mt(THREAD_NUM, N, seed); }

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " [N] [seed]" << std::endl;
    std::cerr << "  Runs the best performing version (multithreaded AVX) with optional N and seed." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Or, run a specific test case:" << std::endl;
    std::cerr << "  " << prog_name << " --basic              - Runs the basic single-threaded matrix multiply test."
              << std::endl;
    std::cerr << "  " << prog_name << " --blocked            - Runs the single-threaded blocked matrix multiply test."
              << std::endl;
    std::cerr << "  " << prog_name << " --avx                 - Runs the single-threaded advanced AVX test."
              << std::endl;
    std::cerr << "  " << prog_name << " --sse                 - Runs the single-threaded advanced SSE test."
              << std::endl;
    std::cerr << "  " << prog_name << " --multithread-test    - Runs the multithreaded performance comparison."
              << std::endl;
    // std::cerr << "  " << prog_name << " --all                 - Runs all of the above tests." << std::endl;
    std::cerr << "  " << prog_name << " --help, -h            - Shows this help message." << std::endl;
}

int main(int argc, char** argv) {

    // 情况1: 没有提供任何参数，运行最佳版本
    if (argc == 1) {
        run_with_best();
        return 0;
    }

    std::string arg1 = argv[1];

    // 情况2: 第一个参数是 flag (以 '--' 或 '-' 开头)
    if (arg1.rfind("--", 0) == 0 || arg1.rfind("-", 0) == 0) {
        if (arg1 == "--avx") {
            blocked_multiply_avx();
        } else if (arg1 == "--sse") {
            blocked_multiply_sse();
        } else if (arg1 == "--multithread-test") {
            test_multithreaded_performance();
        } else if (arg1 == "--all") {
            std::cout << "--- Running Single-Threaded AVX Test ---" << std::endl;
            blocked_multiply_avx();
            std::cout << "\n--- Running Single-Threaded SSE Test ---" << std::endl;
            blocked_multiply_sse();
            test_multithreaded_performance();
        } else if (arg1 == "--help" || arg1 == "-h") {
            print_usage(argv[0]);
        } else if (arg1 == "--basic") {
            test_basic_multiply();
        } else if (arg1 == "--blocked") {
            test_blocked_multiply();

        } else {
            std::cerr << "Error: Unknown option '" << arg1 << "'" << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    } else {
        try {
            int n = 4096;
            float seed = 0.12345f;

            // 从命令行参数解析 N 和 seed
            if (argc >= 2) {
                n = std::stoi(argv[1]);
            }
            if (argc >= 3) {
                seed = std::stof(argv[2]);
            }
            // 如果有多余的参数，可以给个警告
            if (argc > 3) {
                std::cerr << "Warning: Extra arguments are ignored." << std::endl;
            }

            run_with_best(n, seed);

        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid arguments for N and seed. Please provide numbers." << std::endl;
            std::cerr << e.what() << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    return 0;
}

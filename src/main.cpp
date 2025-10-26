#include <chrono>
#include <cstdlib>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <vector>

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

void matrix_multiply(float* a, float* b, float* c, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < N; ++k) {
                // 访问 A 的第 i 行第 k 列 和 B 的第 k 行第 j 列
                // 内存地址为 a[i*N + k] 和 b[k*N + j]
                sum += a[i * N + k] * b[k * N + j];
            }
            c[i * N + j] = sum;
        }
    }
}

int basic_multiply(int n, float seed) {
    if (n <= 0 || seed <= 0.0f || seed >= 1.0f) {
        std::cerr << "错误: 参数无效。请确保 N > 0 且 0 < seed < 1。" << std::endl;
        return 1;
    }

    std::cout << "Matrix_mul N=" << n << " seed=" << seed << std::endl;

    std::vector<float> a((long long)n * n);
    std::vector<float> b((long long)n * n);
    std::vector<float> c((long long)n * n);

    matrix_gen(a.data(), b.data(), n, seed);

    auto start = std::chrono::high_resolution_clock::now();
    matrix_multiply(a.data(), b.data(), c.data(), n);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    float trace = 0.0f;
    for (int i = 0; i < n; ++i) {
        trace += c[i * n + i];
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Trace: " << trace << std::endl;
    std::cout << "计算时间(s): " << duration.count() << std::endl;

    return 0;
}

void test_basic_multiply(float seed = 0.12345f) {
    basic_multiply(512, seed);
    basic_multiply(1024, seed);
    basic_multiply(2048, seed);
    basic_multiply(4096, seed);
}

void matrix_multiply_blocked(float* a, float* b, float* c, int N, int m) {
    for (long long i = 0; i < (long long)N * N; ++i) {
        c[i] = 0.0f;
    }

    for (int i0 = 0; i0 < N; i0 += m) {
        for (int j0 = 0; j0 < N; j0 += m) {
            for (int k0 = 0; k0 < N; k0 += m) {
                // 内层循环在块内部进行常规的矩阵乘法
                // C_block[i0,j0] += A_block[i0,k0] * B_block[k0,j0]
                // 使用 std::min 处理边界情况，当 N 不能被 m 整除时
                int i_limit = std::min(i0 + m, N);
                for (int i = i0; i < i_limit; ++i) {
                    int j_limit = std::min(j0 + m, N);
                    for (int j = j0; j < j_limit; ++j) {
                        int k_limit = std::min(k0 + m, N);
                        // 优化：将 C[i*N+j] 的值加载到寄存器中，在内循环结束后写回
                        float sum_val = c[i * N + j];
                        for (int k = k0; k < k_limit; ++k) {
                            sum_val += a[i * N + k] * b[k * N + j];
                        }
                        c[i * N + j] = sum_val;
                    }
                }
            }
        }
    }
}

void blocked_multiply(int block_size, int N = 4096, float seed = 0.12345f) {
    std::vector<float> a((long long)N * N);
    std::vector<float> b((long long)N * N);
    std::vector<float> c((long long)N * N);

    matrix_gen(a.data(), b.data(), N, seed);

    auto start = std::chrono::high_resolution_clock::now();

    matrix_multiply_blocked(a.data(), b.data(), c.data(), N, block_size);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    float trace = 0.0f;
    for (int i = 0; i < N; ++i) {
        trace += c[i * N + i];
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Trace: " << trace << std::endl;
    std::cout << "计算时间(s): " << duration.count() << std::endl;
}

void test_blocked_multiply() {
    blocked_multiply(8);
    blocked_multiply(16);
    blocked_multiply(32);
    blocked_multiply(64);
}

void matrix_multiply_blocked_sse(float* a, float* b, float* c, int N, int m) {
    for (long long i = 0; i < (long long)N * N; ++i) {
        c[i] = 0.0f;
    }
    for (int i0 = 0; i0 < N; i0 += m) {
        for (int j0 = 0; j0 < N; j0 += m) {
            for (int k0 = 0; k0 < N; k0 += m) {
                // 在块内进行计算
                for (int i = i0; i < std::min(i0 + m, N); ++i) {
                    for (int k = k0; k < std::min(k0 + m, N); ++k) {
                        // 广播 A[i][k] 的值
                        __m128 a_val = _mm_set1_ps(a[i * N + k]);
                        // j 以4为步长进行向量化计算
                        for (int j = j0; j < std::min(j0 + m, N); j += 4) {
                            // 加载 B 和 C 的向量
                            __m128 b_vec = _mm_loadu_ps(&b[k * N + j]);
                            __m128 c_vec = _mm_loadu_ps(&c[i * N + j]);
                            // c_vec = c_vec + a_val * b_vec
                            c_vec = _mm_add_ps(c_vec, _mm_mul_ps(a_val, b_vec));
                            // 将结果写回
                            _mm_storeu_ps(&c[i * N + j], c_vec);
                        }
                    }
                }
            }
        }
    }
}

void matrix_multiply_blocked_avx(float* a, float* b, float* c, int N, int m) {
    for (long long i = 0; i < (long long)N * N; ++i) {
        c[i] = 0.0f;
    }

    for (int i0 = 0; i0 < N; i0 += m) {
        for (int j0 = 0; j0 < N; j0 += m) {
            for (int k0 = 0; k0 < N; k0 += m) {
                // 在块内进行计算
                for (int i = i0; i < std::min(i0 + m, N); ++i) {
                    for (int k = k0; k < std::min(k0 + m, N); ++k) {
                        // 广播 A[i][k] 的值
                        __m256 a_val = _mm256_set1_ps(a[i * N + k]);
                        // j 以8为步长进行向量化计算
                        for (int j = j0; j < std::min(j0 + m, N); j += 8) {
                            // 加载 B 和 C 的向量
                            __m256 b_vec = _mm256_loadu_ps(&b[k * N + j]);
                            __m256 c_vec = _mm256_loadu_ps(&c[i * N + j]);
                            // c_vec = c_vec + a_val * b_vec (使用FMA指令更高效)
                            c_vec = _mm256_fmadd_ps(a_val, b_vec, c_vec);
                            // 将结果写回
                            _mm256_storeu_ps(&c[i * N + j], c_vec);
                        }
                    }
                }
            }
        }
    }
}

void blocked_multiply_sse(int N = 4096, float seed = 0.12345f) {
    std::vector<float> a((long long)N * N);
    std::vector<float> b((long long)N * N);
    std::vector<float> c((long long)N * N);

    matrix_gen(a.data(), b.data(), N, seed);

    auto start = std::chrono::high_resolution_clock::now();

    matrix_multiply_blocked_sse(a.data(), b.data(), c.data(), N, 64);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    float trace = 0.0f;
    for (int i = 0; i < N; ++i) {
        trace += c[i * N + i];
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Trace: " << trace << std::endl;
    std::cout << "计算时间(s): " << duration.count() << std::endl;
}


void blocked_multiply_avx(int N = 4096, float seed = 0.12345f) {
    std::vector<float> a((long long)N * N);
    std::vector<float> b((long long)N * N);
    std::vector<float> c((long long)N * N);

    matrix_gen(a.data(), b.data(), N, seed);

    auto start = std::chrono::high_resolution_clock::now();

    matrix_multiply_blocked_avx(a.data(), b.data(), c.data(), N, 64);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    float trace = 0.0f;
    for (int i = 0; i < N; ++i) {
        trace += c[i * N + i];
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Trace: " << trace << std::endl;
    std::cout << "计算时间(s): " << duration.count() << std::endl;
}

int main(int argc, char** argv) {
    // test_basic_multiply();
    // test_blocked_multiply();
    blocked_multiply_avx();
    blocked_multiply_sse();


    return 0;
}

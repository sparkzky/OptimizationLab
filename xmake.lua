add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

-- 全局配置
set_optimize("fastest")
set_languages("c++17")
add_includedirs("./include")
-- 使用本机架构优化
add_cxflags("-march=native")

-- 矩阵乘法程序
target("matrix_multiply")
    set_kind("binary")
    add_files("src/matrix_multiply/matrix_multiply.cpp")
    add_cxflags("-msse", "-mavx", "-mfma")
    add_syslinks("pthread")

-- 文件行分析程序
target("filelines")
    set_kind("binary")
    add_files("src/basic_benchmark/filelines.cpp", "src/basic_benchmark/filelines_baseline.cpp", "src/find_most_freq.cpp","src/simd_benchmark/filelines_mt.cpp")

-- 测试文件生成器
target("filelines_gen")
    set_kind("binary")
    add_files("src/filelines_gen.cpp")

-- 块大小性能测试程序
target("blocksize_benchmark")
    set_kind("binary")
    add_files("src/blocksize_benchmark/blocksize_benchmark.cpp", "src/blocksize_benchmark/filelines_blocksize.cpp", "src/find_most_freq.cpp")

-- SIMD性能测试程序
target("simd_perf_test")
    set_kind("binary")
    add_files("src/simd_benchmark/simd_perf_test.cpp", "src/basic_benchmark/filelines_baseline.cpp", "src/simd_benchmark/filelines_simd_opt.cpp", "src/find_most_freq.cpp")
    add_cxflags("-mavx2", "-mfma")

-- 多线程SIMD性能测试程序（生产者-消费者模型）
target("mt_perf_test")
    set_kind("binary")
    add_files("src/simd_benchmark/mt_perf_test.cpp", "src/basic_benchmark/filelines_baseline.cpp", "src/simd_benchmark/filelines_simd_opt.cpp", "src/simd_benchmark/filelines_mt.cpp", "src/find_most_freq.cpp")
    add_cxflags("-mavx2", "-mfma")
    add_syslinks("pthread")

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--


/*********************************************************************************
* Copyright 2019 Huawei Technologies Co.,Ltd.
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use
* this file except in compliance with the License.  You may obtain a copy of the
* License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software distributed
* under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
* CONDITIONS OF ANY KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations under the License.
**********************************************************************************
*/
#include "crc64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include "securec.h"
#include "util.h"

/* GF(2) 向量维度（CRC 长度） */
#define GF2_DIM 64

/* CRC64 查找表 (8 层 x 256 项 = 16KB) */
static uint64_t crc64_table[8][256];

/* CRC64 合并矩阵（用于 4-way 并行流合并） */
static uint64_t crc64_even[GF2_DIM];
static uint64_t crc64_odd[GF2_DIM];

/* GF(2) 矩阵乘法 */
static uint64_t gf2_matrix_times(const uint64_t *mat, int mat_len, uint64_t vec)
{
    uint64_t sum = 0;
    int idx = 0;
    while (vec != 0 && idx < mat_len) {
        if ((vec & 1) == 1) {
            sum ^= mat[idx];
        }
        vec >>= 1;
        idx++;
    }
    return sum;
}

/* GF(2) 矩阵平方 */
static void gf2_matrix_square(uint64_t *square, int square_len, const uint64_t *mat, int mat_len)
{
    int n;
    for (n = 0; n < square_len && n < mat_len; n++) {
        square[n] = gf2_matrix_times(mat, mat_len, mat[n]);
    }
}

/* 初始化 CRC64 查找表和合并矩阵 */
static void crc64_init_tables(void)
{
    int n, k;
    uint64_t crc;
    uint64_t row;

    /* 生成基本查找表 */
    for (n = 0; n < 256; n++) {
        crc = (uint64_t)n;
        for (k = 0; k < 8; k++) {
            if ((crc & 1) == 1) {
                crc = (crc >> 1) ^ CRC64_POLY;
            } else {
                crc = (crc >> 1);
            }
        }
        crc64_table[0][n] = crc;
    }

    /* 生成嵌套查找表（slice-by-8） */
    for (n = 0; n < 256; n++) {
        crc = crc64_table[0][n];
        for (k = 1; k < 8; k++) {
            crc = crc64_table[0][crc & 0xff] ^ (crc >> 8);
            crc64_table[k][n] = crc;
        }
    }

    /* 初始化合并矩阵 */
    crc64_odd[0] = CRC64_POLY; /* CRC-64 多项式 */
    row = 1;
    for (n = 1; n < GF2_DIM; n++) {
        crc64_odd[n] = row;
        row <<= 1;
    }
    gf2_matrix_square(crc64_even, GF2_DIM, crc64_odd, GF2_DIM);
    gf2_matrix_square(crc64_odd, GF2_DIM, crc64_even, GF2_DIM);
}

/* 确保表只初始化一次 */
static void crc64_ensure_initialized(void)
{
    static int initialized = 0;
    if (!initialized) {
        crc64_init_tables();
        initialized = 1;
    }
}

/* 初始化 CRC64 上下文 */
void obs_crc64_internal_init(obs_crc64_internal *ctx)
{
    if (ctx == NULL) {
        return;
    }
    crc64_ensure_initialized();
    ctx->value = 0;
}

/* 重置 CRC64 上下文 */
void obs_crc64_internal_reset(obs_crc64_internal *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->value = 0;
}

/* 小数据量: 直接使用 slice-by-8，避免并行分割和合并开销 */
static uint64_t crc64_slice_by_8(uint64_t value, const unsigned char *buffer,
                                 size_t length)
{
    size_t idx = 0;
    while (length >= 8) {
        value =
            crc64_table[7][((value & 0xff) ^ buffer[idx]) & 0xff] ^
            crc64_table[6][(((value >> 8) & 0xff) ^ buffer[idx + 1]) & 0xff] ^
            crc64_table[5][(((value >> 16) & 0xff) ^ buffer[idx + 2]) & 0xff] ^
            crc64_table[4][(((value >> 24) & 0xff) ^ buffer[idx + 3]) & 0xff] ^
            crc64_table[3][(((value >> 32) & 0xff) ^ buffer[idx + 4]) & 0xff] ^
            crc64_table[2][(((value >> 40) & 0xff) ^ buffer[idx + 5]) & 0xff] ^
            crc64_table[1][(((value >> 48) & 0xff) ^ buffer[idx + 6]) & 0xff] ^
            crc64_table[0][((value >> 56) ^ buffer[idx + 7]) & 0xff];
        idx += 8;
        length -= 8;
    }
    while (length > 0) {
        value = crc64_table[0][((value ^ buffer[idx]) & 0xff)] ^ (value >> 8);
        idx++;
        length--;
    }
    return value;
}

/* 处理单个流的剩余字节(slice-by-8 + 逐字节) */
static void crc64_process_remaining(uint64_t *v, const unsigned char *p,
                                    size_t *idx, size_t *remaining)
{
    while (*remaining >= 8) {
        *v = crc64_table[7][((*v & 0xff) ^ p[*idx]) & 0xff] ^
             crc64_table[6][(((*v >> 8) & 0xff) ^ p[*idx + 1]) & 0xff] ^
             crc64_table[5][(((*v >> 16) & 0xff) ^ p[*idx + 2]) & 0xff] ^
             crc64_table[4][(((*v >> 24) & 0xff) ^ p[*idx + 3]) & 0xff] ^
             crc64_table[3][(((*v >> 32) & 0xff) ^ p[*idx + 4]) & 0xff] ^
             crc64_table[2][(((*v >> 40) & 0xff) ^ p[*idx + 5]) & 0xff] ^
             crc64_table[1][(((*v >> 48) & 0xff) ^ p[*idx + 6]) & 0xff] ^
             crc64_table[0][((*v >> 56) ^ p[*idx + 7]) & 0xff];
        *idx += 8; *remaining -= 8;
    }
    while (*remaining > 0) {
        *v = crc64_table[0][((*v ^ p[*idx]) & 0xff)] ^ (*v >> 8);
        (*idx)++; (*remaining)--;
    }
}

/* 大数据量: 4-way 并行流 */
static void crc64_4way_parallel(obs_crc64_internal *ctx,
                                const unsigned char *buffer, size_t length)
{
    size_t quarter = length / 4;
    size_t r0_len = quarter;
    size_t r1_len = quarter;
    size_t r2_len = quarter;
    size_t r3_len = length - 3 * quarter;

    const unsigned char *p0 = buffer;
    const unsigned char *p1 = buffer + quarter;
    const unsigned char *p2 = buffer + 2 * quarter;
    const unsigned char *p3 = buffer + 3 * quarter;

    /* 计算每段可执行的 slice-by-8 迭代次数 */
    size_t s8_0 = r0_len / 8;
    size_t s8_1 = r1_len / 8;
    size_t s8_2 = r2_len / 8;
    size_t s8_3 = r3_len / 8;

    /* 取最小值，保证所有流在交错循环中同步推进 */
    size_t min_chunks = s8_0;
    if (s8_1 < min_chunks) min_chunks = s8_1;
    if (s8_2 < min_chunks) min_chunks = s8_2;
    if (s8_3 < min_chunks) min_chunks = s8_3;

    /* 流 0 从当前 CRC 状态继续，流 1-3 从初始状态开始 */
    uint64_t v0 = ~ctx->value;
    uint64_t v1 = ~(uint64_t)0;
    uint64_t v2 = ~(uint64_t)0;
    uint64_t v3 = ~(uint64_t)0;

    size_t i0 = 0, i1 = 0, i2 = 0, i3 = 0;

    /* 交错主循环: 4 条独立依赖链并行执行 */
    for (size_t c = 0; c < min_chunks; c++) {
        v0 = crc64_table[7][(v0 & 0xff) ^ p0[i0]] ^
             crc64_table[6][((v0 >> 8) & 0xff) ^ p0[i0 + 1]] ^
             crc64_table[5][((v0 >> 16) & 0xff) ^ p0[i0 + 2]] ^
             crc64_table[4][((v0 >> 24) & 0xff) ^ p0[i0 + 3]] ^
             crc64_table[3][((v0 >> 32) & 0xff) ^ p0[i0 + 4]] ^
             crc64_table[2][((v0 >> 40) & 0xff) ^ p0[i0 + 5]] ^
             crc64_table[1][((v0 >> 48) & 0xff) ^ p0[i0 + 6]] ^
             crc64_table[0][((v0 >> 56) ^ p0[i0 + 7]) & 0xff];

        v1 = crc64_table[7][(v1 & 0xff) ^ p1[i1]] ^
             crc64_table[6][((v1 >> 8) & 0xff) ^ p1[i1 + 1]] ^
             crc64_table[5][((v1 >> 16) & 0xff) ^ p1[i1 + 2]] ^
             crc64_table[4][((v1 >> 24) & 0xff) ^ p1[i1 + 3]] ^
             crc64_table[3][((v1 >> 32) & 0xff) ^ p1[i1 + 4]] ^
             crc64_table[2][((v1 >> 40) & 0xff) ^ p1[i1 + 5]] ^
             crc64_table[1][((v1 >> 48) & 0xff) ^ p1[i1 + 6]] ^
             crc64_table[0][((v1 >> 56) ^ p1[i1 + 7]) & 0xff];

        v2 = crc64_table[7][(v2 & 0xff) ^ p2[i2]] ^
             crc64_table[6][((v2 >> 8) & 0xff) ^ p2[i2 + 1]] ^
             crc64_table[5][((v2 >> 16) & 0xff) ^ p2[i2 + 2]] ^
             crc64_table[4][((v2 >> 24) & 0xff) ^ p2[i2 + 3]] ^
             crc64_table[3][((v2 >> 32) & 0xff) ^ p2[i2 + 4]] ^
             crc64_table[2][((v2 >> 40) & 0xff) ^ p2[i2 + 5]] ^
             crc64_table[1][((v2 >> 48) & 0xff) ^ p2[i2 + 6]] ^
             crc64_table[0][((v2 >> 56) ^ p2[i2 + 7]) & 0xff];

        v3 = crc64_table[7][(v3 & 0xff) ^ p3[i3]] ^
             crc64_table[6][((v3 >> 8) & 0xff) ^ p3[i3 + 1]] ^
             crc64_table[5][((v3 >> 16) & 0xff) ^ p3[i3 + 2]] ^
             crc64_table[4][((v3 >> 24) & 0xff) ^ p3[i3 + 3]] ^
             crc64_table[3][((v3 >> 32) & 0xff) ^ p3[i3 + 4]] ^
             crc64_table[2][((v3 >> 40) & 0xff) ^ p3[i3 + 5]] ^
             crc64_table[1][((v3 >> 48) & 0xff) ^ p3[i3 + 6]] ^
             crc64_table[0][((v3 >> 56) ^ p3[i3 + 7]) & 0xff];

        i0 += 8; i1 += 8; i2 += 8; i3 += 8;
    }

    /* 处理各段剩余字节 */
    r0_len -= min_chunks * 8;
    r1_len -= min_chunks * 8;
    r2_len -= min_chunks * 8;
    r3_len -= min_chunks * 8;

    crc64_process_remaining(&v0, p0, &i0, &r0_len);
    crc64_process_remaining(&v1, p1, &i1, &r1_len);
    crc64_process_remaining(&v2, p2, &i2, &r2_len);
    crc64_process_remaining(&v3, p3, &i3, &r3_len);

    /* 合并 4 个部分 CRC */
    uint64_t crc0 = ~v0;
    uint64_t crc1 = ~v1;
    uint64_t crc2 = ~v2;
    uint64_t crc3 = ~v3;

    uint64_t result = crc0;
    result = obs_crc64_internal_combine(result, crc1, quarter);
    result = obs_crc64_internal_combine(result, crc2, quarter);
    result = obs_crc64_internal_combine(result, crc3, length - 3 * quarter);

    ctx->value = result;
}

/* 更新 CRC64 计算（4-way 并行流优化） */
void obs_crc64_internal_update(obs_crc64_internal *ctx, const unsigned char *buffer, size_t length)
{
    if (ctx == NULL || buffer == NULL) {
        return;
    }

    crc64_ensure_initialized();

    /*
     * 4-way 并行流优化: 将数据分为 4 个连续段，各自独立计算 slice-by-8 CRC，
     * 最后通过 GF(2) 合并矩阵将 4 个部分 CRC 合并为最终结果。
     *
     * 原理:
     *   CRC 是 GF(2) 上的线性运算（除初始/最终取反外），因此:
     *     CRC(A || B) = combine(CRC(A), CRC(B), len(B))
     *   其中 combine 使用 GF(2) 矩阵乘法将 CRC(A) 平移 len(B) 位后与 CRC(B) 异或。
     *
     *   将数据分为 4 段后，4 个 slice-by-8 计算互不依赖，
     *   CPU 可以在同一循环体内交错执行 4 条独立的指令依赖链（ILP），
     *   充分利用现代 CPU 的超标量执行能力。
     *
     *   合并开销: 3 次 crc64_combine 调用，每次 O(log N)，
     *   对于大数据（>4KB）分摊到总处理时间中可忽略不计。
     */
    if (length < 64) {
        /* 小数据量: 直接使用 slice-by-8，避免并行分割和合并开销 */
        uint64_t value = ~ctx->value;
        value = crc64_slice_by_8(value, buffer, length);
        ctx->value = ~value;
        return;
    }

    /* 大数据量: 4-way 并行流 */
    crc64_4way_parallel(ctx, buffer, length);
}

/* 完成 CRC64 计算 */
uint64_t obs_crc64_internal_finalize(obs_crc64_internal *ctx)
{
    if (ctx == NULL) {
        return 0;
    }
    return ctx->value;
}

/* 一次性计算 CRC64 */
uint64_t obs_crc64_internal_compute(const unsigned char *buffer, size_t length)
{
    obs_crc64_internal ctx;
    obs_crc64_internal_init(&ctx);
    obs_crc64_internal_update(&ctx, buffer, length);
    return obs_crc64_internal_finalize(&ctx);
}

/* CRC64 值转字符串（unsigned long，无符号） */
void obs_crc64_to_string_internal(uint64_t crc64_value, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size < 21) { /* 最大 20 位数字 + 空终止符 */
        return;
    }

    snprintf_s(buffer, buffer_size, _TRUNCATE, "%llu", (unsigned long long)crc64_value);
}

/* 字符串转 CRC64 值 */
uint64_t obs_crc64_from_string_internal(const char *string)
{
    if (string == NULL) {
        return 0;
    }

    return strtoull(string, NULL, 10);
}

/* 合并两个 CRC64 值（用于分段上传） */
uint64_t obs_crc64_internal_combine(uint64_t crc64_first, uint64_t crc64_second, size_t len2)
{
    uint64_t even[GF2_DIM];
    uint64_t odd[GF2_DIM];

    crc64_ensure_initialized();

    /* 退化情况 */
    if (len2 == 0) {
        return crc64_first;
    }

    /* 复制矩阵 */
    (void)memcpy_s(even, sizeof(even), crc64_even, sizeof(even));
    (void)memcpy_s(odd, sizeof(odd), crc64_odd, sizeof(odd));

    /* 对 crc64_first 应用 len2 个零 */
    do {
        /* 应用当前位的零操作符 */
        gf2_matrix_square(even, GF2_DIM, odd, GF2_DIM);
        if ((len2 & 1) == 1) {
            crc64_first = gf2_matrix_times(even, GF2_DIM, crc64_first);
        }
        len2 >>= 1;

        /* 如果没有更多位，则完成 */
        if (len2 == 0) {
            break;
        }

        /* 另一次迭代，交换 odd 和 even */
        gf2_matrix_square(odd, GF2_DIM, even, GF2_DIM);
        if ((len2 & 1) == 1) {
            crc64_first = gf2_matrix_times(odd, GF2_DIM, crc64_first);
        }
        len2 >>= 1;

        /* 如果没有更多位，则完成 */
    } while (len2 != 0);

    /* 返回合并后的 CRC */
    return crc64_first ^ crc64_second;
}

/* 计算文件的 CRC64 值（公共 API） */
uint64_t obs_compute_file_crc64(const char *file_path)
{
    int fd;
    ssize_t bytes_read;
    unsigned char buffer[8192];
    obs_crc64_internal ctx;
    uint64_t crc64_result;
    char resolved_path[PATH_MAX];

    if (file_path == NULL) {
        return 0;
    }

    if (realpath(file_path, resolved_path) == NULL) {
        return 0;
    }

    fd = open(resolved_path, O_RDONLY);
    if (fd < 0) {
        return 0;
    }

    obs_crc64_internal_init(&ctx);

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        obs_crc64_internal_update(&ctx, buffer, (size_t)bytes_read);
    }

    close(fd);

    if (bytes_read < 0) {
        return 0;
    }

    crc64_result = obs_crc64_internal_finalize(&ctx);
    return crc64_result;
}

/* 计算文件指定偏移量和大小的 CRC64（用于分段上传） */
uint64_t obs_crc64_compute_file_range(const char *file_path, uint64_t offset, uint64_t size)
{
    int fd;
    ssize_t bytes_read;
    unsigned char buffer[8192];
    obs_crc64_internal ctx;
    uint64_t crc64_result;
    uint64_t bytes_processed = 0;
    char resolved_path[PATH_MAX];

    if (file_path == NULL) {
        return 0;
    }

    if (realpath(file_path, resolved_path) == NULL) {
        return 0;
    }

    fd = open(resolved_path, O_RDONLY);
    if (fd < 0) {
        return 0;
    }

    /* 跳到指定偏移量 */
    if (lseek(fd, (off_t)offset, SEEK_SET) < 0) {
        close(fd);
        return 0;
    }

    obs_crc64_internal_init(&ctx);

    /* 读取指定大小的数据 */
    while (bytes_processed < size) {
        uint64_t bytes_to_read = size - bytes_processed;
        if (bytes_to_read > sizeof(buffer)) {
            bytes_to_read = sizeof(buffer);
        }

        bytes_read = read(fd, buffer, (size_t)bytes_to_read);
        if (bytes_read <= 0) {
            break;
        }

        obs_crc64_internal_update(&ctx, buffer, (size_t)bytes_read);
        bytes_processed += (uint64_t)bytes_read;
    }

    close(fd);

    if (bytes_processed != size && size > 0) {
        /* 没有读取到预期的数据量 */
        return 0;
    }

    crc64_result = obs_crc64_internal_finalize(&ctx);
    return crc64_result;
}
// SPDX-FileCopyrightText: 2019–2025 Andy Curtis <contactandyc@gmail.com>
// SPDX-FileCopyrightText: 2024–2025 Knode.ai — technical questions: contact Andy (above)
// SPDX-License-Identifier: Apache-2.0

#include "the-lz4-library/lz4.h"
#include "a-memory-library/aml_buffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_STRING "This is a test string to verify LZ4 compression and decompression functionality."

void test_lz4_compression_and_decompression(void) {
    printf("Running LZ4 compression and decompression test...\n");

    // Original data
    const char *original_data = TEST_STRING;
    size_t original_size = strlen(original_data) + 1;

    // Buffer for compression
    aml_buffer_t *compressed_buffer = aml_buffer_init(1024);
    if (!compressed_buffer) {
        fprintf(stderr, "Failed to initialize compressed buffer.\n");
        return;
    }

    // Compress data
    size_t compressed_size = lz4_compress_appending_to_buffer(compressed_buffer, (void *)original_data, (int)original_size, 1);
    if (compressed_size == 0) {
        fprintf(stderr, "Compression failed.\n");
        aml_buffer_destroy(compressed_buffer);
        return;
    }

    printf("Original size: %zu bytes, Compressed size: %zu bytes\n", original_size, compressed_size);

    // Buffer for decompression
    void *decompressed_data = malloc(original_size);
    if (!decompressed_data) {
        fprintf(stderr, "Failed to allocate decompression buffer.\n");
        aml_buffer_destroy(compressed_buffer);
        return;
    }

    // Decompress data
    bool decompression_success = lz4_decompress_into_fixed_buffer(decompressed_data, (int)original_size,
                                                                  aml_buffer_data(compressed_buffer), (int)compressed_size);
    if (!decompression_success) {
        fprintf(stderr, "Decompression failed.\n");
        free(decompressed_data);
        aml_buffer_destroy(compressed_buffer);
        return;
    }

    printf("Decompressed data: %s\n", (char *)decompressed_data);

    // Validate decompressed data
    if (strcmp(original_data, (char *)decompressed_data) == 0) {
        printf("Test passed: Decompressed data matches original data.\n");
    } else {
        printf("Test failed: Decompressed data does not match original data.\n");
    }

    // Cleanup
    free(decompressed_data);
    aml_buffer_destroy(compressed_buffer);
}

void test_lz4_block_compression(void) {
    printf("\nRunning LZ4 block compression test...\n");

    // Original data
    const char *original_data = TEST_STRING;
    size_t original_size = strlen(original_data) + 1;

    // Initialize LZ4 context
    lz4_t *lz4_ctx = lz4_init(1, s64kb, true, true);
    if (!lz4_ctx) {
        fprintf(stderr, "Failed to initialize LZ4 context.\n");
        return;
    }

    // Buffers for compression
    size_t max_compressed_size = lz4_compress_bound((int)original_size);
    void *compressed_data = malloc(max_compressed_size);
    if (!compressed_data) {
        fprintf(stderr, "Failed to allocate compression buffer.\n");
        lz4_destroy(lz4_ctx);
        return;
    }

    // Compress the block
    uint32_t compressed_size = lz4_compress_block(lz4_ctx, original_data, (uint32_t)original_size,
                                                  compressed_data, (uint32_t)max_compressed_size);
    if (compressed_size == 0) {
        fprintf(stderr, "Block compression failed.\n");
        free(compressed_data);
        lz4_destroy(lz4_ctx);
        return;
    }

    printf("Block compressed size: %u bytes\n", compressed_size);

    // Buffer for decompression
    void *decompressed_data = malloc(original_size);
    if (!decompressed_data) {
        fprintf(stderr, "Failed to allocate decompression buffer.\n");
        free(compressed_data);
        lz4_destroy(lz4_ctx);
        return;
    }

    // Decompress the block
    int result = lz4_decompress(lz4_ctx, compressed_data, compressed_size, decompressed_data, (uint32_t)original_size, true);
    if (result < 0) {
        fprintf(stderr, "Expected block decompression failed: %d\n", result);
        free(decompressed_data);
        free(compressed_data);
        lz4_destroy(lz4_ctx);
        return;
    }

    printf("Decompressed block data: %s\n", (char *)decompressed_data);

    // Validate decompressed data
    if (strcmp(original_data, (char *)decompressed_data) == 0) {
        printf("Block compression test passed: Decompressed data matches original data.\n");
    } else {
        printf("Block compression test failed: Decompressed data does not match original data.\n");
    }

    // Cleanup
    free(decompressed_data);
    free(compressed_data);
    lz4_destroy(lz4_ctx);
}

int main(void) {
    test_lz4_compression_and_decompression();
    test_lz4_block_compression();
    return 0;
}

# LZ4 Compression Library README

## Overview
The LZ4 Compression Library provides functions for compressing and decompressing data using the LZ4 algorithm. It integrates with the A Memory Library for memory management and buffer handling, offering efficient and fast compression methods suitable for various applications.

## Functions

### Hashing
- `lz4_hash64`: Computes a 64-bit hash of the given data.

### Compression Utility
- `lz4_compress_bound`: Calculates the maximum compressed size given the input size.

### Compression and Decompression
- `lz4_compress_appending_to_buffer`: Compresses data and appends it to an `aml_buffer_t` buffer.
- `lz4_decompress_into_fixed_buffer`: Decompresses data into a fixed-size buffer.

### Configuration Types
- `lz4_block_size_t`: Enum type representing different block sizes for compression (64KB, 256KB, 1MB, 4MB).

### Structure and Initialization
- `lz4_t`: Structure representing an LZ4 compression context.
- `lz4_init`: Initializes an LZ4 compression context with various options.
- `lz4_init_decompress`: Initializes an LZ4 decompression context.

### Header Management
- `lz4_get_header`, `lz4_block_size`, `lz4_block_header_size`, `lz4_compressed_size`: Functions for managing and retrieving information from LZ4 headers.

### Header Validation
- `lz4_check_header`: Validates an LZ4 header.

### Decompression
- `lz4_decompress`: Decompresses a block of data.

### Compression
- `lz4_compress`, `lz4_compress_block`: Functions for compressing blocks of data.

### Finalization
- `lz4_finish`: Finalizes the compression or decompression process, verifying the integrity of the data.

### Cleanup
- `lz4_destroy`: Destroys an LZ4 context, releasing any associated resources.

## Usage
The library is designed to be integrated into C or C++ projects. It provides both compression and decompression functionalities along with additional utilities for handling LZ4 headers and checking data integrity. The library is especially useful in scenarios where high-speed compression is required.

### Debugging
The library includes macros for debugging, which can be enabled by defining `_AML_DEBUG_`.

## Dependencies
- A Memory Library (`a-memory-library/aml_alloc.h` and `a-memory-library/aml_buffer.h`): Required for memory management and buffer operations.

## Integration
To use this library, include the relevant headers in your C or C++ project and link against the compiled library. Ensure that the A Memory Library is also included and linked as required.

## License
Please refer to the specific license terms for usage and redistribution.

(Note: This README provides a high-level overview of the functions and usage of the LZ4 Compression Library. For detailed usage and examples, refer to the individual function documentation and provided example code.)
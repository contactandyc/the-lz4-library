// SPDX-FileCopyrightText: 2019–2025 Andy Curtis <contactandyc@gmail.com>
// SPDX-FileCopyrightText: 2024–2025 Knode.ai — technical questions: contact Andy (above)
// SPDX-License-Identifier: Apache-2.0

#ifndef _lz4_H
#define _lz4_H

#include "a-memory-library/aml_alloc.h"
#include "a-memory-library/aml_buffer.h"

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t lz4_hash64(const void *s, size_t len);

int lz4_compress_bound(int inputSize);

size_t lz4_compress_appending_to_buffer(aml_buffer_t *dest, void *src, int src_size, int level);
bool lz4_decompress_into_fixed_buffer(void *dest, int dest_size, void *src, int src_size);


enum lz4_block_size_s { s64kb = 0, s256kb = 1, s1mb = 2, s4mb = 3 };
typedef enum lz4_block_size_s lz4_block_size_t;

struct lz4_s;
typedef struct lz4_s lz4_t;

const char *lz4_get_header(lz4_t *r, uint32_t *length);
uint32_t lz4_block_size(lz4_t *r);
uint32_t lz4_block_header_size(lz4_t *r);
uint32_t lz4_compressed_size(lz4_t *r);

typedef struct {
  uint32_t block_size;
  uint32_t compressed_size;
  lz4_block_size_t size;
  bool block_checksum;
  bool content_checksum;
  char *header;
} lz4_header_t;

bool lz4_check_header(lz4_header_t *r, void *header,
                         uint32_t header_size);

#ifdef _AML_DEBUG_
#define lz4_init(level, size, block_checksum, content_checksum)             \
  _lz4_init(level, size, block_checksum, content_checksum,                  \
               aml_file_line_func("lz4"))
lz4_t *_lz4_init(int level, lz4_block_size_t size, bool block_checksum,
                       bool content_checksum, const char *caller);
#else
#define lz4_init(level, size, block_checksum, content_checksum)             \
  _lz4_init(level, size, block_checksum, content_checksum)
lz4_t *_lz4_init(int level, lz4_block_size_t size, bool block_checksum,
                       bool content_checksum);
#endif

#ifdef _AML_DEBUG_
#define lz4_init_decompress(header, header_size)                            \
  _lz4_init_decompress(header, header_size,                                 \
                       aml_file_line_func("lz4_decompress"))
lz4_t *_lz4_init_decompress(void *header, uint32_t header_size,
                            const char *caller);
#else
#define lz4_init_decompress(header, header_size)                            \
  _lz4_init_decompress(header, header_size)
lz4_t *_lz4_init_decompress(void *header, uint32_t header_size);
#endif

/* caller is expected to read block size in advance and compute src_len.  If
   block_size has high bit, block is uncompressed and high bit should be unset.
   src should not point to the byte just after block size in input stream.
*/
int lz4_decompress(lz4_t *l, const void *src, uint32_t src_len,
                      void *dest, uint32_t dest_len, bool compressed);

uint32_t lz4_compress(lz4_t *l, const void *src, uint32_t src_len,
                         void *dest, uint32_t dest_len);

uint32_t lz4_compress_block(lz4_t *l, const void *src, uint32_t src_len,
                               void *dest, uint32_t dest_len);

/* this will return a negative number if crc doesn't match.  dest should point
   to location for size if compressing and just after block_size if
   decompressing.  If result is non-negative, then it succeeded and read or
   wrote result byte(s).
*/
int lz4_finish(lz4_t *l, void *dest);

void lz4_destroy(lz4_t *r);

#ifdef __cplusplus
}
#endif


#endif

// SPDX-FileCopyrightText: 2019–2025 Andy Curtis <contactandyc@gmail.com>
// SPDX-FileCopyrightText: 2024–2025 Knode.ai — technical questions: contact Andy (above)
// SPDX-License-Identifier: Apache-2.0

#include "the-lz4-library/lz4.h"

#include "impl/lz4.c"
#include "impl/lz4hc.c"
#include "impl/xxhash.c"

#include "a-memory-library/aml_alloc.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t lz4_hash64(const void *s, size_t len) {
  return (uint64_t)XXH64(s, len, 0);
}

int lz4_compress_bound(int inputSize) {
    return LZ4_compressBound(inputSize);
}


static uint8_t _64kb[7] = {0x04, 0x22, 0x4d, 0x18, 0x60, 0x40, 0x82};
static uint8_t c_64kb[7] = {0x04, 0x22, 0x4d, 0x18, 0x64, 0x40, 0xa7};
static uint8_t b_64kb[7] = {0x04, 0x22, 0x4d, 0x18, 0x70, 0x40, 0xad};
static uint8_t cb_64kb[7] = {0x04, 0x22, 0x4d, 0x18, 0x74, 0x40, 0xbd};

static uint8_t _256kb[7] = {0x04, 0x22, 0x4d, 0x18, 0x60, 0x50, 0xfb};
static uint8_t c_256kb[7] = {0x04, 0x22, 0x4d, 0x18, 0x64, 0x50, 0x08};
static uint8_t b_256kb[7] = {0x04, 0x22, 0x4d, 0x18, 0x70, 0x50, 0x84};
static uint8_t cb_256kb[7] = {0x04, 0x22, 0x4d, 0x18, 0x74, 0x50, 0xff};

static uint8_t _1mb[7] = {0x04, 0x22, 0x4d, 0x18, 0x60, 0x60, 0x51};
static uint8_t c_1mb[7] = {0x04, 0x22, 0x4d, 0x18, 0x64, 0x60, 0x85};
static uint8_t b_1mb[7] = {0x04, 0x22, 0x4d, 0x18, 0x70, 0x60, 0x33};
static uint8_t cb_1mb[7] = {0x04, 0x22, 0x4d, 0x18, 0x74, 0x60, 0xd9};

static uint8_t _4mb[7] = {0x04, 0x22, 0x4d, 0x18, 0x60, 0x70, 0x73};
static uint8_t c_4mb[7] = {0x04, 0x22, 0x4d, 0x18, 0x64, 0x70, 0xb9};
static uint8_t b_4mb[7] = {0x04, 0x22, 0x4d, 0x18, 0x70, 0x70, 0x72};
static uint8_t cb_4mb[7] = {0x04, 0x22, 0x4d, 0x18, 0x74, 0x70, 0x8e};

struct lz4_s {
  lz4_block_size_t size;
  bool content_checksum;
  bool block_checksum;
  int level;

  uint8_t *header;
  uint32_t header_size;

  uint32_t block_size;
  uint32_t compressed_size;
  uint32_t block_header_size;
  XXH32_state_t xxh;
  void *ctx;
};

const char *lz4_get_header(lz4_t *r, uint32_t *length) {
  *length = r->header_size;
  return (const char *)r->header;
}

uint32_t lz4_block_size(lz4_t *r) { return r->block_size; }

uint32_t lz4_block_header_size(lz4_t *r) { return r->block_header_size; }

uint32_t lz4_compressed_size(lz4_t *r) {
  return r->compressed_size + r->block_header_size;
}

static void write_little_endian_32(char *dest, uint32_t v) {
  (*(uint32_t *)(dest)) = v;
}

static uint32_t read_little_endian_32(char *dest) {
  return (*(uint32_t *)(dest));
}

uint32_t lz4_compress(lz4_t *l, const void *src, uint32_t src_len,
                         void *dest, uint32_t dest_len) {
  int level = l->level;
  void *ctx = l->ctx;
  if (level < LZ4HC_CLEVEL_MIN) {
    /* this does a bit more than just attaching dictionary (needed?) */
    LZ4_attach_dictionary((LZ4_stream_t *)ctx, NULL);
    int const acceleration = (level < 0) ? -level + 1 : 1;

    return LZ4_compress_fast_extState_fastReset(
        (LZ4_stream_t *)ctx, (const char *)src, (char *)dest, src_len, dest_len,
        acceleration);
  } else {
    LZ4_resetStreamHC_fast((LZ4_streamHC_t *)ctx, level);
    /* this does a bit more than just attaching dictionary (needed?) */
    LZ4_attach_HC_dictionary((LZ4_streamHC_t *)ctx, NULL);
    return LZ4_compress_HC_extStateHC_fastReset(
        ctx, (const char *)src, (char *)dest, src_len, dest_len, level);
  }
}

int lz4_finish(lz4_t *l, void *dest) {
  char *destp = (char *)dest;
  if (l->ctx) {
    write_little_endian_32(destp, 0);
    destp += sizeof(uint32_t);
    if (l->content_checksum) {
      uint32_t crc = XXH32_digest(&(l->xxh));
      write_little_endian_32(destp, crc);
      return 8;
    }
    return 4;
  } else {
    if (l->content_checksum) {
      uint32_t crc = XXH32_digest(&(l->xxh));
      uint32_t content_crc = read_little_endian_32(destp);
      if (crc != content_crc)
        return -500;
    }
    return 0;
  }
}

/*
   make sure that the block checksum matches if desired.
   If this is being used for seeking, you should assume that
   the last block can be less than dest_len decompressed.
*/
bool lz4_skip(lz4_t *l, const void *src, uint32_t src_len, void *dest,
                 uint32_t dest_len, bool compressed) {
  if (l->block_checksum) {
    char *srcp = (char *)src;
    uint32_t checksum = read_little_endian_32(srcp + src_len - 4);
    uint32_t crc32 = XXH32(src, src_len - 4, 0);
    if (crc32 != checksum)
      return false;
    src_len -= 4;
  }
  if (l->content_checksum) {
    int r =
        LZ4_decompress_safe((const char *)src, (char *)dest, src_len, dest_len);
    if (r < 0)
      return false;
    if (l->content_checksum)
      (void)XXH32_update(&l->xxh, dest, r);
  }
  return true;
}

int lz4_decompress(lz4_t *l, const void *src, uint32_t src_len,
                      void *dest, uint32_t dest_len, bool compressed) {
  if (l->block_checksum) {
    char *srcp = (char *)src;
    uint32_t checksum = read_little_endian_32(srcp + src_len - 4);
    uint32_t crc32 = XXH32(src, src_len - 4, 0);
    if (crc32 != checksum)
      return -500;
    src_len -= 4;
  }

  int r = src_len;
  if (compressed)
    r = LZ4_decompress_safe((const char *)src, (char *)dest, src_len, dest_len);
  else
    memcpy(dest, src, src_len);
  if (r < 0)
    return r;
  if (l->content_checksum)
    (void)XXH32_update(&l->xxh, dest, r);
  return r;
}

uint32_t lz4_compress_block(lz4_t *l, const void *src, uint32_t src_len,
                               void *dest, uint32_t dest_len) {
  if (l->content_checksum)
    (void)XXH32_update(&l->xxh, src, src_len);

  char *destp = (char *)dest;
  uint32_t compressed_size =
      lz4_compress(l, src, src_len, destp + sizeof(uint32_t),
                      dest_len - l->block_header_size);
  if (compressed_size >= src_len) {
    compressed_size = src_len;
    write_little_endian_32(destp, src_len | 0x80000000U);
    memcpy(destp + sizeof(uint32_t), src, src_len);
  } else
    write_little_endian_32(destp, compressed_size);
  destp += sizeof(uint32_t);

  if (l->block_checksum) {
    uint32_t crc32 = XXH32(destp, compressed_size, 0);
    write_little_endian_32(destp + compressed_size, crc32);
  }
  return compressed_size + l->block_header_size;
}

bool lz4_check_header(lz4_header_t *r, void *header,
                         uint32_t header_size) {
  if (header_size != 7 || !r)
    return false;
  uint8_t *h = (uint8_t *)header;
  if (h[0] != _64kb[0] || h[1] != _64kb[1] || h[2] != _64kb[2] ||
      h[3] != _64kb[3])
    return false;
  lz4_block_size_t size;
  bool block_checksum;
  bool content_checksum;
  uint32_t block_size;
  uint8_t *headerp;

  if (h[5] == 0x40) {
    size = s64kb;
    block_size = 64 * 1024;
    if (h[4] == _64kb[4] && h[6] == _64kb[6]) {
      headerp = &_64kb[0];
      block_checksum = false;
      content_checksum = false;
    } else if (h[4] == b_64kb[4] && h[6] == b_64kb[6]) {
      headerp = &b_64kb[0];
      block_checksum = true;
      content_checksum = false;
    } else if (h[4] == c_64kb[4] && h[6] == c_64kb[6]) {
      headerp = &c_64kb[0];
      block_checksum = false;
      content_checksum = true;
    } else if (h[4] == cb_64kb[4] && h[6] == cb_64kb[6]) {
      headerp = &cb_64kb[0];
      block_checksum = true;
      content_checksum = true;
    } else
      return false;
  } else if (h[5] == 0x50) {
    size = s256kb;
    block_size = 256 * 1024;
    if (h[4] == _256kb[4] && h[6] == _256kb[6]) {
      headerp = &_256kb[0];
      block_checksum = false;
      content_checksum = false;
    } else if (h[4] == b_256kb[4] && h[6] == b_256kb[6]) {
      headerp = &b_256kb[0];
      block_checksum = true;
      content_checksum = false;
    } else if (h[4] == c_256kb[4] && h[6] == c_256kb[6]) {
      headerp = &c_256kb[0];
      block_checksum = false;
      content_checksum = true;
    } else if (h[4] == cb_256kb[4] && h[6] == cb_256kb[6]) {
      headerp = &cb_256kb[0];
      block_checksum = true;
      content_checksum = true;
    } else
      return false;
  } else if (h[5] == 0x60) {
    size = s1mb;
    block_size = 1024 * 1024;
    if (h[4] == _1mb[4] && h[6] == _1mb[6]) {
      headerp = &_1mb[0];
      block_checksum = false;
      content_checksum = false;
    } else if (h[4] == b_1mb[4] && h[6] == b_1mb[6]) {
      headerp = &b_1mb[0];
      block_checksum = true;
      content_checksum = false;
    } else if (h[4] == c_1mb[4] && h[6] == c_1mb[6]) {
      headerp = &c_1mb[0];
      block_checksum = false;
      content_checksum = true;
    } else if (h[4] == cb_1mb[4] && h[6] == cb_1mb[6]) {
      headerp = &cb_1mb[0];
      block_checksum = true;
      content_checksum = true;
    } else
      return false;
  } else if (h[5] == 0x70) {
    size = s4mb;
    block_size = 4 * 1024 * 1024;
    if (h[4] == _4mb[4] && h[6] == _4mb[6]) {
      headerp = &_4mb[0];
      block_checksum = false;
      content_checksum = false;
    } else if (h[4] == b_4mb[4] && h[6] == b_4mb[6]) {
      headerp = &b_4mb[0];
      block_checksum = true;
      content_checksum = false;
    } else if (h[4] == c_4mb[4] && h[6] == c_4mb[6]) {
      headerp = &c_4mb[0];
      block_checksum = false;
      content_checksum = true;
    } else if (h[4] == cb_4mb[4] && h[6] == cb_4mb[6]) {
      headerp = &cb_4mb[0];
      block_checksum = true;
      content_checksum = true;
    } else
      return false;
  } else
    return false;
  uint32_t compressed_size = LZ4_compressBound(block_size);
  r->compressed_size = compressed_size;
  r->block_size = block_size;
  r->block_checksum = block_checksum;
  r->content_checksum = content_checksum;
  r->header = (char *)headerp;
  return true;
}

#ifdef _AML_DEBUG_
lz4_t *_lz4_init_decompress(void *header, uint32_t header_size,
                                  const char *caller) {
#else
lz4_t *_lz4_init_decompress(void *header, uint32_t header_size) {
#endif
  lz4_header_t h;
  if (!lz4_check_header(&h, header, header_size))
    return NULL;

#ifdef _AML_DEBUG_
  lz4_t *r = (lz4_t *)_aml_malloc_d(caller, sizeof(lz4_t), false);
#else
  lz4_t *r = (lz4_t *)aml_malloc(sizeof(lz4_t));
#endif
  r->ctx = NULL;
  r->level = 1;
  r->block_size = h.block_size;
  r->compressed_size = h.compressed_size;
  r->size = h.size;
  r->content_checksum = h.content_checksum;
  r->block_checksum = h.block_checksum;
  r->block_header_size = h.block_checksum ? 4 : 0;
  r->header = (uint8_t *)h.header;
  r->header_size = 7;
  if (h.content_checksum)
    XXH32_reset(&(r->xxh), 0);
  return r;
}

#ifdef _AML_DEBUG_
lz4_t *_lz4_init(int level, lz4_block_size_t size, bool block_checksum,
                       bool content_checksum, const char *caller) {
#else
lz4_t *_lz4_init(int level, lz4_block_size_t size, bool block_checksum,
                       bool content_checksum) {
#endif
  uint32_t ctx_size =
      level < LZ4HC_CLEVEL_MIN ? sizeof(LZ4_stream_t) : sizeof(LZ4_streamHC_t);
  uint8_t *header;
  uint32_t block_size;
  if (size == s64kb) {
    if (!content_checksum)
      header = block_checksum ? &b_64kb[0] : &_64kb[0];
    else
      header = block_checksum ? &cb_64kb[0] : &c_64kb[0];
    block_size = 64 * 1024;
  } else if (size == s256kb) {
    if (!content_checksum)
      header = block_checksum ? &b_256kb[0] : &_256kb[0];
    else
      header = block_checksum ? &cb_256kb[0] : &c_256kb[0];
    block_size = 256 * 1024;
  } else if (size == s1mb) {
    if (!content_checksum)
      header = block_checksum ? &b_1mb[0] : &_1mb[0];
    else
      header = block_checksum ? &cb_1mb[0] : &c_1mb[0];
    block_size = 1024 * 1024;
  } else if (size == s4mb) {
    if (!content_checksum)
      header = block_checksum ? &b_4mb[0] : &_4mb[0];
    else
      header = block_checksum ? &cb_4mb[0] : &c_4mb[0];
    block_size = 4 * 1024 * 1024;
  } else
    return NULL;

  uint32_t compressed_size = LZ4_compressBound(block_size);

#ifdef _AML_DEBUG_
  lz4_t *r = (lz4_t *)_aml_malloc_d(caller,
                                         sizeof(lz4_t) + ctx_size, false);
#else
  lz4_t *r = (lz4_t *)aml_malloc(sizeof(lz4_t) + ctx_size);
#endif
  r->ctx = (void *)(r + 1);
  r->level = level;
  r->block_size = block_size;
  r->compressed_size = compressed_size;
  r->size = size;
  r->content_checksum = content_checksum;
  r->block_checksum = block_checksum;
  r->block_header_size = 4 + (block_checksum ? 4 : 0);
  r->header = header;
  r->header_size = 7;
  if (level < LZ4HC_CLEVEL_MIN) {
    LZ4_initStream((LZ4_stream_t *)r->ctx, sizeof(LZ4_stream_t));
  } else {
    LZ4_initStreamHC((LZ4_streamHC_t *)r->ctx, sizeof(LZ4_streamHC_t));
    LZ4_setCompressionLevel((LZ4_streamHC_t *)r->ctx, level);
  }
  return r;
}

void lz4_destroy(lz4_t *r) { aml_free(r); }

size_t lz4_compress_appending_to_buffer(aml_buffer_t *dest, void *src, int src_size, int level) {
    int max_dst_size = LZ4_compressBound(src_size);
    size_t olen = aml_buffer_length(dest);

    // Allocate space for both the compressed data and the HC stream
    size_t hc_stream_size = (level > 0) ? (sizeof(LZ4_streamHC_t)+8) : 0;
    void *dst = (void *)aml_buffer_append_ualloc(dest, max_dst_size + hc_stream_size);

    if (!dst) {
        return 0; // Memory allocation failed
    }

    int compressed_data_size = 0;

    if (level <= 0) {
        // Use default compression (fast mode)
        compressed_data_size = LZ4_compress_default((const char *)src, (char *)dst, src_size, max_dst_size);
    } else {
        // Use high-compression mode
        char *ep = dst+max_dst_size;
        ep += 8 - ((size_t)ep & 7); // Ensure 8-byte alignment
        LZ4_streamHC_t *hc_stream = (LZ4_streamHC_t *)ep; // Place HC stream after the compressed data space

        // Initialize the stream
        LZ4_initStreamHC(hc_stream, sizeof(LZ4_streamHC_t));
        LZ4_setCompressionLevel(hc_stream, level);

        compressed_data_size = LZ4_compress_HC_extStateHC(hc_stream, (const char *)src, (char *)dst, src_size, max_dst_size, level);
    }

    if (compressed_data_size <= 0) {
        // Compression failed, revert buffer size
        aml_buffer_resize(dest, olen);
        return 0;
    }

    // Resize the buffer to match the compressed data size (ignore HC stream memory)
    aml_buffer_resize(dest, olen + compressed_data_size);
    return compressed_data_size;
}

bool lz4_decompress_into_fixed_buffer(void *dest, int dest_size, void *src, int src_size) {
  int decompressed_size = LZ4_decompress_safe((const char *)src, (char *)dest, src_size, dest_size);
  if (decompressed_size != dest_size)
    return false;
  return true;
}

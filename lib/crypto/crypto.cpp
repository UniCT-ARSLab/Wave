#include <crypto.h>

#include "mbedtls/gcm.h"

#include <string.h>

extern const uint8_t test_key[AES_KEY_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

// Helper per scrivere uint32_t in formato Big-Endian (Network Byte Order)
static void write_uint32_be(uint8_t *buffer, uint32_t value) {
  buffer[0] = (value >> 24) & 0xFF;
  buffer[1] = (value >> 16) & 0xFF;
  buffer[2] = (value >> 8) & 0xFF;
  buffer[3] = value & 0xFF;
}

void buildGcmIv(uint32_t originId, uint32_t seq, uint8_t iv[GCM_IV_SIZE]) {
  memcpy(iv, &originId, 4);
  memcpy(iv + 4, &seq, 4);

  memset(iv + 8, 0, 4);
}

static void buildAad(uint32_t originId, uint32_t seq, uint8_t ttl,
                     uint8_t aad[9]) {
  memcpy(aad, &originId, 4);
  memcpy(aad + 4, &seq, 4);
  aad[8] = ttl;
}

bool aesGcmEncrypt(const uint8_t key[AES_KEY_SIZE],

                   uint32_t originId, uint32_t seq, uint8_t ttl,

                   const uint8_t *plaintext, size_t plaintextLen,

                   uint8_t *ciphertext,

                   uint8_t tag[GCM_TAG_SIZE]) {
  uint8_t iv[GCM_IV_SIZE];
  uint8_t aad[9];

  buildGcmIv(originId, seq, iv);
  buildAad(originId, seq, ttl, aad);

  mbedtls_gcm_context ctx;

  mbedtls_gcm_init(&ctx);

  if (mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, AES_KEY_SIZE * 8) !=
      0) {
    mbedtls_gcm_free(&ctx);
    return false;
  }

  int ret = mbedtls_gcm_crypt_and_tag(&ctx,

                                      MBEDTLS_GCM_ENCRYPT,

                                      plaintextLen,

                                      iv, sizeof(iv),

                                      aad, sizeof(aad),

                                      plaintext, ciphertext,

                                      GCM_TAG_SIZE, tag);

  mbedtls_gcm_free(&ctx);

  return ret == 0;
}

bool aesGcmDecrypt(const uint8_t key[AES_KEY_SIZE],

                   uint32_t originId, uint32_t seq, uint8_t ttl,

                   const uint8_t *ciphertext, size_t ciphertextLen,

                   const uint8_t tag[GCM_TAG_SIZE],

                   uint8_t *plaintext) {
  uint8_t iv[GCM_IV_SIZE];
  uint8_t aad[9];

  buildGcmIv(originId, seq, iv);
  buildAad(originId, seq, ttl, aad);

  mbedtls_gcm_context ctx;

  mbedtls_gcm_init(&ctx);

  if (mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
    mbedtls_gcm_free(&ctx);
    return false;
  }

  int ret = mbedtls_gcm_auth_decrypt(&ctx,

                                     ciphertextLen,

                                     iv, sizeof(iv),

                                     aad, sizeof(aad),

                                     tag, GCM_TAG_SIZE,

                                     ciphertext,

                                     plaintext);

  mbedtls_gcm_free(&ctx);

  return ret == 0;
}

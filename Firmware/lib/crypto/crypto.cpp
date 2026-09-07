#include <crypto.h>

#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include <mesh_packet.h>

#include <string.h>

extern const uint8_t test_key[AES_KEY_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

extern const uint8_t NETWORK_KEY[AES_KEY_SIZE] = {
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
    0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40};

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

static void buildAad(uint32_t originId, uint32_t seq, uint8_t aad[9]) {
  memcpy(aad, &originId, 4);
  memcpy(aad + 4, &seq, 4);
  aad[8] = 1;
}

bool aesGcmEncrypt(const uint8_t key[AES_KEY_SIZE],

                   uint32_t originId, uint32_t seq, uint8_t ttl,

                   const uint8_t *plaintext, size_t plaintextLen,

                   uint8_t *ciphertext,

                   uint8_t tag[GCM_TAG_SIZE]) {
  uint8_t iv[GCM_IV_SIZE];
  uint8_t aad[9];

  buildGcmIv(originId, seq, iv);
  buildAad(originId, seq, aad);

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
  buildAad(originId, seq, aad);

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

static void buildCtrNonce(uint32_t originId, uint32_t seq, uint8_t nonce[16]) {
  memset(nonce, 0, 16);

  memcpy(&nonce[0], &originId, sizeof(originId));
  memcpy(&nonce[4], &seq, sizeof(seq));

  /*ultimi 8 byte lasciati a zero*/
}

bool aesCtrEncrypt(uint8_t *data, size_t len, uint32_t originId, uint32_t seq) {
  mbedtls_aes_context ctx;

  uint8_t nonce_counter[16];
  uint8_t stream_block[16];

  size_t nc_off = 0;

  buildCtrNonce(originId, seq, nonce_counter);

  memset(stream_block, 0, sizeof(stream_block));

  mbedtls_aes_init(&ctx);

  if (mbedtls_aes_setkey_enc(&ctx, NETWORK_KEY, AES_KEY_SIZE * 8) != 0) {
    mbedtls_aes_free(&ctx);
    return false;
  }

  int ret = mbedtls_aes_crypt_ctr(&ctx, len, &nc_off, nonce_counter,
                                  stream_block, data, data);

  mbedtls_aes_free(&ctx);

  return ret == 0;
}

bool aesCtrDecrypt(uint8_t *data, size_t len, uint32_t originId, uint32_t seq) {
  return aesCtrEncrypt(data, len, originId, seq);
}

bool aesCtrEncryptPacket(MeshPacket &p) {
  uint8_t buffer[25];

  size_t index = 0;

  // deviceId
  memcpy(buffer + index, &p.deviceId, sizeof(p.deviceId));

  index += sizeof(p.deviceId);

  // ttl
  memcpy(buffer + index, &p.ttl, sizeof(p.ttl));

  index += sizeof(p.ttl);

  // payload AES-GCM
  memcpy(buffer + index, p.payload, sizeof(p.payload));

  index += sizeof(p.payload);

  // tag AES-GCM
  memcpy(buffer + index, p.tag, sizeof(p.tag));

  bool ret = aesCtrEncrypt(buffer, sizeof(buffer), p.originId, p.seq);

  if (!ret)
    return false;

  index = 0;

  memcpy(&p.deviceId, buffer + index, sizeof(p.deviceId));

  index += sizeof(p.deviceId);

  memcpy(&p.ttl, buffer + index, sizeof(p.ttl));

  index += sizeof(p.ttl);

  memcpy(p.payload, buffer + index, sizeof(p.payload));

  index += sizeof(p.payload);

  memcpy(p.tag, buffer + index, sizeof(p.tag));

  return true;
}

bool aesCtrDecryptPacket(MeshPacket &p) {
  uint8_t buffer[25];

  size_t index = 0;

  memcpy(buffer + index, &p.deviceId, sizeof(p.deviceId));

  index += sizeof(p.deviceId);

  memcpy(buffer + index, &p.ttl, sizeof(p.ttl));

  index += sizeof(p.ttl);

  memcpy(buffer + index, p.payload, sizeof(p.payload));

  index += sizeof(p.payload);

  memcpy(buffer + index, p.tag, sizeof(p.tag));

  bool ret = aesCtrDecrypt(buffer, sizeof(buffer), p.originId, p.seq);

  if (!ret)
    return false;

  index = 0;

  memcpy(&p.deviceId, buffer + index, sizeof(p.deviceId));

  index += sizeof(p.deviceId);

  memcpy(&p.ttl, buffer + index, sizeof(p.ttl));

  index += sizeof(p.ttl);

  memcpy(p.payload, buffer + index, sizeof(p.payload));

  index += sizeof(p.payload);

  memcpy(p.tag, buffer + index, sizeof(p.tag));

  return true;
}

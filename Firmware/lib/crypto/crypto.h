#include <stddef.h>
#include <stdint.h>

#include <mesh_packet.h>

constexpr size_t AES_KEY_SIZE = 32;
constexpr size_t GCM_TAG_SIZE = 12;
constexpr size_t GCM_IV_SIZE = 12;
extern const uint8_t test_key[AES_KEY_SIZE];
extern const uint8_t NETWORK_KEY[AES_KEY_SIZE];

/*
Questa funzione calcola IV per AES-GCM basandosi solamente su originId e Seq
Cosi il proprietario può calcolare IV una volta ricevuto il pacchetto.
 */

void buildGcmIv(uint32_t originId, uint32_t seq, uint8_t iv[GCM_IV_SIZE]);

bool aesGcmEncrypt(const uint8_t key[AES_KEY_SIZE],

                   uint32_t originId, uint32_t seq, uint8_t ttl,

                   const uint8_t *plaintext, size_t plaintextLen,

                   uint8_t *ciphertext,

                   uint8_t tag[GCM_TAG_SIZE]);

/*La decrypt non sarà necessarie nel prototipo finale*/
bool aesGcmDecrypt(const uint8_t key[AES_KEY_SIZE],

                   uint32_t originId, uint32_t seq, uint8_t ttl,

                   const uint8_t *ciphertext, size_t ciphertextLen,

                   const uint8_t tag[GCM_TAG_SIZE],

                   uint8_t *plaintext);

bool aesCtrEncrypt(uint8_t *data, size_t len, uint32_t originId, uint32_t seq);
bool aesCtrEncryptPacket(MeshPacket &p);

bool aesCtrDecrypt(uint8_t *data, size_t len, uint32_t originId, uint32_t seq);

bool aesCtrDecryptPacket(MeshPacket &p);

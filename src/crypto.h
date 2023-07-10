
#pragma once

#include <stdbool.h>
#include "defs.h"

extern const unsigned CRYPTO_KEY_SIZE;
extern const unsigned CRYPTO_HEADER_SIZE;
extern const unsigned CRYPTO_SIGNATURE_SIZE;

struct Crypto_t;
typedef struct Crypto_t Crypto;

// shared:
Crypto* nullable cryptoInit(void);

// as client:
bool cryptoExchangeKeys(Crypto* crypto, const byte* serverPublicKey); // returns true on success
byte* nullable cryptoInitializeCoderStreams(Crypto* crypto, const byte* serverStreamHeader); // expects a HEADER_SIZE-sized server header's bytes, returns a deallocation-required HEADER-SIZE-sized client header's bytes on success and null otherwise
bool cryptoCheckServerSignedBytes(const byte* signature, const byte* unsignedBytes, unsigned unsignedSize);

// as 'server', kinda ('cause sodium has ..._client_... & ..._server_..., using only client api won't work - tested):
const byte* cryptoGenerateKeyPairAsServer(Crypto* crypto); // returns public key that must be deallocated, acts as a server
bool cryptoExchangeKeysAsServer(Crypto* crypto, const byte* clientPublicKey); // returns true on success
byte* nullable cryptoCreateEncoderAsServer(Crypto* crypto); // returns encoder header on success, deallocation's needed
bool cryptoCreateDecoderStreamAsServer(Crypto* crypto, const byte* clientStreamHeader); // returns true on success, expects client's encoderr stream header with size of HEADER_SIZE

// shared:
unsigned cryptoEncryptedSize(unsigned unencryptedSize);
const byte* cryptoClientPublicKey(const Crypto* crypto);
byte* nullable cryptoEncrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize, bool server); // returns encryptedSize()-sized encrypted bytes
byte* nullable cryptoDecrypt(Crypto* crypto, const byte* bytes, unsigned bytesSize, bool server); // consumes what is returned by encrypt
void cryptoFillWithRandomBytes(byte* filled, unsigned size);
void cryptoDestroy(Crypto* crypto);

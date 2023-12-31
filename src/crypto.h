/*
 * Exchatge - a secured realtime message exchanger (desktop client).
 * Copyright (C) 2023  Vadim Nikolaev (https://github.com/vadniks)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include "defs.h"

extern const unsigned CRYPTO_KEY_SIZE;
extern const unsigned CRYPTO_HEADER_SIZE;
extern const unsigned CRYPTO_SIGNATURE_SIZE;
extern const unsigned CRYPTO_STREAMS_STATES_SIZE;
extern const unsigned CRYPTO_HASH_SIZE;

struct CryptoKeys_t;
typedef struct CryptoKeys_t CryptoKeys;

struct CryptoCoderStreams_t;
typedef struct CryptoCoderStreams_t CryptoCoderStreams;

struct CryptoBundle_t;
typedef struct CryptoBundle_t CryptoBundle; // unifies keys and coder streams

// shared:
void cryptoInit(void); // initialize the module
CryptoKeys* cryptoKeysInit(void);
CryptoCoderStreams* cryptoCoderStreamsInit(void);
CryptoBundle* cryptoBundleInit(const CryptoKeys* keys, const CryptoCoderStreams* coderStreams); // copies objects themselves, not just their pointers

// shared
CryptoKeys* cryptoBundleKeys(CryptoBundle* bundle); // doesn't modify the object itself but returns non-const pointer to it's part
CryptoCoderStreams* cryptoBundleCoderStreams(CryptoBundle* bundle); // the same

// as client:
void cryptoSetServerSignPublicKey(const byte* xServerSignPublicKey, unsigned serverSignPublicKeySize); // must be called before performing any client side operations
bool cryptoExchangeKeys(CryptoKeys* keys, const byte* serverPublicKey); // returns true on success
byte* nullable cryptoInitializeCoderStreams(const CryptoKeys* keys, CryptoCoderStreams* coderStreams, const byte* serverStreamHeader); // expects a HEADER_SIZE-sized server header's bytes, returns a deallocation-required HEADER-SIZE-sized client header's bytes on success and null otherwise
bool cryptoCheckServerSignedBytes(const byte* signature, const byte* unsignedBytes, unsigned unsignedSize);

// as an autonomous client (without need for server)
byte* cryptoMakeKey(const byte* passwordBuffer, unsigned size); // makes KEY_SIZE-sized key (deallocation's required) from a 'size'-sized password
void cryptoSetUpAutonomous(CryptoCoderStreams* coderStreams, const byte* key, const byte* nullable streamsStates); // sets up for standalone encryption with either creation of new encryption/decryption streams or with recreation of the existed ones, in which case the streamsStates mustn't be null; key must be a KEY_SIZE-sized byte array
byte* cryptoExportStreamsStates(const CryptoCoderStreams* coderStreams); // exports encryption/decryption streams states in a byte array form with size of STREAMS_STATES_SIZE which requires freeing

// as 'server', kinda ('cause sodium has ..._client_... & ..._server_..., using only client api won't work - tested):
const byte* cryptoGenerateKeyPairAsServer(CryptoKeys* keys); // returns public key that must be deallocated, acts as a server
bool cryptoExchangeKeysAsServer(CryptoKeys* keys, const byte* clientPublicKey); // returns true on success
byte* nullable cryptoCreateEncoderAsServer(const CryptoKeys* keys, CryptoCoderStreams* coderStreams); // returns encoder header on success, deallocation's needed
bool cryptoCreateDecoderStreamAsServer(const CryptoKeys* keys, CryptoCoderStreams* coderStreams, const byte* clientStreamHeader); // returns true on success, expects client's encoderr stream header with size of HEADER_SIZE

// shared:
unsigned cryptoEncryptedSize(unsigned unencryptedSize);
const byte* cryptoClientPublicKey(const CryptoKeys* keys);
byte* nullable cryptoEncrypt(CryptoCoderStreams* coderStreams, const byte* bytes, unsigned bytesSize, bool server); // returns encryptedSize()-sized encrypted bytes
byte* nullable cryptoDecrypt(CryptoCoderStreams* coderStreams, const byte* bytes, unsigned bytesSize, bool server); // consumes what is returned by encrypt
void cryptoFillWithRandomBytes(byte* filled, unsigned size);
unsigned cryptoSingleEncryptedSize(unsigned unencryptedSize);
byte* nullable cryptoEncryptSingle(const byte* key, const byte* bytes, unsigned bytesSize); // used to encrypt a single message, returns mac (tag) + encrypted bytes + nonce
byte* nullable cryptoDecryptSingle(const byte* key, const byte* bytes, unsigned bytesSize); // used to decrypt a single message, consumes what is returned by encrypt
char* cryptoBase64Encode(const byte* bytes, unsigned bytesSize); // returns newly allocated null-terminated string
byte* nullable cryptoBase64Decode(const char* encoded, unsigned encodedSize, unsigned* xDecodedSize); // also accepts pointer to a variable in which the size of the decoded bytes will be stored
void* nullable cryptoHashMultipart(void* nullable previous, const byte* nullable bytes, unsigned size); // init - (null, null, any) - returns heap-allocated state, update - (state, bytes, sizeof(bytes)) - returns null, finish - (state, null, any) - frees the state and returns heap-allocated hash (cast to byte*)

// shared
void cryptoKeysDestroy(CryptoKeys* keys); // fills the memory region, occupied by the object, with random data and then frees that area
void cryptoCoderStreamsDestroy(CryptoCoderStreams* coderStreams);
void cryptoBundleDestroy(CryptoBundle* bundle);
void cryptoClean(void); // deinitialize the module

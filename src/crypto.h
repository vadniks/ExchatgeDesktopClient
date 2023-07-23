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

struct Crypto_t;
typedef struct Crypto_t Crypto;

// shared:
Crypto* nullable cryptoInit(void);

// as client:
bool cryptoExchangeKeys(Crypto* crypto, const byte* serverPublicKey); // returns true on success
byte* nullable cryptoInitializeCoderStreams(Crypto* crypto, const byte* serverStreamHeader); // expects a HEADER_SIZE-sized server header's bytes, returns a deallocation-required HEADER-SIZE-sized client header's bytes on success and null otherwise
bool cryptoCheckServerSignedBytes(const byte* signature, const byte* unsignedBytes, unsigned unsignedSize);

// as an autonomous client (without need for server)
byte* cryptoMakeKey(const byte* passwordBuffer, unsigned size); // makes KEY_SIZE-sized key (deallocation's required) from a 'size'-sized password
void cryptoSetUpAutonomous(Crypto* crypto, const byte* key, const byte* nullable streamsStates); // sets up for standalone encryption with either creation of new encryption/decryption streams or with recreation of the existed ones, in which case the streamsStates mustn't be null; key must be a KEY_SIZE-sized byte array
byte* cryptoExportStreamsStates(const Crypto* crypto); // exports encryption/decryption streams states in a byte array form with size of STREAMS_STATES_SIZE which requires freeing
const byte* cryptoClientKey(const Crypto* crypto); // returns client key which used for creating secret streams

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
unsigned cryptoSingleEncryptedSize(unsigned unencryptedSize);
byte* nullable cryptoEncryptSingle(const byte* key, const byte* bytes, unsigned bytesSize); // used to encrypt a single message, returns mac (tag) + encrypted bytes + nonce
byte* nullable cryptoDecryptSingle(const byte* key, const byte* bytes, unsigned bytesSize); // used to decrypt a single message, consumes what is returned by encrypt
void cryptoDestroy(Crypto* crypto);

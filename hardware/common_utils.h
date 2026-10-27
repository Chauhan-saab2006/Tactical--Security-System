#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include "mbedtls/aes.h"
#include "mbedtls/base64.h"

// Keys (256-bit = 32 chars)
const char* master_midway_key = "MIDWAY_MASTER_KEY_00000000000000";
const char* wristband_key     = "WRISTBAND_KEY_12345_000000000000";
const char* node_a_key        = "NODE_A_KEY_12345678_000000000000";
const char* node_b_key        = "NODE_B_KEY_12345678_000000000000";
const char* node_c_key        = "NODE_C_KEY_12345678_000000000000";

// Standard IV (16 bytes)
unsigned char standard_iv[16] = {'i','v','s','e','c','r','e','t','i','v','s','e','c','r','e','t'};

// --- PKCS7 Padding ---
String blockPadding(String data) {
    int len = data.length();
    int padLength = 16 - (len % 16);
    for(int i = 0; i < padLength; i++) {
        data += (char)padLength;
    }
    return data;
}

String removePadding(unsigned char* data, int len) {
    uint8_t pad = (uint8_t)data[len - 1];
    if(pad > 0 && pad <= 16) {
        data[len - pad] = '\0';
    }
    return String((char*)data);
}

// --- Encrypt AES-256-CBC ---
String encryptPayload(String rawText, const char* keyStr) {
    String paddedData = blockPadding(rawText);
    int padLen = paddedData.length();
    
    unsigned char output[512] = {0};
    unsigned char iv_copy[16];
    memcpy(iv_copy, standard_iv, 16);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*)keyStr, 256);
    
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padLen, iv_copy, (const unsigned char*)paddedData.c_str(), output);
    mbedtls_aes_free(&aes);

    // Base64 Encode
    size_t b64_len = 0;
    unsigned char b64_buf[512] = {0};
    mbedtls_base64_encode(b64_buf, 512, &b64_len, output, padLen);
    
    return String((char*)b64_buf);
}

// --- Decrypt AES-256-CBC ---
String decryptPayload(String b64Text, const char* keyStr) {
    unsigned char decoded[512] = {0};
    size_t decodeLen = 0;
    
    mbedtls_base64_decode(decoded, 512, &decodeLen, (const unsigned char*)b64Text.c_str(), b64Text.length());

    unsigned char output[512] = {0};
    unsigned char iv_copy[16];
    memcpy(iv_copy, standard_iv, 16);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, (const unsigned char*)keyStr, 256);

    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, decodeLen, iv_copy, decoded, output);
    mbedtls_aes_free(&aes);

    return removePadding(output, decodeLen);
}

// --- Relay History Cache ---
// Avoid duplication errors in multi-path mesh
#define MAX_CACHE 10
String relayCache[MAX_CACHE];
int cacheIdx = 0;

bool isPacketDuplicate(String payload) {
    String sig = payload.substring(0, min((unsigned int)12, payload.length()));
    for(int i = 0; i < MAX_CACHE; i++) {
        if(relayCache[i] == sig) return true;
    }
    relayCache[cacheIdx] = sig;
    cacheIdx = (cacheIdx + 1) % MAX_CACHE;
    return false;
}

#endif

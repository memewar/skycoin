
#include "cipher.testsuite.testsuite.go.h"

void deserialize_string(json_value* value, GoString* s) {
  if (value == NULL || value->type != json_string) {
    s->n = 0;
    // FIXME: this satisfies 'all buffers allocated' contract
    s->p = (const char *) calloc(1, 1);
  } else {
    s->n = value->u.string.length;
    s->p = (const char *) calloc(s->n + 1, 1);
    memcpy((void *) s->p, (void *)value->u.string.ptr, s->n);
    // Append NULL char , just in case
    ((char *) s->p)[s->n] = 0;
  }
}

// FIXME: Move elsewhere
json_value* loadGoldenFile(const char* file) {
  char path[FILENAME_MAX];
  if(strlen(TEST_DATA_DIR) + strlen(file) < FILENAME_MAX){
    strcat( strcpy(path, TEST_DATA_DIR) , file );
    return loadJsonFile(path);
  }
  return NULL;
}

// Deserialize InputTestData JSON representation
InputTestDataJSON* jsonToInputTestData(json_value* json, InputTestDataJSON* input_data) {
  if (json->type != json_object) {
    return NULL;
  }
  json_value* hashes = json_get_string(json, "hashes");
  if (hashes == NULL || hashes->type != json_array) {
    return NULL;
  }
  int i = 0,
      length = hashes->u.array.length;
  json_value** hashstr_value = hashes->u.array.values;
  input_data->Hashes.len = input_data->Hashes.cap = length;
  input_data->Hashes.data = calloc(length, sizeof(GoString));
  GoString* s = (GoString *) input_data->Hashes.data;
  for (; i < length; i++, hashstr_value++) {
    if ((*hashstr_value)->type != json_string) {
      // String value expected. Skip everything else
      --input_data->Hashes.len;
    } else {
      deserialize_string(*hashstr_value, s);
      s++;
    }
  }
  return input_data;
}

// Mark all elements of input data for disposal
//
// Cleanup is consistent with InputTestDataToJSON
InputTestData* registerInputTestDataCleanup(InputTestData* input_data) {
  registerMemCleanup(input_data->Hashes.data);
}

// Mark all elements of input data for disposal
//
// Cleanup is consistent with InputTestDataFromJSON
InputTestDataJSON* registerInputTestDataJSONCleanup(InputTestDataJSON* input_data) {
  int i = 0,
      length = input_data->Hashes.len;
  GoString* s = input_data->Hashes.data;
  for (; i < length; ++i, s++) {
    registerMemCleanup((void *) s->p);
  }
  registerMemCleanup(input_data->Hashes.data);
}

// InputTestDataToJSON converts InputTestData to InputTestDataJSON
//
// Allocated memory has to be disposed for:
//
// - input_data.len * sizeof(GoString_) bytes for the strings slice data
// - Buffers to store individual string data
void InputTestDataToJSON(InputTestData* input_data, InputTestDataJSON* json_data) {
  GoSlice* hashes = &input_data->Hashes;
  GoSlice* hexstrings = &json_data->Hashes;
  GoString_* s = hexstrings->data = calloc(hashes->len, sizeof(GoString_));
  hexstrings->len = hexstrings->cap = hashes->len;

  cipher__SHA256* hash = hashes->data;

  int i;
  for (i = 0; i < hashes->len; i++, hash++, s++) {
    SKY_cipher_SHA256_Hex(hash, s);
  }
}

// InputTestDataFromJSON converts InputTestDataJSON to InputTestData
//
// Allocated memory has to be disposed for:
//
// - json_data.len * sizeof(cipher_SHA256) bytes for the strings slice data
GoUint32 InputTestDataFromJSON(InputTestDataJSON* json_data, InputTestData* input_data) {
  GoSlice *hexstrings = &json_data->Hashes;
  GoSlice* hashes = &input_data->Hashes; 
  cipher__SHA256* hash = hashes->data = calloc(hexstrings->len, sizeof(cipher__SHA256));
  hashes->len = hashes->cap = hexstrings->len;

  GoString* s = hexstrings->data;

  int i;
  GoUint32 err = SKY_OK;
  for (i = 0; i < hexstrings->len && err == SKY_OK; i++, s++, hash++) {
    err = SKY_cipher_SHA256FromHex(*s, hash);
  }
  if (err != SKY_OK)
    free(hashes->data);
  return err;
}

// Deserialize KeysTestData JSON representation
KeysTestDataJSON* jsonToKeysTestData(json_value* json, KeysTestDataJSON* input_data) {
  if (json->type != json_object) {
    return NULL;
  }
  json_value* value = json_get_string(json, "address");
  deserialize_string(value, &input_data->Address);
  value = json_get_string(json, "secret");
  deserialize_string(value, &input_data->Secret);
  value = json_get_string(json, "public");
  deserialize_string(value, &input_data->Public);

  value = json_get_string(json, "signatures");
  int i = 0,
      length = value->u.array.length;
  json_value** array_value = value->u.array.values;
  input_data->Signatures.len = input_data->Signatures.cap = length;
  input_data->Signatures.data = calloc(length, sizeof(GoString));
  GoString* s = (GoString *) input_data->Signatures.data;
  for (; i < length; i++, array_value++) {
    if ((*array_value)->type != json_string) {
      // String value expected. Skip everything else
      --input_data->Signatures.len;
    } else {
      deserialize_string(*array_value, s);
      s++;
    }
  }
  return input_data;
}

// Mark all elements of input data for disposal
//
// Cleanup is consistent with KeysTestDataFromJSON
KeysTestData* registerKeysTestDataCleanup(KeysTestData* input_data) {
  registerMemCleanup(input_data->Signatures.data);
}

// Mark all elements of input data for disposal
//
// Cleanup is consistent with KeysTestDataFromJSON
KeysTestDataJSON* registerKeysTestDataJSONCleanup(KeysTestDataJSON* input_data) {
  registerMemCleanup((void*) input_data->Address.p);
  registerMemCleanup((void*) input_data->Secret.p);
  registerMemCleanup((void*) input_data->Public.p);

  int i = 0,
      length = input_data->Signatures.len;
  GoString* s = input_data->Signatures.data;
  for (; i < length; ++i, s++) {
    registerMemCleanup((void *) s->p);
  }
  registerMemCleanup(input_data->Signatures.data);
}

// KeysTestDataToJSON converts KeysTestData to KeysTestDataJSON
//
// Allocated memory has to be disposed for:
//
// - input_data.Signatures.len * sizeof(GoString_) bytes for the strings slice data
// - Buffers to store individual string data
// - Buffer to store address hex string data
// - Buffer to store pubkey hex string data
// - Buffer to store seckey secret hex string data
void KeysTestDataToJson(KeysTestData* input_data, KeysTestDataJSON* json_data) {
  SKY_cipher_Address_String(&input_data->Address, (GoString_*) &json_data->Address);
  SKY_cipher_SecKey_Hex(&input_data->Secret, (GoString_*) &json_data->Secret);
  SKY_cipher_PubKey_Hex(&input_data->Public, (GoString_*) &json_data->Public);

  json_data->Signatures.len = json_data->Signatures.cap = input_data->Signatures.len;
  GoString* s = json_data->Signatures.data = calloc(input_data->Signatures.len, sizeof(GoString));

  cipher__Sig* sig = (cipher__Sig*) input_data->Signatures.data;
  int i;

  for (i = 0; i < input_data->Signatures.len; i++, sig++, s++) {
    SKY_cipher_Sig_Hex(sig, (GoString_*) s);
  }
} 

// KeysTestDataFromJSON converts KeysTestDataJSON to KeysTestData
//
//
// Allocated memory has to be disposed for:
//
// - json_data.Signatures.len * sizeof(cipher__Sig) bytes for sigs slice data
GoUint32 KeysTestDataFromJSON(KeysTestDataJSON* json_data, KeysTestData* input_data) {
  GoUint32 err = SKY_cipher_DecodeBase58Address(json_data->Address, &input_data->Address);
  if (err != SKY_OK)
    return err;
  err = SKY_cipher_SecKeyFromHex(json_data->Secret, &input_data->Secret);
  if (err != SKY_OK)
    return err;
  err = SKY_cipher_PubKeyFromHex(json_data->Public, &input_data->Public);
  if (err != SKY_OK)
    return err;

  input_data->Signatures.len = input_data->Signatures.cap = json_data->Signatures.len;
  input_data->Signatures.data = calloc(input_data->Signatures.len, sizeof(GoString));
  cipher__Sig* sig = (cipher__Sig*) input_data->Signatures.data;

  GoString* s = (GoString*) json_data->Signatures.data;
  int i;
  err = SKY_OK;

  for (i = 0; i < json_data->Signatures.len && err == SKY_OK; i++, sig++, s++) {
    SKY_cipher_SigFromHex(*s, sig);
  }
  if (err != SKY_OK)
    free(input_data->Signatures.data);
  return err;
}

// Deserialize SeedTestData JSON representation
SeedTestDataJSON* jsonToSeedTestData(json_value* json, SeedTestDataJSON* input_data) {
  if (json->type != json_object) {
    return NULL;
  }
  json_value* value = json_get_string(json, "seed");
  deserialize_string(value, &input_data->Seed);

  value = json_get_string(json, "keys");
  int i = 0,
      length = value->u.array.length;
  json_value** array_value = value->u.array.values;
  input_data->Keys.len = input_data->Keys.cap = length;
  input_data->Keys.data = calloc(length, sizeof(KeysTestDataJSON));
  KeysTestDataJSON* kd = (KeysTestDataJSON*) input_data->Keys.data;
  for (; i < length; i++, array_value++) {
    if ((*array_value)->type != json_object) {
      // String value expected. Skip everything else
      --input_data->Keys.len;
    } else {
      jsonToKeysTestData(*array_value, kd);
      kd++;
    }
  }
  return input_data;
}

// Mark all elements of input data for disposal
//
// Cleanup is consistent with SeedTestDataFromJSON
SeedTestData* registerSeedTestDataCleanup(SeedTestData* input_data) {
  registerMemCleanup(input_data->Seed.data);

  int i = 0,
      length = input_data->Keys.len;
  KeysTestData* kd = input_data->Keys.data;
  for (; i < length; ++i, kd++) {
    registerKeysTestDataCleanup(kd);
  }
  registerMemCleanup(input_data->Keys.data);
}

// Mark all elements of input data for disposal
//
// Cleanup is consistent with SeedTestDataFromJSON
SeedTestDataJSON* registerSeedTestDataJSONCleanup(SeedTestDataJSON* input_data) {
  registerMemCleanup((void*) input_data->Seed.p);

  int i = 0,
      length = input_data->Keys.len;
  KeysTestDataJSON* kd = input_data->Keys.data;
  for (; i < length; ++i, kd++) {
    registerKeysTestDataJSONCleanup((void*) kd);
  }
  registerMemCleanup(input_data->Keys.data);
}

// SeedTestDataToJSON converts SeedTestData to SeedTestDataJSON
//
// Allocated memory has to be disposed for:
//
// - Buffer to store seed hex data
// - input_data.Keys.len * sizeof(KeysTestDataJSON) bytes for keys test data slice
// - Memory requirements to allocate JSON data for instances of KeysTestDataJSON in Keys
//   see KeysTestDataToJSON
void SeedTestDataToJson(SeedTestData* input_data, SeedTestDataJSON* json_data) {
  json_data->Keys.len = json_data->Keys.cap = input_data->Keys.len;
  json_data->Keys.data = calloc(input_data->Keys.len, sizeof(KeysTestDataJSON));
  KeysTestDataJSON* kj = (KeysTestDataJSON*) json_data->Keys.data;

  KeysTestData* k = (KeysTestData*) input_data->Keys.data;
  int i;

  for (i = 0; i < input_data->Keys.len; i++, k++, kj++) {
    KeysTestDataToJson(k, kj);
  }

  unsigned int b64seed_size = b64e_size(input_data->Seed.len + 1) + 1;
  json_data->Seed.p = malloc(b64seed_size);
  json_data->Seed.n = b64_encode((const unsigned int*) input_data->Seed.data,
      input_data->Seed.len, input_data->Seed.data);
} 

// SeedTestDataFromJSON converts SeedTestDataJSON to SeedTestData
//
//
// Allocated memory has to be disposed for:
//
// - Seed slice bytes buffer
// - json_data.Keys.len * sizeof(cipher__KeysTestData) bytes for keys test slice data
// - Memory requirements to allocate individual instances of KeyTestData in Keys
//   see KeysTestDataFromJSON
GoUint32 SeedTestDataFromJSON(SeedTestDataJSON* json_data, SeedTestData* input_data) {
  unsigned int b64seed_size = b64d_size(json_data->Seed.n) + 1;
  input_data->Seed.cap = b64seed_size;
  input_data->Seed.len = b64_decode(json_data->Seed.p, json_data->Seed.n,
      input_data->Seed.data);

  input_data->Keys.len = input_data->Keys.cap = json_data->Keys.len;
  input_data->Keys.data = calloc(input_data->Keys.len, sizeof(KeysTestData));
  KeysTestData* k = (KeysTestData*) input_data->Keys.data;

  KeysTestDataJSON* kj = (KeysTestDataJSON*) json_data->Keys.data;
  int i;
  GoUint32 err = SKY_OK;

  for (i = 0; i < json_data->Keys.len && err == SKY_OK; i++, k++, kj++) {
    err = KeysTestDataFromJSON(kj, k);
  }
  if (err != SKY_OK)
    free(input_data->Keys.data);
  return err;
}

// ValidateSeedData validates the provided SeedTestData against the current cipher library.
// inputData is required if SeedTestData contains signatures
void ValidateSeedData(SeedTestData* seedData, InputTestData* inputData) {
  cipher__PubKey pubkey;
  cipher__SecKey seckey;
  GoSlice keys;

  SKY_cipher_GenerateDeterministicKeyPairs(seedData->Seed, seedData->Keys.len, (GoSlice_*) &keys);
  registerMemCleanup(keys.data);

  cr_assert(eq(llong, seedData->Keys.len, keys.len),
      "cipher.GenerateDeterministicKeyPairs must generate expected number of keys");

  cipher__SecKey  skNull;
  cipher__PubKey  pkNull;
  cipher__Address addrNull;
  cipher__Sig     sigNull;

  memset((void *) &skNull, 0, sizeof(cipher__SecKey));
  memset((void *)&pkNull, 0, sizeof(cipher__PubKey));
  memset((void *)&addrNull, 0, sizeof(cipher__Address));
  memset((void *)&sigNull, 0, sizeof(cipher__Sig));

  int i = 0;
  KeysTestData* expected = (KeysTestData*) seedData->Keys.data;
  cipher__SecKey *s = (cipher__SecKey*) keys.data;
  for (; i < keys.len; i++, s++, expected++) {
    cr_assert(ne(u8[32], skNull, (*s)),
        "secret key must not be null");
    cr_assert(eq(u8[32], expected->Secret, (*s)),
        "generated secret key must match provided secret key");

    cipher__PubKey p;
    SKY_cipher_PubKeyFromSecKey(s, &p);
    cr_assert(ne(u8[33], pkNull, p),
        "public key must not be null");
    cr_assert(eq(u8[33], expected->Public, p),
        "derived public key must match provided public key");

    cipher__Address addr1;
    SKY_cipher_AddressFromPubKey(&p, &addr1);
    cr_assert(ne(type(cipher__Address), addrNull, addr1),
        "address from pubkey must not be null");
    cr_assert(eq(type(cipher__Address), expected->Address, addr1),
        "derived address must match provided address");

    cipher__Address addr2;
    SKY_cipher_AddressFromSecKey(s, &addr2);
    cr_assert(ne(type(cipher__Address), addrNull, addr1),
        "address from sec key must not be null");
    cr_assert(eq(type(cipher__Address), addr1, addr2),
        "cipher.AddressFromPubKey and cipher.AddressFromSecKey must generate same addresses");

    // TODO : Translate once secp256k1 be part of libskycoin
    /*
    validSec := secp256k1.VerifySeckey(s[:])
    if validSec != 1 {
      return errors.New("secp256k1.VerifySeckey failed")
    }

    validPub := secp256k1.VerifyPubkey(p[:])
    if validPub != 1 {
      return errors.New("secp256k1.VerifyPubkey failed")
    }
    */

    // FIXME: without cond : not give a valid preprocessing token
    bool cond = (!(inputData == NULL && expected->Signatures.len != 0));
    cr_assert(cond, "seed data contains signatures but input data was not provided");

    if (inputData != NULL) {
      cr_assert(expected->Signatures.len == inputData->Hashes.len,
          "Number of signatures in seed data does not match number of hashes in input data");

      cipher__SHA256* h = (cipher__SHA256*) inputData->Hashes.data;
      cipher__Sig* sig = (cipher__Sig*) expected->Signatures.data;
      int j = 0;
      for (; j < inputData->Hashes.len; j++, h++, sig++) {
        cr_assert(ne(u8[65], (*sig), sigNull), "provided signature must not be null");
        GoUint32 err = SKY_cipher_VerifySignature(&p, sig, h);
        cr_assert(err == SKY_OK, "cipher.VerifySignature failed: %d", err);
        err = SKY_cipher_ChkSig(&addr1, h, sig);
        cr_assert(err == SKY_OK, "cipher.ChkSig failed: %d", err);
        err = SKY_cipher_VerifySignedHash(sig, h);
        cr_assert(err == SKY_OK, "cipher.VerifySignedHash failed: %d", err);

        cipher__PubKey p2;
        err = SKY_cipher_PubKeyFromSig(sig, h, &p2);
        cr_assert(err == SKY_OK, "cipher.PubKeyFromSig failed: %d", err);
        cr_assert(eq(u8[32], p, p2),
          "public key derived from signature must match public key derived from secret", err);

        cipher__Sig sig2;
        SKY_cipher_SignHash(h, s, &sig2);
        cr_assert(ne(u8[65], sigNull, sig2), "created signature is null");

        // NOTE: signatures are not deterministic, they use a nonce,
        // so we don't compare the generated sig to the provided sig
      }
    }
  }
}

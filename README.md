# FPE - Format Preserving Encryption Implementation in C via FF3-1


An implementation of the NIST approved Format Preserving Encryption (FPE) FF1, FF3  and FF3-1 algorithms in C.

NIST decided to update FF3 (called FF3-1) to address the vulnerability in a new version of SP 800-38Grev1. This announcement also noted that FF3 is no longer suitable as a general-purpose FPE method.

Vendors shall not claim any compliance to FF3 in an approved mode of operation.

SP 800-38G requires that the underlying encryption function used in the format-preserving algorithms is
an approved block cipher operating on the 128-bit blocks of data. As of August 2020, and for the
foreseeable future, the only such function is the AES algorithm. The length of an AES key may be 128, 192, or 256 bits.


[NIST Recommendation SP 800-38G](http://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-38G.pdf)

This follows the FF1 and FF3 schemes for Format Preserving Encryption outlined in the NIST Recommendation, released in March 2016. For FF1, it builds on and formalizes (differing from but remaining mathematically equivalent to) the FFX-A10 scheme by Bellare, Rogaway and Spies as defined [here](http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/ffx/ffx-spec.pdf) and [here](http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/ffx/ffx-spec2.pdf). For FF3, it formalizes the BPS scheme.

A note about FF2: FF2 was originally NOT recommended by NIST, but it is under review again as DFF. You can read about it [here](http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/dff/dff-ff2-fpe-scheme-update.pdf).

## Example Usage

This implementation is based on openssl's BIGNUM and AES, so you need to install openssl first.

There are several functions for FF1 and FF3 algorithm, respectively.

1. Set and unset ff1 key and tweak

```c++
int FPE_set_ff1_key(const unsigned char *userKey, const int bits, const unsigned char *tweak, const unsigned int tweaklen, const int radix, FPE_KEY *key);

void FPE_unset_ff1_key(FPE_KEY *key);
```

| name     | description                              |
| -------- | ---------------------------------------- |
| userKey  | encryption key (128 bit, 192 bits or 256 bits), represented as a c string |
| bits     | length of userKey (128, 192 or 256)      |
| tweak    | tweak, represented as a c string         |
| tweaklen | the byte length of the tweak             |
| radix    | number of characters in the given alphabet, it must be in [2, 2^16] |
| key      | FPE_KEY structure                        |

2. encrypt or decrypt text using ff1 algorithm

```c++
void FPE_ff1_encrypt(unsigned int *in, unsigned int *out, unsigned int inlen, FPE_KEY *key, const int enc)
```

| name  | description                              |
| ----- | ---------------------------------------- |
| in    | numeral string to be encrypted, represented as an array of integers |
| out   | encrypted numeral string, represented as an array of integers |
| inlen | the length of input numeral string (in)  |
| key   | FPE_KEY structure that have been set with key and tweak |
| enc   | can be two value: FPE_ENCRYPT for encrypt and FPE_DECRYPT for decrypt |

3. Set and unset ff3 key and tweak

```c++
int FPE_set_ff3_key(const unsigned char *userKey, const int bits, const unsigned char *tweak, const unsigned int radix, FPE_KEY *key);

void FPE_unset_ff3_key(FPE_KEY *key);
```

| name    | description                              |
| ------- | ---------------------------------------- |
| userKey | encryption key (128 bit, 192 bits or 256 bits), represented as a c string |
| bits    | length of userKey (128, 192 or 256)      |
| tweak   | tweak, represented as a c string (it must be 64 bytes) |
| radix   | number of characters in the given alphabet, it must be in [2, 2^16] |
| key     | FPE_KEY structure                        |

4. encrypt or decrypt text using ff3 algorithm

```c++
void FPE_ff3_encrypt(unsigned int *in, unsigned int *out, unsigned int inlen, FPE_KEY *key, const int enc);
```

| name  | description                              |
| ----- | ---------------------------------------- |
| in    | numeral string to be encrypted, represented as an array of integers |
| out   | encrypted numeral string, represented as an array of integers |
| inlen | the length of input numeral string (in)  |
| key   | FPE_KEY structure that have been set with key and tweak |
| enc   | can be two value: FPE_ENCRYPT for encrypt and FPE_DECRYPT for decrypt |


5. Set and unset ff31 key and tweak

```c++
int FPE_set_ff31_key(const unsigned char *userKey, const int bits, const unsigned char *tweak, const unsigned int radix, FPE_KEY *key);

void FPE_unset_ff31_key(FPE_KEY *key);
```

| name    | description                              |
| ------- | ---------------------------------------- |
| userKey | encryption key (128 bit, 192 bits or 256 bits), represented as a c string |
| bits    | length of userKey (128, 192 or 256)      |
| tweak   | tweak, represented as a c string (it must be 56 bytes) |
| radix   | number of characters in the given alphabet,  if the key is used within an IETF protocol; it must be in [2, 2^16] // maxRadix   = 65536 // 2^16 |
| key     | FPE_KEY structure                        |

6. encrypt or decrypt text using ff31 algorithm

```c++
void FPE_ff31_encrypt(unsigned int *in, unsigned int *out, unsigned int inlen, FPE_KEY *key, const int enc);
```

| name  | description                              |
| ----- | ---------------------------------------- |
| in    | numeral string to be encrypted, represented as an array of integers |
| out   | encrypted numeral string, represented as an array of integers |
| inlen | the length of input numeral string (in)  |
| key   | FPE_KEY structure that have been set with key and tweak |
| enc   | can be two value: FPE_ENCRYPT for encrypt and FPE_DECRYPT for decrypt |


The example code is [example.c](https://github.com/0NG/Format-Preserving-Encryption/blob/master/example.c). Also, there are some official [test vectors](http://csrc.nist.gov/groups/ST/toolkit/examples.html) for both FF1 and FF3 provided by NIST. You can run [test.py](https://github.com/0NG/Format-Preserving-Encryption/blob/master/test.py) with python 3.x.

To compile the example.c with the fpe library, just run *make example* or *make*.

**Run [test.py](https://github.com/0NG/Format-Preserving-Encryption/blob/master/test.py) for testing with official test vectors.**

## Existing Implementations

Based on searching GitHub and the Internet, there are no known reference implementations for either algorithm.

An [existing C++ implementation](https://github.com/randombit/botan/tree/753b4c2d5301574d3c9390b79aa275a49809e6c8/src/lib/misc/fpe_fe1) based on the FFX mode, but the implementation differs from the NIST recommendation. 

Also, another [implementation in Go](https://github.com/capitalone/fpe) is great. 

Rust FPE Implementation  https://github.com/str4d/fpe

## TODO

1. Create RUST Wrapper

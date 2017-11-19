#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <openssl/aes.h>
#include <openssl/bn.h>
#include "fpe.h"
#include "fpe_locl.h"

// convert numeral string to number
void str2num(BIGNUM *Y, const unsigned int *X, unsigned long long radix, unsigned int len, BN_CTX *ctx)
{
    BN_CTX_start(ctx);
    BIGNUM *r = BN_CTX_get(ctx),
           *x = BN_CTX_get(ctx);

    BN_set_word(Y, 0);
    BN_set_word(r, radix);
    for (int i = 0; i < len; ++i) {
        // Y = Y * radix + X[i]
        BN_set_word(x, X[i]);
        BN_mul(Y, Y, r, ctx);
        BN_add(Y, Y, x);
    }

    BN_CTX_end(ctx);
    return;
}

// convert number to numeral string
void num2str(const BIGNUM *X, unsigned int *Y, unsigned int radix, int len, BN_CTX *ctx)
{
    BN_CTX_start(ctx);
    BIGNUM *dv = BN_CTX_get(ctx),
           *rem = BN_CTX_get(ctx),
           *r = BN_CTX_get(ctx),
           *XX = BN_CTX_get(ctx);

    BN_copy(XX, X);
    BN_set_word(r, radix);
    memset(Y, 0, len * 4);
    
    for (int i = len - 1; i >= 0; --i) {
        // XX / r = dv ... rem
        BN_div(dv, rem, XX, r, ctx);
        // Y[i] = XX % r
        Y[i] = BN_get_word(rem);
        // XX = XX / r
        BN_copy(XX, dv);
    }

    BN_CTX_end(ctx);
    return;
}

void FF1_encrypt(const unsigned int *in, unsigned int *out, const unsigned char *key, const unsigned char *tweak, const unsigned int radix, size_t inlen, size_t tweaklen)
{
    BIGNUM *bnum = BN_new(),
           *y = BN_new(),
           *c = BN_new(),
           *anum = BN_new(),
           *qpow_u = BN_new(),
           *qpow_v = BN_new();
    BN_CTX *ctx = BN_CTX_new();

    memcpy(out, in, inlen * 4);
    int u = inlen / 2;
    int v = inlen - u;
    unsigned int *A = out, *B = out + u;
    pow_uv(qpow_u, qpow_v, radix, u, v, ctx);

    const int b = ceil(v * bits(radix) / 8.0);
    const int d = 4 * ceil(b / 4.0) + 4;

    int pad = ( (-tweaklen - b - 1) % 16 + 16 ) % 16;
    int Qlen = tweaklen + pad + 1 + b;
    unsigned char P[16];
    unsigned char *Q = (unsigned char *)malloc(Qlen), *Bytes = (unsigned char *)malloc(b);

    // initialize P
    P[0] = 0x1;
    P[1] = 0x2;
    P[2] = 0x1;
    *( (unsigned int *)(P + 3) ) = (radix << 8) | 10;
    P[7] = u % 256;
    *( (unsigned int *)(P + 8) ) = inlen;
    *( (unsigned int *)(P + 12) ) = tweaklen;

    // initialize Q
    memcpy(Q, tweak, tweaklen);
    memset(Q + tweaklen, 0x00, pad);
    assert(tweaklen + pad - 1 <= Qlen);

    unsigned char iv[16], R[16];
    AES_KEY aes_enc_ctx;
    AES_set_encrypt_key(key, 128, &aes_enc_ctx);
    int cnt = ceil(d / 16.0) - 1;
    int Slen = 16 + cnt * 16;
    unsigned char *S = (unsigned char *)malloc(Slen);
    for (int i = 0; i < FF1_ROUNDS; ++i) {
        // v
        int m = (i & 1)? v: u;

        // i
        Q[tweaklen + pad] = i & 0xff;
        str2num(bnum, B, radix, inlen - m, ctx);
        int BytesLen = BN_bn2bin(bnum, Bytes);
        memset(Q + Qlen - b, 0x00, b);

        int qtmp = Qlen - BytesLen;
        memcpy(Q + qtmp, Bytes, BytesLen);

        // ii PRF(P || Q), P is always 16 unsigned chars long
        memset(iv, 0x00, sizeof(iv));
        memset(R, 0x00, sizeof(R));
        AES_cbc_encrypt(P, R, 16, &aes_enc_ctx, iv, AES_ENCRYPT);
        int count = Qlen / 16;
        unsigned char Ri[16];
        unsigned char *Qi = Q;
        for (int cc = 0; cc < count; ++cc) {
            memset(iv, 0x00, sizeof(iv));
            for (int j = 0; j < 16; ++j)    Ri[j] = Qi[j] ^ R[j];
            AES_cbc_encrypt(Ri, R, 16, &aes_enc_ctx, iv, AES_ENCRYPT);
            Qi += 16;
        }

        // iii 
        unsigned char tmp[16], SS[16];
        memset(S, 0x00, Slen);
        memcpy(S, R, 16);
        for (int j = 1; j <= cnt; ++j) {
            memset(tmp, 0x00, 16);
            *( (unsigned int *)tmp + 3 ) = j;
            for (int k = 0; k < 16; ++k)    tmp[k] ^= R[k];
            memset(iv, 0x00, sizeof(iv));
            AES_cbc_encrypt(tmp, SS, 16, &aes_enc_ctx, iv, AES_ENCRYPT);
            assert((S + 16 * j)[0] == 0x00);
            memcpy(S + 16 * j, SS, 16);
        }

        // iv
        BN_bin2bn(S, d, y);
        // vi
        // (num(A, radix, m) + y) % qpow(radix, m);
        str2num(anum, A, radix, m, ctx);
        // anum = (anum + y) mod qpow_uv
        if (m == u)    BN_mod_add(c, anum, y, qpow_u, ctx);
        else    BN_mod_add(c, anum, y, qpow_v, ctx);

        // swap A and B
        assert(A != B);
        A = (unsigned int *)( (uintptr_t)A ^ (uintptr_t)B );
        B = (unsigned int *)( (uintptr_t)B ^ (uintptr_t)A );
        A = (unsigned int *)( (uintptr_t)A ^ (uintptr_t)B );
        num2str(c, B, radix, m, ctx);

    }

    // free the space
    BN_clear_free(anum);
    BN_clear_free(bnum);
    BN_clear_free(c);
    BN_clear_free(y);
    BN_clear_free(qpow_u);
    BN_clear_free(qpow_v);
    BN_CTX_free(ctx);
    free(Bytes);
    free(Q);
    free(S);
    return;
}

void FF1_decrypt(const unsigned int *in, unsigned int *out, const unsigned char *key, const unsigned char *tweak, const unsigned int radix, size_t inlen, size_t tweaklen)
{
    BIGNUM *bnum = BN_new(),
           *y = BN_new(),
           *c = BN_new(),
           *anum = BN_new(),
           *qpow_u = BN_new(),
           *qpow_v = BN_new();
    BN_CTX *ctx = BN_CTX_new();

    memcpy(out, in, inlen * 4);
    int u = inlen / 2;
    int v = inlen - u;
    unsigned int *A = out, *B = out + u;
    pow_uv(qpow_u, qpow_v, radix, u, v, ctx);

    const int b = ceil(v * bits(radix) / 8.0);
    const int d = 4 * ceil(b / 4.0) + 4;

    int pad = ( (-tweaklen - b - 1) % 16 + 16 ) % 16;
    int Qlen = tweaklen + pad + 1 + b;
    unsigned char P[16];
    unsigned char *Q = (unsigned char *)malloc(Qlen), *Bytes = (unsigned char *)malloc(b);
    // initialize P
    P[0] = 0x1;
    P[1] = 0x2;
    P[2] = 0x1;
    *( (unsigned int *)(P + 3) ) = (radix << 8) | 10;
    P[7] = u % 256;
    *( (unsigned int *)(P + 8) ) = inlen;
    *( (unsigned int *)(P + 12) ) = tweaklen;

    // initialize Q
    memcpy(Q, tweak, tweaklen);
    memset(Q + tweaklen, 0x00, pad);
    assert(tweaklen + pad - 1 <= Qlen);

    unsigned char iv[16], R[16];
    AES_KEY aes_enc_ctx;
    AES_set_encrypt_key(key, 128, &aes_enc_ctx);
    int cnt = ceil(d / 16.0) - 1;
    int Slen = 16 + cnt * 16;
    unsigned char *S = (unsigned char *)malloc(Slen);
    for (int i = FF1_ROUNDS - 1; i >= 0; --i) {
        // v
        int m = (i & 1)? v: u;

        // i
        Q[tweaklen + pad] = i & 0xff;
        str2num(anum, A, radix, inlen - m, ctx);
        memset(Q + Qlen - b, 0x00, b);
        int BytesLen = BN_bn2bin(anum, Bytes);
        int qtmp = Qlen - BytesLen;
        memcpy(Q + qtmp, Bytes, BytesLen);

        // ii PRF(P || Q)
        memset(iv, 0x00, sizeof(iv));
        memset(R, 0x00, sizeof(R));
        AES_cbc_encrypt(P, R, 16, &aes_enc_ctx, iv, AES_ENCRYPT);
        int count = Qlen / 16;
        unsigned char Ri[16];
        unsigned char *Qi = Q;
        for (int cc = 0; cc < count; ++cc) {
            memset(iv, 0x00, sizeof(iv));
            for (int j = 0; j < 16; ++j)    Ri[j] = Qi[j] ^ R[j];
            AES_cbc_encrypt(Ri, R, 16, &aes_enc_ctx, iv, AES_ENCRYPT);
            Qi += 16;
        }

        // iii 
        unsigned char tmp[16], SS[16];
        memset(S, 0x00, Slen);
        memcpy(S, R, 16);
        for (int j = 1; j <= cnt; ++j) {
            memset(tmp, 0x00, 16);
            *( (unsigned int *)tmp + 3 ) = j;
            for (int k = 0; k < 16; ++k)    tmp[k] ^= R[k];
            memset(iv, 0x00, sizeof(iv));
            AES_cbc_encrypt(tmp, SS, 16, &aes_enc_ctx, iv, AES_ENCRYPT);
            assert((S + 16 * j)[0] == 0x00);
            memcpy(S + 16 * j, SS, 16);
        }

        // iv
        BN_bin2bn(S, d, y);
        // vi
        // (num(B, radix, m) - y) % qpow(radix, m);
        str2num(bnum, B, radix, m, ctx);
        if (m == u)    BN_mod_sub(c, bnum, y, qpow_u, ctx);
        else    BN_mod_sub(c, bnum, y, qpow_v, ctx);

        // swap A and B
        assert(A != B);
        A = (unsigned int *)( (uintptr_t)A ^ (uintptr_t)B );
        B = (unsigned int *)( (uintptr_t)B ^ (uintptr_t)A );
        A = (unsigned int *)( (uintptr_t)A ^ (uintptr_t)B );
        num2str(c, A, radix, m, ctx);

    }

    // free the space
    BN_clear_free(anum);
    BN_clear_free(bnum);
    BN_clear_free(y);
    BN_clear_free(c);
    BN_clear_free(qpow_u);
    BN_clear_free(qpow_v);
    BN_CTX_free(ctx);
    free(Bytes);
    free(Q);
    free(S);
    return;
}

void FPE_ff1_encrypt(unsigned int *in, unsigned int *out, const unsigned char *key, const unsigned char *tweak, unsigned int radix, unsigned int inlen, unsigned int tweaklen, const int enc)
{
    if (enc)
        FF1_encrypt(in, out, key, tweak,
                    radix, inlen, tweaklen);

    else
        FF1_decrypt(in, out, key, tweak,
                    radix, inlen, tweaklen);
}

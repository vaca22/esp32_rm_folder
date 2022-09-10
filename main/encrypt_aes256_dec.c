
#include "stdlib.h"
#include "string.h"

#include "mbedtls/aes.h"
#include "mbedtls/cipher_internal.h"
#include "mbedtls/cipher.h"
#include "mbedtls/sha256.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/base64.h"

#include "encrypt_aes256_dec.h"

#define AES_ENCODE_IN_SIZE      (32*16 - 1)
#define AES_ENCODE_OUT_SIZE     (32*16)
#define os_mem_alloc malloc
#define os_mem_free  free
#define robot_log    printf

static void* encrypt_ctx_alloc_aes(void);
static void  encrypt_ctx_free_aes(mbedtls_aes_context* ctx);

typedef int (*ecb_func)( void *ctx, mbedtls_operation_t mode,
        const unsigned char *input, unsigned char *output );
typedef int (*cbc_func)( void *ctx, mbedtls_operation_t mode, size_t length,
        unsigned char *iv, const unsigned char *input,
        unsigned char *output );
typedef int (*cfb_func)( void *ctx, mbedtls_operation_t mode, size_t length, size_t *iv_off,
        unsigned char *iv, const unsigned char *input,
        unsigned char *output );
typedef int (*ctr_func)( void *ctx, size_t length, size_t *nc_off,
        unsigned char *nonce_counter, unsigned char *stream_block,
        const unsigned char *input, unsigned char *output );
typedef int (*setkey_enc_func)( void *ctx, const unsigned char *key,
        unsigned int key_bitlen );
typedef int (*setkey_dec_func)( void *ctx, const unsigned char *key,
        unsigned int key_bitlen);
typedef void (*ctx_free_func)( void *ctx );
typedef void * (*ctx_alloc_func)( void );

const struct mbedtls_cipher_base_t aes_base =
{
    .cipher = MBEDTLS_CIPHER_ID_AES,
    .ecb_func = (ecb_func)mbedtls_aes_crypt_ecb,
    .cbc_func = (cbc_func)mbedtls_aes_crypt_cbc,
    .cfb_func = (cfb_func)mbedtls_aes_crypt_cfb128,
    .ctr_func = (ctr_func)mbedtls_aes_crypt_ctr,
    //.stream_func = NULL,
    .setkey_enc_func = (setkey_enc_func)mbedtls_aes_setkey_enc,
    .setkey_dec_func = (setkey_dec_func)mbedtls_aes_setkey_dec,
    .ctx_alloc_func = (ctx_alloc_func)encrypt_ctx_alloc_aes,
    .ctx_free_func = (ctx_free_func)encrypt_ctx_free_aes,
};

const mbedtls_cipher_info_t cipher_info_aes_256_cbc = 
{
    .type       = MBEDTLS_CIPHER_AES_256_CBC,
    .mode       = MBEDTLS_MODE_CBC,
    .key_bitlen = 256,
    .name       = "aes_256",
    .iv_size    = 16,
    .flags      = 0,
    .block_size = 16,
    .base       = &aes_base,
};

const mbedtls_cipher_info_t cipher_info_aes_128_cbc = 
{
    .type       = MBEDTLS_CIPHER_AES_256_CBC,
    .mode       = MBEDTLS_MODE_CBC,
    .key_bitlen = 128,
    .name       = "aes_256",
    .iv_size    = 16,
    .flags      = 0,
    .block_size = 16,
    .base       = &aes_base,
};

static void* encrypt_ctx_alloc_aes(void)
{
    mbedtls_aes_context *ctx;  
    ctx = os_mem_alloc(sizeof(mbedtls_aes_context));
    //robot_log("[encrypt]aes_ctx_alloc: 0x%x\n", (uint32_t)ctx);
    return ctx;
}

static void encrypt_ctx_free_aes(mbedtls_aes_context* ctx)
{
    //robot_log("[encrypt]aes_ctx_free: 0x%x\n", (uint32_t)ctx);
    if(ctx != NULL)
    {
        os_mem_free(ctx);
    }
}

/*
 * 初始化aes解密对象, CBC PKCS7
 * ctx:
 *  要初始化的对象
 * mod:
 *  用来指定是加密还是解密
 *  ENCRYPT_MOD_ENC/ENCRYPT_MOD_DEC
 * key:
 *  密钥
 * keybits:
 *  用于指定定是密鉏是128/192/256
 *  ENCRYPT_AES_KEYBITS_128/192/256
 */
int32_t encrypt_aes_cipher_init(mbedtls_cipher_context_t* ctx, int32_t mod, uint8_t* key, int32_t keybits)
{
    mod =  MBEDTLS_DECRYPT;
    mbedtls_cipher_init(ctx);

            mbedtls_cipher_setup(ctx, &cipher_info_aes_256_cbc);
            mbedtls_cipher_setkey(ctx, (const unsigned char*)key, 256, mod);

    mbedtls_cipher_set_padding_mode(ctx, MBEDTLS_PADDING_PKCS7);
    mbedtls_cipher_reset(ctx);
    return 0;
}

typedef struct encrypt_aes256_struct
{
    mbedtls_cipher_context_t ctx;
    uint8_t iv[16 + 1];
    uint8_t key[32 + 1];

    encrypt_read_data_cb read_data;
    void *user_data;
    int8_t cache[AES_ENCODE_OUT_SIZE * 4];
    int32_t cache_size;
    int32_t first;
}encrypt_aes256_t;

void *encrypt_aes_256_init(int8_t *user, int8_t *password, int32_t mode, encrypt_read_data_cb read_data, void *user_data)
{
    encrypt_aes256_t *aes256 = os_mem_alloc(sizeof(encrypt_aes256_t));
    if(aes256 == NULL)
    {
        return NULL;
    }
    memset(aes256, 0, sizeof(encrypt_aes256_t));

    aes256->read_data = read_data;
    aes256->user_data = user_data;
  //  snprintf((char *)aes256->iv, sizeof(aes256->iv) - 1, "%s", user);
    memcpy(aes256->iv,user,16);
    //snprintf((char *)aes256->key, sizeof(aes256->key) - 1, "%s", password);
    for(int k=0;k<32;k++){
        aes256->key[k]=password[k];
    }
    switch(mode)
    {
        case ENCRYPT_MOD_ENC:
            encrypt_aes_cipher_init(&aes256->ctx, ENCRYPT_MOD_ENC, aes256->key, ENCRYPT_AES_KEYBITS_256);
            break;
        case ENCRYPT_MOD_DEC:
            encrypt_aes_cipher_init(&aes256->ctx, ENCRYPT_MOD_DEC, aes256->key, ENCRYPT_AES_KEYBITS_256);
            break;
        default:
            return NULL;
            break;
    }
    return aes256;
}

int32_t encrypt_aes_256_decode(void *hd, uint8_t *output, int32_t output_len)
{
    encrypt_aes256_t *aes256 = (encrypt_aes256_t *)hd;
    uint8_t data_in[AES_ENCODE_OUT_SIZE] = { 0 }, data_out[AES_ENCODE_IN_SIZE] = { 0 };
    size_t in_once = sizeof(data_in), out_once = sizeof(data_out);
    int32_t ret = 0, out_index = 0;

    if(aes256->first == 0)
    {
        robot_log("[encrypt]decode first in\n");
        aes256->first = 1;
        uint8_t *read_data = os_mem_alloc(AES_ENCODE_OUT_SIZE * 4);
        int32_t read_size = aes256->read_data(aes256->user_data, read_data, AES_ENCODE_OUT_SIZE * 4);
        if(read_size <= 0)
        {
            robot_log("[encrypt]decode 0 failed:%d\n", read_size);
            os_mem_free(read_data);
            return -1;
        }
        int32_t in_once_size = AES_ENCODE_OUT_SIZE;
        for(int i = 0; i < 4; i++)
        {
            if(read_size >= AES_ENCODE_OUT_SIZE)
            {
                in_once_size = AES_ENCODE_OUT_SIZE;
                read_size -= in_once_size;
            }
            else if(read_size <= 0)
            {
                break;
            }
            else if(read_size < AES_ENCODE_OUT_SIZE)
            {
                in_once_size = read_size;
                read_size -= in_once_size;
            }
            ret = mbedtls_cipher_crypt(&aes256->ctx, aes256->iv, 16,
                    read_data , 48, data_out, &out_once);
            if(ret != 0 )
            {
                robot_log("[encrypt]decode 0 failed:%d, %d\n", ret, out_once);
                os_mem_free(read_data);
                return -1;
            }
            memmove(aes256->cache + aes256->cache_size, data_out, out_once);
            aes256->cache_size += out_once;
        }
        os_mem_free(read_data);
        robot_log("[encrypt]decode first in ok out\n");
    }

    if(aes256->cache_size > 0)  //缓存中的数据残余
    {
        if(output_len <= aes256->cache_size)    //缓存数据超过想要的
        {
            memmove(output, aes256->cache, output_len);
            memmove(aes256->cache, aes256->cache + output_len, aes256->cache_size - output_len);
            aes256->cache_size -= output_len;
            return output_len;
        }
        else    //缓存的数据全部拿走
        {
            memmove(output, aes256->cache, aes256->cache_size);
            out_index = aes256->cache_size;
            aes256->cache_size = 0;
        }
    }

    while(1)
    {
        ret = aes256->read_data(aes256->user_data, data_in, in_once);
        if(ret <= 0)
        {
            break;
        }
        ret = mbedtls_cipher_crypt(&aes256->ctx, aes256->iv, 16, 
                data_in, in_once, data_out, &out_once);
        if(ret != 0 || out_once != sizeof(data_out))
        {
            robot_log("[encrypt]decode failed:%d, %d\n", ret, out_once);
            return -1;
        }
        if(out_index + out_once < output_len)
        {
            memmove(output + out_index, data_out, out_once);
            out_index += out_once;
        }
        else        //数据多出一部分了
        {
            memmove(output + out_index, data_out, (output_len - out_index));
            if(out_index + out_once == output_len)
            {
                out_index += (output_len - out_index);
            }
            else
            {
                memmove(aes256->cache, data_out + (output_len - out_index), out_once - (output_len - out_index));
                aes256->cache_size = out_once - (output_len - out_index);
                out_index += (output_len - out_index);
            }
            break;
        }

    }
    return out_index;
}

int32_t encrypt_aes_256_term(void *hd)
{
    encrypt_aes256_t *aes256 = (encrypt_aes256_t *)hd;
    if(hd == NULL)
    {
        return 0;
    }
    mbedtls_cipher_free(&aes256->ctx);
    os_mem_free(aes256);
    return 0;
}


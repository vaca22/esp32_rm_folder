
#ifndef __encry_aes256_dec_h__
#define __encry_aes256_dec_h__

#define ENCRYPT_MOD_ENC             0
#define ENCRYPT_MOD_DEC             1

#define ENCRYPT_AES_KEYBITS_128     0
#define ENCRYPT_AES_KEYBITS_192     1
#define ENCRYPT_AES_KEYBITS_256     2

typedef int32_t (*encrypt_read_data_cb)(void *user_data, uint8_t *buf, int32_t size);

/*
 * aes加密/解密初始化
 * user:
 *  iv
 * password:
 *  密钥
 * read_data:
 *  获取数据的回调
 * user_data:
 *  decode调用read_data的时候会把这个参数作为第一个参数传给read_data
 * retval:
 *  用来做aes256加密和解密的句柄
 */
void *encrypt_aes_256_init(int8_t *user, int8_t *password, int32_t mode, encrypt_read_data_cb read_data, void *user_data);

/*
 * 执行加密
 * input[in]:
 *  输入数据
 * input_len[in]:
 *  输入数据长度
 * output[in]:
 *  输出数据,解密后的数据存放在这里
 * output_len[in]:
 *  outbuf空间大小
 * retval:
 *  实际输出数据长度
 */
int32_t encrypt_aes_256_decode(void *hd, uint8_t *output, int32_t output_len);

/*
 * 逆初始化,释放init时申请的资源
 */
int32_t encrypt_aes_256_term(void *hd);

#endif


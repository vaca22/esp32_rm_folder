/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SPI peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "rm.h"
#include "mbedtls/md5.h"
#include "encrypt_aes256_dec.h"

static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

// Pin mapping
#if CONFIG_IDF_TARGET_ESP32

#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

#elif CONFIG_IDF_TARGET_ESP32S2

// adapted for internal test board ESP-32-S3-USB-OTG-Ev-BOARD_V1.0 (with ESP32-S2-MINI-1 module)
#define PIN_NUM_MISO 37
#define PIN_NUM_MOSI 35
#define PIN_NUM_CLK  36
#define PIN_NUM_CS   34

#elif CONFIG_IDF_TARGET_ESP32C3
#define PIN_NUM_MISO 6
#define PIN_NUM_MOSI 4
#define PIN_NUM_CLK  5
#define PIN_NUM_CS   1

#endif //CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2

#if CONFIG_IDF_TARGET_ESP32S2
#define SPI_DMA_CHAN    host.slot
#elif CONFIG_IDF_TARGET_ESP32C3
#define SPI_DMA_CHAN    SPI_DMA_CH_AUTO
#else
#define SPI_DMA_CHAN    1
#endif











#define MD5_MAX_LEN 16

void md5_sum(char * file_path,int8_t * md5){
    FILE *f = fopen(file_path, "rb");
    if(f==NULL){
        ESP_LOGE("sdcard","file not found");
        return;
    }
    char buf[64];
    mbedtls_md5_context ctx;
    unsigned char digest[MD5_MAX_LEN];

    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts_ret(&ctx);

    size_t read;

    do {
        read = fread((void*) buf, 1, sizeof(buf), f);
        if(read==0){
            break;
        }
        mbedtls_md5_update_ret(&ctx, (unsigned const char*) buf, read);
    } while(1);

    fclose(f);

    mbedtls_md5_finish_ret(&ctx, digest);


    char digest_str[MD5_MAX_LEN * 2+1];

    for (int i = 0; i < MD5_MAX_LEN; i++) {
        sprintf(&digest_str[i * 2], "%02x", (unsigned int)digest[i]);
    }
    digest_str[MD5_MAX_LEN * 2]='\0';

    ESP_LOGI("md5","%s",digest_str);
    if(md5){
        memcpy(md5,digest,16);
    }

}








#include "mbedtls/aes.h"
#include "mbedtls/cipher_internal.h"
#include "mbedtls/cipher.h"
#include "mbedtls/sha256.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/base64.h"

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


int32_t read_data_cb(void *user_data, uint8_t *buf, int32_t size){
    ESP_LOGE("gaga2","%d",size);
    int gaga=fread(buf,1,size,user_data);
    ESP_LOGE("gaga","%d",gaga);
    return gaga;
}
uint8_t  data_out[16];
void app_main(void) {
    esp_err_t ret;


    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");


    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
            .mosi_io_num = PIN_NUM_MOSI,
            .miso_io_num = PIN_NUM_MISO,
            .sclk_io_num = PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }


    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");


    sdmmc_card_print_info(stdout, card);

    mbedtls_aes_context *ctx;
    ctx = malloc(sizeof(mbedtls_aes_context));

    mbedtls_cipher_init(ctx);

    mbedtls_cipher_setup(ctx, &cipher_info_aes_256_cbc);
    mbedtls_cipher_setkey(ctx, (const unsigned char*)key, 256, mod);

    mbedtls_cipher_set_padding_mode(ctx, MBEDTLS_PADDING_PKCS7);
    mbedtls_cipher_reset(ctx);

    int8_t user[33];
    int8_t password[]="c36922f99f99453289ae5e6676d9791e";
    md5_sum("/sdcard/a.mp3",user);
//
//    FILE * fd_in=fopen("/sdcard/a.mp3","rb+");
//    FILE * fd_out=fopen("/sdcard/c.mp3","wb+");
//    void *hd=encrypt_aes_256_init(user,password,ENCRYPT_MOD_DEC,read_data_cb,fd_in);
//    while(1){
//
//        int32_t  ret_aes = encrypt_aes_256_decode(hd,data_out, 64);
//        if(ret_aes<=0){
//            ESP_LOGE("aaxx","error %d",ret_aes);
//            break;
//        }
//        int32_t ret_wb = fwrite(data_out,1,ret_aes,fd_out);
//        if(ret_aes!=ret_wb){
//            break;
//        }
//        ESP_LOGE("aaxx","%d",ret_wb);
//    }
//    encrypt_aes_256_term(hd);

    fclose(fd_out);











    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");

    //deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);
}

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







#define INPUT_LENGTH 16

mbedtls_aes_context aes;

// key length 32 bytes for 256 bit encrypting, it can be 16 or 24 bytes for 128 and 192 bits encrypting mode
unsigned char key[] = {0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

unsigned char input[INPUT_LENGTH] = {0};




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



    char fuck[]="c36922f99f99453289ae5e6676d9791e";
    for(int k=0;k<32;k++){
        key[k]=fuck[k];
    }


    memcpy(input,"123456",6);
    for(int k=6;k<16;k++){
        input[k]=10;
    }


    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 256);




    //82 D2 10 CA D3 E3 A9 09 A4 41 17 A8 53 67 DA F2

    unsigned char iv[] = {0x82, 0xD2 ,0x10 ,0xCA ,0xD3 ,0xE3 ,0xA9 ,0x09 ,0xA4 ,0x41 ,0x17 ,0xA8 ,0x53 ,0x67 ,0xDA ,0xF2};
    unsigned char iv1[] =  {0x82, 0xD2 ,0x10 ,0xCA ,0xD3 ,0xE3 ,0xA9 ,0x09 ,0xA4 ,0x41 ,0x17 ,0xA8 ,0x53 ,0x67 ,0xDA ,0xF2};
    unsigned char encrypt_output[INPUT_LENGTH] = {0};
    unsigned char decrypt_output[INPUT_LENGTH] = {0};
    ESP_LOG_BUFFER_HEX("cbcxx", iv, INPUT_LENGTH);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, INPUT_LENGTH, iv, input, encrypt_output);
    ESP_LOG_BUFFER_HEX("cbcxx", iv, INPUT_LENGTH);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, INPUT_LENGTH, iv1, encrypt_output, decrypt_output);
    ESP_LOG_BUFFER_HEX("cbc", encrypt_output, INPUT_LENGTH);
    ESP_LOG_BUFFER_HEX("cbc", decrypt_output, INPUT_LENGTH);
    ESP_LOGI("cbc", "%s", decrypt_output);












    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");

    //deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);
}

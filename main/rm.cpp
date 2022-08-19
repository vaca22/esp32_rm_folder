//
// Created by vaca on 8/19/22.
//

#include <sys/dirent.h>
#include <esp_log.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "rm.h"
#include "string"

using namespace std;


void rm(const char *path){

    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if(entry->d_type == DT_DIR){
            string s1=string (path);
            string s2=string ("/");
            string s3=string (entry->d_name);
            rm((s1+s2+s3).c_str());
        }else{
            string s1=string (path);
            string s2=string ("/");
            string s3=string (entry->d_name);
            unlink((s1+s2+s3).c_str());
        }
    }
    rmdir(path);
    closedir(dir);
}
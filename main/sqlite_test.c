//
// Created by vaca on 8/19/22.
//

#include <stdio.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_log.h>
#include "sqlite_test.h"



#include "sqlite3.h"

const char *data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    printf("callback: %s \n", (const char *)data);
    for (i = 0; i < argc; i++)
    {
        printf("callback: %s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int db_open(const char *filename, sqlite3 **db)
{
    int rc = sqlite3_open(filename, db);  //文件名不存在会自动创建
    if (rc)
    {
        printf("Can't open database: %s\n", sqlite3_errmsg(*db));
        return rc;
    }
    else
    {
        printf("Opened database successfully\n");
    }
    return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql)
{
    printf("%s\n", sql);
    int64_t start = esp_timer_get_time();
    int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        printf("Operation done successfully\n");
    }
    printf("Time taken: %lld\n", esp_timer_get_time() - start);
    return rc;
}

void example_sqlite3_test_fn(void)
{
    printf("\n\n\n########## start test for %s ##########\n", __func__);




    if(sqlite3_initialize()!=SQLITE_OK){
        printf("sqlite3 init error");
        return;
    }

    sqlite3 *db;
    int rc;
    if(db_open("/sdcard/plan.db", &db))
    {
        printf("Open plan.db fail \n");
        return;
    }
    int tick = xTaskGetTickCount();
//    rc = db_exec(db, "CREATE TABLE p_plan (id INTEGER, content);");
//    rc = db_exec(db, "INSERT INTO p_plan VALUES (1, 'Hello, World from test1');");
    rc = db_exec(db, "SELECT * FROM p_plan");


    sqlite3_close(db);



    printf("########## stop test for %s ##########\n", __func__);
}
/****************************************************
 > File Name:mysqlite3.h
 > Author: 
 > Mail: 
 > Create Time:2020年05月21日 星期四 10时11分18秒
***************************************************/

#ifndef __MYSQLITE3_
#define __MYSQLITE3_

cJSON *json_construction(char **resultp, int nrow, int ncloumn); //查询的数据转换成cJSON格式
cJSON *json_construction_reversed_order(char **resultp, int nrow, int ncloumn);//查询的数据,按从下往上的顺序排序,转换成cJSON格式
cJSON *nokey_json_construction(char **resultp, int nrow, int ncloumn); //查询的数据转换成不带key的cJSON格式
int select_info_sqlite3(char *db_name, const char *sql, char ***resultp, int *nrow, int *ncloumn);		//查询数据
int select_info_json_sqlite3(char *db_name, const char *sql, cJSON **JSON_Data);						       //查询数据，转换成JSON格式
int select_info_json_sqlite3_reversed_order(char *db_name, const char *sql, cJSON **JSON_Data);//查询数据,按从下往上的顺序排序，转换成JSON格式
int select_info_nokey_json_sqlite3(char *db_name, const char *sql, cJSON **JSON_Data);				//查询数据，转换成不带key的JSON格式
int change_info_sqlite3(char *db_name, const char *sql);//增加、删除和更改数据,清空整个表中的内容

#endif 

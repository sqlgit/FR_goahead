/********************************* Includes ***********************************/

#include 	"goahead.h"
#include	"cJSON.h"
#include 	"sqlite3.h"
#include	"mysqlite3.h"

/********************************* Defines ************************************/

/*********************************** Code *************************************/

cJSON *json_construction(char **resultp, int nrow, int ncloumn) 
{
	int i;
	int j;
	char info[1024] = {0};
	cJSON *array = NULL;
	cJSON *forceast = NULL;
		
	forceast = cJSON_CreateObject();
	for (i = 0; i < nrow; i++) {
		array = cJSON_CreateObject();
		for (j = 0; j < ncloumn; j++) {
			cJSON_AddStringToObject(array, resultp[j], resultp[(i + 1) * ncloumn + j]);
		}
		memset(info, 0, sizeof(info));
		strcpy(info, resultp[(i + 1) * ncloumn]);
		cJSON_AddItemToObject(forceast, info, array);
	}

	return forceast;
}

int select_info_sqlite3(char *db_name, const char *sql, char ***resultp, int *nrow, int *ncloumn)
{
	int i = 0, j = 0;
	int index = 0;
	char *errmsg = NULL;
	sqlite3 *db = NULL;

	if (sqlite3_open(db_name, &db) != SQLITE_OK) {
		fprintf(stderr, "Cannot open database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		return -1;
	}

	//查询数据库，nrow为行数（不包含表头），ncloumn为列数，resultp顺序放着表头和数据
	if (sqlite3_get_table(db, sql, resultp, nrow, ncloumn, &errmsg) != SQLITE_OK) {
		printf("%s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);

		return -1;
	}

	if (0 == (*nrow)) {
		printf("There is no data!\n");
		sqlite3_close(db);

		return -1; //没有此数据，返回
	}

	printf("nrow = %d,ncloumn = %d\n", (*nrow), (*ncloumn));
	for (i = 0; i < (*nrow) + 1; i++) {
		for (j = 0; j < (*ncloumn); j++) {
			printf("%s\t", (*resultp)[index++]);
		}
		printf("\n");
	}

	sqlite3_close(db);
	printf("Select_info_sqlite3_over \n");

	return 0;
}

int select_info_json_sqlite3(char *db_name, const char *sql, cJSON **JSON_Data)
{
	int nrow = 0;
	int ncloumn = 0;
	char **resultp = NULL;

	if (select_info_sqlite3(db_name, sql, &resultp, &nrow, &ncloumn) == -1) {
		return -1;
	}

	(*JSON_Data) = json_construction(resultp, nrow, ncloumn);

	sqlite3_free_table(resultp); //释放结果集
	printf("Select_info_JSON_sqlite3  over \n");

	return 0;
}

/**
  	sql：数据库要执行的语句
	db_nmae: 数据库名称，table：执行的表的名称, name：数据的名称
	返回值 -1:失败，0：成功
*/
//type：1.插入数据, 2.删除数据，3更改数据， name：数据的名称, table：执行的表的名称

int change_info_sqlite3(char *db_name, const char *table, const char *name, const char *sql)
{
	char *errmsg = NULL;
	sqlite3 *db = NULL;
	/*char *select[1024] = {0};
	char **resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	int index = 0;
	int i = 0;
	int j = 0;*/

	if (sqlite3_open(db_name, &db) != SQLITE_OK) {
		fprintf(stderr, "Cannot open database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		return -1;
	}
	printf("%s database open success!\n", db_name);

	//判读type的类型：1.插入数据, 2.删除数据，3更改数据
/*	if (2 == type || 3 == type) { //若为删除和更改，先查询是否有此数据
		sprintf(select, "select * from \'%s\' where name = \'%s\'", table, name);
		if (select_info_sqlite3(db_name, select, &resultp, &nrow, &ncloumn) == -1) {//查询没有此数据，返回-1

			return -1;
		}
		printf("---------------------------------\n");
		for (i = 0; i < nrow + 1; i++) {
			for (j = 0; j < ncloumn; j++) {
				printf("%s\t", resultp[index++]);
			}
			printf("\n");
		}
		sqlite3_free_table(resultp);
	}*/

	//执行sql中的语句
	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);

		return -1;
	}

	printf("database executed successfully\n");
	sqlite3_close(db);

	return 0;
}

//清空整个表的内容
int delete_all_info_sqlite3(char *db_name, const char *sql) {
	char *errmsg = NULL;
	sqlite3 *db = NULL;

	if (sqlite3_open(db_name, &db) != SQLITE_OK) {
		fprintf(stderr, "Cannot open database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		return -1;
	}
	printf("%s database open success!\n", db_name);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		printf("%s\n", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);

		return -1;
	}
	printf("database executed successfully\n");
	sqlite3_close(db);

	return 0;
}

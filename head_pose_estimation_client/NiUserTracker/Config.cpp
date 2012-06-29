#include <stdio.h>
#include "Config.h"

using namespace std;

Config::Config(char *fname)
{
	sprintf_s(cFile, sizeof(cFile), "%s", fname);
}

bool Config::IsExist(void)
{
	FILE *fp;
	int ret;
	if ((ret = fopen_s(&fp, cFile, "r")) != 0)
	{
        return false;
	}
    fclose(fp);
    return true;
}

bool Config::Initialize(void)
{
	int ret;
	FILE *fp;
	char buf[1048];

//	printf("Config file:[%s]\n", cFile);
	if((ret = fopen_s(&fp, cFile, "r")) != 0)
	{
		printf("File Open ERROR!! errno:%d fname:[%s]\n", ret, cFile);
		return false;
	}

	char* dp;
	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
		// 改行削除
		buf[strlen(buf)-1] = NULL;
//		printf("LINE:[%s]\n", buf);

		if(buf[0] == '#')		continue;
		if(strlen(buf) == 0)	continue;

		// 要素名とその値文字列取り出し
		char *next = strtok_s(buf, "=", &dp);
		string name(next);
		next = strtok_s(NULL, "=", &dp);
		string value(next);

//		printf("name:[%s] value:[%s]\n", name.c_str(), value.c_str());

		// 保存
		itemMap.insert(pair<string, string>(name, value));
	}

	fclose(fp);

	return true;
}


bool Config::GetValue(const char* name, int size, char* str)
{
	map<string, string>::iterator itr;

	itr = itemMap.find(name);

	if(itr == itemMap.end())
	{
		printf("GET ERROR!! name:[%s] value:[%s]\n", name, str);
		return false;
	}

	strcpy_s(str, size, itr->second.c_str());
//	printf("GET!! name:[%s] value:[%s]\n", name, str);

	return true;
}

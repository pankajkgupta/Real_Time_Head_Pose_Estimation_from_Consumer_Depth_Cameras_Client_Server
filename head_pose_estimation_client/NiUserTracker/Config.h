
#ifndef CONFIG_H_
#define CONFIG_H_

#include <iostream>
#include <string>
#include <map>

using namespace std;

class Config
{
public:
	Config(char *fname);
	bool IsExist();
	bool Initialize(void);
	bool GetValue(const char* name, int* value);
	bool GetValue(const char* name, int size, char* str);

private:
	char cFile[128];
	map<string, string> itemMap;
};

#endif
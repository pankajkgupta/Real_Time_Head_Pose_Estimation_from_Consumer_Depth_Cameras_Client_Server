
#ifndef SERVER_COM_H_
#define SERVER_COM_H_

#include <string>

#ifdef _MSC_VER
	#include <winsock2.h>
#else
	#include <sys/unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif

#include "Util.h"

class ServerCom
{
public:
	// コンストラクタ
	ServerCom(void);
	// デストラクタ
	~ServerCom(void);
	// 初期設定
	int InitServer(char* host,short port);
	// データ送信
	int SendInfo(const char *time_stamp, const int valsize, const float *value);
	// データ送信
	int SendData(const int datasize, const char *data);

private:
	// サーバ情報
	char	host[16];
	short	port;
	SOCKET	csock;

	Util	util;

	// サーバ接続
	int  InitServer(void);
	void EndServer(void);
};

#endif
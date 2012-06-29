#include <stdio.h>
#include <time.h>
#include <string.h>

#include "Common.h"
#include "Util.h"
#include "Config.h"
#include "ServerCom.h"

// コンストラクタ
ServerCom::ServerCom(void)
{
	host[0] = NULL;
	port = -1;
	csock = NULL;
}

// デストラクタ
ServerCom::~ServerCom(void)
{
	EndServer();
}

int ServerCom::InitServer(char* host, short port)
{
	struct sockaddr_in dest;

	sprintf_s(ServerCom::host, sizeof(ServerCom::host), "%s", host);
	ServerCom::port = port;

#ifdef _MSC_VER
	//ソケット通信の準備
	WSADATA data;
	WSAStartup(MAKEWORD(2,0), &data);
#endif
	
	//接続先（サーバ）のアドレス情報を設定
	memset(&dest, 0, sizeof(dest));

	//ポート番号はサーバプログラムと共通
	dest.sin_port = htons(port);
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr(host);

	printf("Connect to %s:%d\n", host, port);

	//ソケットの生成
	csock = socket(AF_INET, SOCK_STREAM, 0);

	//サーバへの接続
	if( connect(csock, (struct sockaddr *) &dest, sizeof(dest)) ) {
		printf("%s:%d に接続できませんでした。\n", host, port);
		csock = NULL;
		return -1;
	}

	return 0;
}

void ServerCom::EndServer(void)
{
	if(csock != NULL) {
#ifdef _MSC_VER
		closesocket(csock);
#else
		close(csock);
#endif
	}
}

// スケルトン情報送信
int ServerCom::SendInfo(const char *time_stamp, const int valsize, const float *value)
{
	char *dp;
	int dsize;
	int timesize;
	int ret = -1;

	timesize = (int)strlen(time_stamp) + 1;
	dsize = valsize + timesize;
	if( (dp = (char *)malloc(dsize)) == NULL ) {
		perror("malloc Failed!!:");
		return ret;
	}

	// 送信データ設定
	memcpy(dp, value, valsize);
	// 最後にタイムスタンプ設定
	memcpy((dp+valsize), time_stamp, timesize);

	// 送信
	if(SendData(dsize, (char *)dp) == 0) {
		ret = 0;
	}
	else {
		printf("SendData ERROR!!\n");
	}

	free(dp);

	return ret;
}

int ServerCom::SendData(const int datasize, const char *data)
{
	if (csock == NULL) {
		printf("サーバ未接続 [%s:%d] datasize:%d\n", host, port, datasize);
		return -1;
	}

	// 送信
	int ssize;
	if((ssize = send(csock, (char *)data, datasize, 0)) != datasize) {
		printf("送信サイズ異常 sendSize:%d sentSize:%d\n", datasize, ssize);
		return -1;
	}

	return 0;
}
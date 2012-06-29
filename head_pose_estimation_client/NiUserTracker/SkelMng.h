#ifndef SKEL_MNG_H_
#define SKEL_MNG_H_

#include "Common.h"
#include "ServerCom.h"

class SkelMng
{
public:
	SkelMng(void);

	void Initialize();

	void End(void);

	int ConnectSkelServer(void);

	void Update(const char* time_stamp, const float* values);

	float GetKinectId(void)  { return initValues[0]; }

protected:
	// 初回送信データ
	float	initValues[7];
	struct SingleData
	{
		char  time_stamp[TIME_STR_LEN];
		float values[SRV_SEND_INFO_NUM];
	};
	XnBool m_bSkelServerConnected;
	SingleData* m_pData;
	XnUInt32 m_nNextWrite;
	XnUInt32 m_nExec;
	XnUInt32 m_nBufferSize;
	XnUInt32 m_nBufferSizeExec;
	XnUInt32 m_nDataCount;
	static volatile XnBool m_bEnd;
	XnChar m_skelHost[16];
	XnUInt16 m_skelPort;

	ServerCom m_ServerCom;
	char m_strDirName[128];
	int m_nFileLength;
	bool m_bSend;
	bool m_bWrite;
#ifdef _MSC_VER
	HANDLE m_ManagerTh;
	unsigned m_ManagerId;
	CRITICAL_SECTION csManager;
#endif

private:
	static unsigned __stdcall SkelMng::Manager(void *p);
	void Sender(void);
	void FileWriter(void);
	Util g_util;
	FILE *fp;
};

#endif
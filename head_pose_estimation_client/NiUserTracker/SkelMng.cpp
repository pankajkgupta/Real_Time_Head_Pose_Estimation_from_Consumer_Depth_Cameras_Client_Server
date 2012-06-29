#include <process.h>
#include <time.h>

#include "SkelMng.h"
#include "Config.h"

#define LOG_WRITE_ENABLE
#ifdef LOG_WRITE_ENABLE
	#define LOG_NAME	"log_SkelMng.txt"
	FILE *skel_fp = NULL;
#endif

#define DATA_BUF_LENGTH		10		// スケルトン蓄積バッファ長(xx秒分)

volatile XnBool SkelMng::m_bEnd = FALSE;

SkelMng::SkelMng(void)
{
	m_bSend = FALSE;
	m_bWrite = FALSE;
	m_nNextWrite = 0;
	m_nExec = m_nNextWrite;
	m_nBufferSize = 0;
	m_nBufferSizeExec = 0;
	m_nDataCount = 0;
	m_ManagerTh = NULL;
	m_ManagerTh = NULL;
	fp = NULL;
	memset(initValues, 0x00, sizeof(initValues));
}

void SkelMng::Initialize(void)
{
#ifdef LOG_WRITE_ENABLE
	fopen_s(&skel_fp, LOG_NAME, "a+");
#endif
	// コンフィグファイル読み込み
	char tmp[16];
	Config config(CONFIG_FILE);
	config.Initialize();

	config.GetValue("KINECT_ID", sizeof(tmp), tmp);
	initValues[0] = (float)atof(tmp);

	config.GetValue("POSITION_X", sizeof(tmp), tmp);
	initValues[1] = (float)atof(tmp);

	config.GetValue("POSITION_Y", sizeof(tmp), tmp);
	initValues[2] = (float)atof(tmp);

	config.GetValue("POSITION_Z", sizeof(tmp), tmp);
	initValues[3] = (float)atof(tmp);

	config.GetValue("ANGLE_X", sizeof(tmp), tmp);
	initValues[4] = (float)atof(tmp);

	config.GetValue("ANGLE_Y", sizeof(tmp), tmp);
	initValues[5] = (float)atof(tmp);

	config.GetValue("ANGLE_Z", sizeof(tmp), tmp);
	initValues[6] = (float)atof(tmp);

	config.GetValue("SKEL_HOST", sizeof(m_skelHost), m_skelHost);
	config.GetValue("SKEL_PORT", sizeof(tmp), tmp);
	m_skelPort = (short)atoi(tmp);

	config.GetValue("DATA_DIR", sizeof(m_strDirName), m_strDirName);

	config.GetValue("SKEL_SEND", sizeof(tmp), tmp);
	if (strcmp(tmp, "TRUE") == 0)
	{
		m_bSend = TRUE;
	}

	config.GetValue("SKEL_WRITE", sizeof(tmp), tmp);
	if (strcmp(tmp, "TRUE") == 0)
	{
		m_bWrite = TRUE;
	}

	config.GetValue("SKEL_FILE_DIR", sizeof(tmp), tmp);
	strcat_s(m_strDirName, sizeof(m_strDirName), tmp);

	config.GetValue("SKEL_LENGTH", sizeof(tmp), tmp);
	m_nFileLength = atoi(tmp);

	// バッファ生成 バッファサイズ：10秒分(30fpsと仮定)
	m_nBufferSize = m_nBufferSizeExec = DATA_BUF_LENGTH * 30;
	m_pData = new SingleData[m_nBufferSize];

	// スレッド生成
	if ((m_bSend == TRUE) || (m_bWrite == TRUE))
	{
		ConnectSkelServer();
		InitializeCriticalSection(&csManager);
		m_ManagerTh = (HANDLE)_beginthreadex( NULL, 0, &Manager, this, 0, &m_ManagerId );
	}
}

void SkelMng::End(void)
{
	m_bEnd = TRUE;

	WaitForSingleObject(m_ManagerTh, 30 * 1000);

	if (fp != NULL)
	{
		fclose(fp);
	}

	delete [] m_pData;

#ifdef LOG_WRITE_ENABLE
	fclose(skel_fp);
#endif
}

// スケルトン情報受信サーバ接続
int SkelMng::ConnectSkelServer(void)
{
	// サーバ接続
	if( m_ServerCom.InitServer(m_skelHost, m_skelPort) == 0 )
	{
		m_bSkelServerConnected = TRUE;
	}
	else
	{
		m_bSkelServerConnected = FALSE;
		return -1;
	}

	char time_stamp[TIME_STR_LEN];
	g_util.GetTimeStr(sizeof(time_stamp), time_stamp);

	printf("TIME STAMP:[%s]\n", time_stamp);
	printf("VALUES;");
	for(int i = 0; i < 7; i++)
	{
		printf("%f ", initValues[i]);
	}
	printf("\n");

	m_ServerCom.SendInfo(time_stamp, (int)sizeof(initValues), initValues);

	return 0;
}

void SkelMng::Update(const char* time_stamp, const float* values)
{
	if ((m_bSend == FALSE) && (m_bWrite == FALSE))
	{
		return;
	}

	// バッファ空き確認
	if(m_nBufferSizeExec == 0)
	{
		printf("バッファ空きなし！！\n");
		return;
	}

	SingleData* pData = m_pData + m_nExec;

	sprintf_s(pData->time_stamp, sizeof(pData->time_stamp), "%s", time_stamp);
	memcpy((void*)pData->values, (const void*)values, sizeof(pData->values));
#ifdef LOG_WRITE_ENABLE
	fprintf(skel_fp, "[%s] time:[%s]\n", __FUNCTION__, pData->time_stamp);
#endif
	// バッファ管理情報書き換え
	EnterCriticalSection(&csManager);
	m_nDataCount++;
	m_nBufferSizeExec--;
	m_nNextWrite++;
	if (m_nBufferSize <= m_nNextWrite)
	{
		m_nNextWrite = 0;
	}
	LeaveCriticalSection(&csManager);
}

unsigned __stdcall SkelMng::Manager(void *p)
{
	printf("Skel Manager START!!\n");

	SkelMng* smngp = reinterpret_cast<SkelMng*>(p);

	while(!(m_bEnd && (smngp->m_nDataCount == 0)))
	{
		XnUInt32 execNum = smngp->m_nDataCount;

		if(execNum == 0)
		{
#ifdef _MSC_VER
			Sleep(30);
#else
#endif
			continue;
		}

#ifdef LOG_WRITE_ENABLE
		fprintf(skel_fp, "[%s]EXEC NUM:%d\n", __FUNCTION__, execNum);
#endif
		while(execNum)
		{
			smngp->Sender();
			smngp->FileWriter();
			execNum--;

			smngp->m_nExec++;
			if(smngp->m_nBufferSize <= smngp->m_nExec)
			{
				smngp->m_nExec = 0;
			}

			EnterCriticalSection(&smngp->csManager);
			smngp->m_nDataCount--;
			smngp->m_nBufferSizeExec++;
			LeaveCriticalSection(&smngp->csManager);
		}
	}
	printf("Skel Manager END...\n");
	_endthreadex( 0 );
    return 0;
}

void SkelMng::Sender(void)
{
	SingleData* pData = m_pData + m_nExec;
	if ((m_bSend == FALSE) || (m_bSkelServerConnected == FALSE))
	{
#ifdef LOG_WRITE_ENABLE
	fprintf(skel_fp, "[%s] Not send..  m_bSend:%d m_bSkelServerConnected:%d time:[%s]\n", 
		__FUNCTION__, m_bSend, m_bSkelServerConnected, pData->time_stamp);
#endif
		return;
	}

#ifdef LOG_WRITE_ENABLE
	fprintf(skel_fp, "[%s] time:[%s]\n", __FUNCTION__, pData->time_stamp);
#endif
	m_ServerCom.SendInfo(pData->time_stamp, sizeof(pData->values), pData->values);
}

void SkelMng::FileWriter(void)
{
	if (m_bWrite == FALSE)
	{
		return;
	}

	SingleData* pData = m_pData + m_nExec;

	static time_t mk_tm = time(NULL);
//	printf("SkelMng FileWriter\n");

	// 前回ファイルオープンから1分以上経過していれば閉じる
	time_t now_tm = time(NULL);
	if(now_tm - mk_tm > (m_nFileLength*60))
	{
		fclose(fp);
		fp = NULL;
	}

	if (fp == NULL)
	{
		mk_tm = time(NULL);

		char prefix[] = "skeleton_";\
		char extention[] = ".txt";
		char fname[256];
		memset(fname, NULL, sizeof(fname));

		// ファイル名を、今回のデータのタイムスタンプから生成
		sprintf_s(fname, sizeof(fname), "%s/%s%s", m_strDirName, prefix, pData->time_stamp);
		Util::ChangeIntoNum(&fname[strlen(m_strDirName)+strlen(prefix)+1]);

		strcat_s(fname, sizeof(fname), extention);

		int ret;
		if((ret = fopen_s(&fp, fname, "w")) != 0)
		{
			printf("File Open ERROR!! errno:%d fname:[%s]\n", ret, fname);
			fp = NULL;
			return;
		}
	}
#ifdef LOG_WRITE_ENABLE
	fprintf(skel_fp, "[%s] time:[%s]\n", __FUNCTION__, pData->time_stamp);
#endif
	float* dp = pData->values;
	for(int i = 0; i < SRV_SEND_INFO_NUM; i++)
	{
		fprintf(fp, "%f", *dp);
		dp++;
		fprintf(fp, ",");
	}
	fprintf(fp, "%s\n", pData->time_stamp);
}
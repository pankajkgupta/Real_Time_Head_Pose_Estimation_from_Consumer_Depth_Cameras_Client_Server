#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#include "Util.h"

Util::Util(void)
{
	memset(&startTime, 0, sizeof(startTime));
	prePassedMsec = 0;
}

struct stTimeInfo Util::GetCurTime(void)
{
	SYSTEMTIME systime;
	GetLocalTime(&systime);

	struct stTimeInfo timeInfo;
	timeInfo.year = systime.wYear;
	timeInfo.month = systime.wMonth;
	timeInfo.day = systime.wDay;

	timeInfo.hour = systime.wHour;
	timeInfo.min = systime.wMinute;
	timeInfo.sec = systime.wSecond;
	timeInfo.msec = systime.wMilliseconds;

	struct tm t;
	t.tm_year = timeInfo.year - 1900;
	t.tm_mon  = timeInfo.month - 1;
	t.tm_mday = timeInfo.day;
	t.tm_wday = systime.wDayOfWeek;
	t.tm_hour = timeInfo.hour;
	t.tm_min  = timeInfo.min;
	t.tm_sec  = timeInfo.sec;
	t.tm_isdst= -1;      /* 夏時間無効 */

	// 通算秒保存
	timeInfo.cTime = mktime(&t);

	printf("CUR TIME:%04d/%02d/%02d %02d:%02d:%02d.%03d\n", 
		timeInfo.year, timeInfo.month, timeInfo.day, 
		timeInfo.hour, timeInfo.min, timeInfo.sec, timeInfo.msec);

	return timeInfo;
}

void Util::GetTimeStr(const size_t buf_size, char *time_stamp)
{
	struct stTimeInfo timeInfo;
	timeInfo = GetCurTime();

	sprintf_s(time_stamp, buf_size, "%4d/%02d/%02d %02d:%02d:%02d.%03d",
		timeInfo.year, timeInfo.month, timeInfo.day,
		timeInfo.hour, timeInfo.min, timeInfo.sec,
		timeInfo.msec);

	printf("NOW TIME:[%s]\n", time_stamp);
}

void SetTimeInfo(struct stTimeInfo *timeInfo)
{
	struct tm get_tm;
	localtime_s(&get_tm, &(timeInfo->cTime));

	timeInfo->year = get_tm.tm_year + 1900;
	timeInfo->month = get_tm.tm_mon + 1;
	timeInfo->day = get_tm.tm_mday;

	timeInfo->hour = get_tm.tm_hour;
	timeInfo->min = get_tm.tm_min;
	timeInfo->sec = get_tm.tm_sec;

	//printf("TIME INFO. cTime:%ld %04d/%02d/%02d %02d:%02d:%02d.%03d\n",
	//	timeInfo->cTime, timeInfo->year, timeInfo->month, timeInfo->day, 
	//	timeInfo->hour, timeInfo->min, timeInfo->sec, timeInfo->msec);
}

void PlusTime(const XnUInt64 passedMsec, struct stTimeInfo *timeInfo)
{
	// マイクロ秒をミリ秒に変換
	XnUInt64 tMsec = passedMsec / 1000;

	int msec;
	time_t sec;
	msec = tMsec % 1000;
	sec = tMsec / 1000;

	timeInfo->msec += msec;
	if(timeInfo->msec >= 1000)
	{
		timeInfo->msec = timeInfo->msec - 1000;
		sec += 1;
	}

	timeInfo->cTime += sec;
	SetTimeInfo(timeInfo);
}

void MinusTime(const XnUInt64 passedMsec, struct stTimeInfo *timeInfo)
{
	// マイクロ秒をミリ秒に変換
	XnUInt64 tMsec = passedMsec / 1000;

	int msec, sec;
	msec = tMsec % 1000;
	sec = (int)(tMsec / 1000);

	timeInfo->msec -= msec;
	if(timeInfo->msec < 0)
	{
		timeInfo->msec = 1000 - (-timeInfo->msec);
		sec += 1;
	}

	timeInfo->cTime -= sec;
	SetTimeInfo(timeInfo);
}

void Util::GetTimeStr(const XnUInt64 passedMsec, const size_t buf_size, char *time_stamp)
{
	struct stTimeInfo timeInfo;
	if((startTime.year == 0) || (passedMsec < prePassedMsec))
	{
		startTime = GetCurTime();
		
		// スタート時間を経過時間分戻す
		MinusTime(passedMsec, &startTime);
	}
	// 経過時間を足す
	timeInfo = startTime;
	PlusTime(passedMsec, &timeInfo);

	prePassedMsec = passedMsec;

	sprintf_s(time_stamp, buf_size, "%4d/%02d/%02d %02d:%02d:%02d.%03d",
		timeInfo.year, timeInfo.month, timeInfo.day,
		timeInfo.hour, timeInfo.min, timeInfo.sec,
		timeInfo.msec);

	//printf("GET TIME:[%s]\n", time_stamp);
}

void Util::ChangeIntoNum(char *time_str)
{
	int ssize = (int)strlen(time_str);
	char *new_str = new char[ssize];
	memset(new_str, NULL, sizeof(new_str));

	char* wp = new_str;
	for(int i = 0; i < strlen(time_str); i++)
	{
		if(!isdigit(time_str[i]))  continue;
		*wp = time_str[i];
		wp++;
	}
	*wp = NULL;
//	printf("new:[%s]\n", new_str);
	sprintf_s(time_str, ssize, "%s", new_str);
	delete [] new_str;
}

// エポックタイム(マイクロ秒)の差分をミリ秒で返す
XnUInt64 Util:: GetTimeDiff(const XnUInt64& nNow, const XnUInt64& nLast)
{
	XnUInt64 retVal;
	if(nLast <= nNow)
	{
		retVal = (nNow - nLast) / 1000;
	}
	else
	{
		retVal = (~nLast + nNow) / 1000;
	}

	return retVal;
}

int Util::CheckMissedFrame(const std::string msg, XnUInt64& nTimestamp, XnUInt32* pFrames, XnUInt64* pLastTime, XnUInt32* pMissedFrames)
{
	int ret = 0;

	++(*pFrames);
//		printf("Timestamp(User):%d\n", nTimestamp);
//	if ((*pLastTime != 0) && ((nTimestamp - *pLastTime) > 35000))
	if ((*pLastTime != 0) && (GetTimeDiff(nTimestamp, *pLastTime) > 35))
	{
		int missed = (int)(nTimestamp - *pLastTime) / 32000 - 1;
		printf("Missed %s: %llu -> %llu = %d > 35000 - %d frames\n",
			msg.c_str(), *pLastTime, nTimestamp, XnUInt32(nTimestamp - *pLastTime), missed);
		*pMissedFrames += missed;

		ret = missed;
	}
	*pLastTime = nTimestamp;

	return ret;
}

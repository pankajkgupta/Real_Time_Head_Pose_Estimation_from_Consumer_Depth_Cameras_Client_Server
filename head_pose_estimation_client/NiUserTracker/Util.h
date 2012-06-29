
#ifndef UTIL_H_
#define UTIL_H_

#include <XnPlatform.h>
#include <string>

struct stTimeInfo {
	time_t cTime;
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
	int msec;
};

class Util
{
public:
	Util(void);
	struct stTimeInfo GetCurTime(void);
	void GetTimeStr(const size_t buf_size, char *time_stamp);
	void GetTimeStr(const XnUInt64 passedMsec, const size_t buf_size, char *time_stamp);
	static void ChangeIntoNum(char *time_str);
	static XnUInt64 GetTimeDiff(const XnUInt64& nNow, const XnUInt64& nLast);
	static int CheckMissedFrame(const std::string msg, XnUInt64& nTimestamp, XnUInt32* pFrames, XnUInt64* pLastTime, XnUInt32* pMissedFrames);

private:
	struct stTimeInfo startTime;
	XnUInt64 prePassedMsec;
};

#endif
#ifndef COMMON_H_
#define COMMON_H_

// コンフィグファイル名
#define CONFIG_FILE		"Config.txt"
#ifdef _MSC_VER
	#define DIR_DIV			"\\"
#else
	#define DIR_DIV			"/"
#endif

// 骨格情報関連
#define SKEL_NUM			20		// 骨格情報数
#define SRV_SEND_HDR_NUM	5		// ヘッダ付加情報
#define SRV_SEND_INFO_NUM	(SRV_SEND_HDR_NUM+(SKEL_NUM*3))	// 全体数

#define TIME_STR_LEN		24		//	日時文字列長(YYYY/MM/DD hh:mm:ss.fff)

#define REALTIME_FPS_CHECK_ENABLE

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------
#define CHECK_RC(rc, what)											\
	if (rc != XN_STATUS_OK)											\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(rc));		\
		return rc;													\
	}

#define CHECK_RC_ERR(rc, what, errors)			\
{												\
	if (rc == XN_STATUS_NO_NODE_PRESENT)		\
	{											\
		XnChar strError[1024];					\
		errors.ToString(strError, 1024);		\
		printf("%s\n", strError);				\
	}											\
	CHECK_RC(rc, what)							\
}

#endif
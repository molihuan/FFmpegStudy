// HiFFmpeg.cpp: 定义应用程序的入口点。
//

#include "HiFFmpeg.h"

using namespace std;

extern "C" {
// 引入 FFmpeg 头文件
#include <libavcodec/avcodec.h>
}

int main()
{
	cout << "Hello CMake." << endl;

	// 输出 avcodec 配置信息
	printf("%s", avcodec_configuration());

	return 0;
}

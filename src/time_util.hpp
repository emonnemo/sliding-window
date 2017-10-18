#ifndef TIME_UTIL
#define TIME_UTIL

#include <string.h>
using namespace std;

string get_header(string ip) {
	time_t rawtime;
	struct tm* timeinfo;

	char buffer[80];
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%d/%m/%Y:%I:%M:%S", timeinfo);

	return ip + " - - [" + string(buffer) + "]";

	// return ip + " - - [" + (string)timeinfo->tm_mday + "/" + (string)timeinfo->tm_mon + "/"
	// 	+ (string)(timeinfo->tm_year + 1900) + ":" + (string)timeinfo->tm_hour + ":"
	// 	+ (string)timeinfo->tm_min + ":" + (string)timeinfo->tm_sec + "] ";
}

#endif
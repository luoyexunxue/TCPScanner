#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif
#include <cstring>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <list>
#include <queue>
using namespace std;

mutex ProcessMutex;
mutex ResultMutex;
queue<sockaddr_in*> ProcessList;
list<sockaddr_in*> ResultList;

const char* getDefinedPortName(int port) {
	if (port == 21) return "- FTP";
	if (port == 22) return "- SSH";
	if (port == 23) return "- Telnet";
	if (port == 25) return "- SMTP";
	if (port == 43) return "- WHOIS";
	if (port == 53) return "- DNS";
	if (port == 69) return "- TFTP";
	if (port == 80) return "- HTTP";
	if (port == 110) return "- POP3";
	if (port == 135) return "- RPC";
	if (port == 143) return "- IMAP4";
	if (port == 443) return "- HTTPS";
	if (port == 444) return "- SNPP";
	if (port == 445) return "- RPC";
	if (port == 1433) return "- SQL Server";
	if (port == 1521) return "- Oracle";
	if (port == 1723) return "- PPTP";
	if (port == 3306) return "- MYSQL Server";
	if (port == 3389) return "- Microsoft RDP";
	if (port == 4000) return "- QQ";
	if (port == 4300) return "- QQ";
	if (port == 8080) return "- Tomcat";
	if (port == 10000) return "- BaiduYun";
	return "";
}

void printUsage(char* exe) {
	cout << "Usage: ";
	cout << exe << " -ip ips -port ports [-thread num]" << endl;
	cout << "  " << "-ip     ip address to be scanned, split by comma, can be ranged." << endl;
	cout << "  " << "-port   port to be scanned, split by comma, can be ranged, default is 1-65535" << endl;
	cout << "  " << "-thread thread pool size, default is 100." << endl;
}

void parseIP(const char* ips, vector<in_addr>& ipList) {
	int count = strlen(ips);
	int begin = 0;
	int end = 0;
	while (end < count) {
		while (ips[end] != ',' && ips[end] != '-' && ips[end] != '\0') end += 1;
		if (ips[end] == '-') {
			in_addr istart;
			in_addr istop;
			string start_ip(ips + begin, end - begin);
			begin = ++end;
			while (ips[end] != ',' && ips[end] != '-' && ips[end] != '\0') end += 1;
			string stop_ip(ips + begin, end - begin);
			inet_pton(AF_INET, start_ip.c_str(), &istart);
			inet_pton(AF_INET, stop_ip.c_str(), &istop);
			u_long a = htonl(istart.s_addr);
			u_long b = htonl(istop.s_addr);
			while (a <= b) {
				in_addr addr;
				addr.s_addr = ntohl(a++);
				ipList.push_back(addr);
			}
		}
		else {
			in_addr iip;
			string ip(ips + begin, end - begin);
			inet_pton(AF_INET, ip.c_str(), &iip);
			ipList.push_back(iip);
		}
	}
}

void parsePort(const char* ports, vector<int>& portList) {
	int count = strlen(ports);
	int begin = 0;
	int end = 0;
	while (end < count) {
		while (ports[end] != ',' && ports[end] != '-' && ports[end] != '\0') end += 1;
		if (ports[end] == '-') {
			string start_port(ports + begin, end - begin);
			begin = ++end;
			while (ports[end] != ',' && ports[end] != '-' && ports[end] != '\0') end += 1;
			string stop_port(ports + begin, end - begin);
			int istart = atoi(start_port.c_str());
			int istop = atoi(stop_port.c_str());
			while (istart <= istop) {
				portList.push_back(istart++);
			}
		}
		else {
			portList.push_back(atoi(string(ports + begin, end - begin).c_str()));
		}
	}
}

void testHost() {
	while (true) {
		sockaddr_in* addr = nullptr;
		ProcessMutex.lock();
		if (!ProcessList.empty()) {
			addr = ProcessList.front();
			ProcessList.pop();
		}
		ProcessMutex.unlock();
		if (addr != nullptr) {
#ifdef _WIN32
			SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
			if (connect(sock, (sockaddr*)addr, sizeof(sockaddr_in)) == S_OK) {
				ResultMutex.lock();
				ResultList.push_back(addr);
				ResultMutex.unlock();
			}
			else delete addr;
			closesocket(sock);
#else
			int sock = socket(AF_INET, SOCK_STREAM, 0);
			if (connect(sock, (sockaddr*)addr, sizeof(sockaddr_in)) == 0) {
				ResultMutex.lock();
				ResultList.push_back(addr);
				ResultMutex.unlock();
			}
			else delete addr;
			shutdown(sock, SHUT_RDWR);
#endif
		}
		else break;
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printUsage(argv[0]);
		return -1;
	}
	int threadCount = 100;
	vector<in_addr> ipList;
	vector<int> portList;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-ip")) parseIP(argv[++i], ipList);
		if (!strcmp(argv[i], "-port")) parsePort(argv[++i], portList);
		if (!strcmp(argv[i], "-thread")) threadCount = atoi(argv[++i]);
	}
	if (threadCount > 1000) threadCount = 1000;
	if (portList.size() == 0) {
		for (int i = 1; i <= 65535; i++) portList.push_back(i);
	}
	for (size_t x = 0; x < ipList.size(); x++) {
		for (size_t y = 0; y < portList.size(); y++) {
			sockaddr_in* addr = new sockaddr_in();
			addr->sin_family = AF_INET;
			addr->sin_port = htons(portList[y]);
			addr->sin_addr = ipList[x];
			ProcessList.push(addr);
		}
	}
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	vector<thread*> threadPool;
	threadPool.resize(threadCount);
	for (int i = 0; i < threadCount; i++) threadPool[i] = new thread(testHost);
	for (int i = 0; i < threadCount; i++) threadPool[i]->join();
	for (int i = 0; i < threadCount; i++) delete threadPool[i];
#ifdef _WIN32
	WSACleanup();
#endif
	// print result
	cout << "Open address:" << endl;
	list<sockaddr_in*>::iterator iter = ResultList.begin();
	while (iter != ResultList.end()) {
		sockaddr_in* addr = *iter;
		char ip[16];
		inet_ntop(AF_INET, &addr->sin_addr, ip, 16);
		int port = ntohs(addr->sin_port);
		cout << "\t" << ip << " : " << port << " " << getDefinedPortName(port) << endl;
		delete *iter;
		++iter;
	}
	return 0;
}
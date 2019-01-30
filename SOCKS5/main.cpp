#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "BitStream.h"


int dblSpace;
char buffer[131072];

TCHAR GetChar(TCHAR _char) {
	if (_char < 32)
		return '.';

	return _char;
}

char *DumpMem(unsigned char *pAddr, int len) {
	memset(buffer, 0, 16384);

	char temp[256];
	BYTE fChar;
	int i = 0;
	long bytesRead = 0;
	TCHAR line[17];
	int nSpaces = 3 * 16 + 2;
	memset(&line, 0, sizeof(line));
	int dataidx = 0;

	if (dblSpace)
		strcat(buffer, "\n\n");

	for (;;) {
		// print hex address
		sprintf(temp, "%08X  ", i);
		strcat(buffer, temp);

		// print first 8 bytes
		for (int j = 0; j < 0x08; j++) {
			fChar = pAddr[dataidx];
			if (dataidx >= len) break;
			dataidx++;

			sprintf(temp, "%02X ", fChar);
			strcat(buffer, temp);

			// add to the ASCII text
			line[bytesRead++] = GetChar(fChar);

			// this took three characters
			nSpaces -= 3;
		}

		// print last 8 bytes - change in the "xx " to " xx" provides
		// the double space in between the first 8 and the last 8 bytes.
		for (int j = 0x08; j < 0x10; j++) {
			fChar = pAddr[dataidx];
			if (dataidx >= len) break;
			dataidx++;

			sprintf(temp, " %02X", (unsigned char)fChar);
			strcat(buffer, temp);

			// add to the ASCII text
			line[bytesRead++] = GetChar(fChar);

			// this took three characters
			nSpaces -= 3;
		}

		// fill in any leftover spaces.
		for (int j = 0; j <= nSpaces; j++) {
			strcat(buffer, " ");
		}

		// print ASCII text
		sprintf(temp, "%s", line);
		strcat(buffer, temp);

		// quit if the file is done
		if (dataidx >= len) break;

		// new line
		strcat(buffer, "\n");

		if (dblSpace)
			strcat(buffer, "\n");

		// reset everything
		bytesRead = 0;
		memset(&line, 0, sizeof(line));
		i += 16;
		//dataidx++;
		nSpaces = 3 * 16 + 2;
	}

	return buffer;
}

#define SAMP_IP "217.106.106.116"
#define SAMP_PORT 7762

int main() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr("99.194.112.26");
	sockAddr.sin_port = htons(64558);

	connect(hSocket, (SOCKADDR *)&sockAddr, sizeof(sockAddr));

	BitStream bsSend;
	bsSend.Write((BYTE)0x05); // VERSION X'05'
	bsSend.Write((BYTE)0x01); // йнк-бн лернднб X'01'
	bsSend.Write((BYTE)0x00); // лернд: аег юбрнпхгюжхх X'00'

	send(hSocket, (const char *)bsSend.GetData(), bsSend.GetNumberOfBytesUsed(), NULL);
	printf("< [SEND] len: %d\n%s\n\n", bsSend.GetNumberOfBytesUsed(), DumpMem(bsSend.GetData(), bsSend.GetNumberOfBytesUsed()));
	bsSend.Reset();

	unsigned char byteRecv[4096];
	int iRespLen;

	ZeroMemory(byteRecv, sizeof(byteRecv));
	iRespLen = recv(hSocket, (char *)byteRecv, sizeof(byteRecv), NULL);

	// нрбер
	// ==============
	// бепяхъ X'05'
	// сяоеьмн X'00'
	// ==============

	if (iRespLen < 2) {
		printf("Proxy not active");
		closesocket(hSocket);
		getchar();
		return 0;
	}

	printf("> [RECV] len: %d\n%s\n\n", iRespLen, DumpMem(byteRecv, iRespLen));

	bsSend.Write((BYTE)0x05); // бепяхъ X'05'
	bsSend.Write((BYTE)0x03); // лернд: юяянжхюжхъ UDP онпрю X'03'
	bsSend.Write((BYTE)0x00); // гюпегепбхпнбюммши аюир X'00'
	bsSend.Write((BYTE)0x01); // рхо юдпеяю: IPv4 X'01'
	bsSend.Write((DWORD)inet_addr(SAMP_IP)); // юдпея жекебнцн яепбепю - 4 аюирю
	bsSend.Write((WORD)htons(SAMP_PORT)); // онпр жекебнцн яепбепю - 2 аюирю

	send(hSocket, (const char *)bsSend.GetData(), bsSend.GetNumberOfBytesUsed(), NULL);
	printf("< [SEND] len: %d\n%s\n\n", bsSend.GetNumberOfBytesUsed(), DumpMem(bsSend.GetData(), bsSend.GetNumberOfBytesUsed()));
	bsSend.Reset();

	ZeroMemory(byteRecv, sizeof(byteRecv));
	iRespLen = recv(hSocket, (char *)byteRecv, sizeof(byteRecv), NULL);

	// нрбер
	// ==============
	// бепяхъ X'05'
	// сяоеьмн X'00'
	// гюпегепбхпнбюммши аюир X'00'
	// рхо юдпеяю X'01'
	// юдпея - 4 аюирю
	// онпр - 2 аюирю
	// ==============

	if (iRespLen < 10) {
		printf("Proxy disconnected");
		closesocket(hSocket);
		getchar();
		return 0;
	}

	printf("> [RECV] len: %d\n%s\n\n", iRespLen, DumpMem(byteRecv, iRespLen));

	WORD wPortUDP;
	DWORD dwAddressUDP;
	BitStream bsRecv(byteRecv, iRespLen, FALSE);

	bsRecv.IgnoreBits(32);
	bsRecv.Read(dwAddressUDP); // юдпея - 4 аюирю
	bsRecv.Read(wPortUDP); // онпр - 2 аюирю

						   // сярюмнбйю онксвеммнцн юдпеяю х онпрю дкъ нропюбйх UDP оюйерю

	sockAddr.sin_addr.s_addr = dwAddressUDP;
	sockAddr.sin_port = wPortUDP;

	printf("Opening UDP session to %s with port %d...\n\n", inet_ntoa(sockAddr.sin_addr), ntohs(wPortUDP));

	SOCKET hSocketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	bsSend.Write((WORD)0x0000); // гюпегепбхпнбюммше аюирш X'0000'
	bsSend.Write((BYTE)0x00); // тпюцлемр X'00'
	bsSend.Write((BYTE)0x01); // рхо юдпеяю: IPv4 X'01'
	bsSend.Write((DWORD)inet_addr(SAMP_IP)); // юдпея жекебнцн яепбепю - 4 аюирю
	bsSend.Write((WORD)htons(SAMP_PORT)); // онпр жекебнцн яепбепю - 4 аюирю
	bsSend.Write("SAMP", 4); // дюммше - 11 аюир
	bsSend.Write((DWORD)inet_addr(SAMP_IP));
	bsSend.Write((WORD)htons(SAMP_PORT));
	bsSend.Write((BYTE)'i');

	sendto(hSocketUDP, (char *)bsSend.GetData(), bsSend.GetNumberOfBytesUsed(), NULL, (SOCKADDR *)&sockAddr, sizeof(sockAddr));
	printf("< [SENDTO] len: %d\n%s\n\n", bsSend.GetNumberOfBytesUsed(), DumpMem(bsSend.GetData(), bsSend.GetNumberOfBytesUsed()));
	bsSend.Reset();

	// дюкэье йнд ме опнундхр, рюй йюй мер бундъыецн UDP оюйерю

	iRespLen = recvfrom(hSocketUDP, (char *)byteRecv, sizeof(byteRecv), NULL, (sockaddr *)NULL, 0);
	printf("> [RECVFROM] len: %d\n%s\n\n", iRespLen, DumpMem(byteRecv, iRespLen));

	if (iRespLen == -1) {
		printf("Proxy disconnected");
		closesocket(hSocketUDP);
		closesocket(hSocket);
		getchar();
		return 0;
	}

	closesocket(hSocketUDP);
	closesocket(hSocket);
	WSACleanup();
	getchar();
	return 0;
}
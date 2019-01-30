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
	bsSend.Write((BYTE)0x01); // ���-�� ������� X'01'
	bsSend.Write((BYTE)0x00); // �����: ��� ����������� X'00'

	send(hSocket, (const char *)bsSend.GetData(), bsSend.GetNumberOfBytesUsed(), NULL);
	printf("< [SEND] len: %d\n%s\n\n", bsSend.GetNumberOfBytesUsed(), DumpMem(bsSend.GetData(), bsSend.GetNumberOfBytesUsed()));
	bsSend.Reset();

	unsigned char byteRecv[4096];
	int iRespLen;

	ZeroMemory(byteRecv, sizeof(byteRecv));
	iRespLen = recv(hSocket, (char *)byteRecv, sizeof(byteRecv), NULL);

	// �����
	// ==============
	// ������ X'05'
	// ������� X'00'
	// ==============

	if (iRespLen < 2) {
		printf("Proxy not active");
		closesocket(hSocket);
		getchar();
		return 0;
	}

	printf("> [RECV] len: %d\n%s\n\n", iRespLen, DumpMem(byteRecv, iRespLen));

	bsSend.Write((BYTE)0x05); // ������ X'05'
	bsSend.Write((BYTE)0x03); // �����: ���������� UDP ����� X'03'
	bsSend.Write((BYTE)0x00); // ����������������� ���� X'00'
	bsSend.Write((BYTE)0x01); // ��� ������: IPv4 X'01'
	bsSend.Write((DWORD)inet_addr(SAMP_IP)); // ����� �������� ������� - 4 �����
	bsSend.Write((WORD)htons(SAMP_PORT)); // ���� �������� ������� - 2 �����

	send(hSocket, (const char *)bsSend.GetData(), bsSend.GetNumberOfBytesUsed(), NULL);
	printf("< [SEND] len: %d\n%s\n\n", bsSend.GetNumberOfBytesUsed(), DumpMem(bsSend.GetData(), bsSend.GetNumberOfBytesUsed()));
	bsSend.Reset();

	ZeroMemory(byteRecv, sizeof(byteRecv));
	iRespLen = recv(hSocket, (char *)byteRecv, sizeof(byteRecv), NULL);

	// �����
	// ==============
	// ������ X'05'
	// ������� X'00'
	// ����������������� ���� X'00'
	// ��� ������ X'01'
	// ����� - 4 �����
	// ���� - 2 �����
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
	bsRecv.Read(dwAddressUDP); // ����� - 4 �����
	bsRecv.Read(wPortUDP); // ���� - 2 �����

						   // ��������� ����������� ������ � ����� ��� �������� UDP ������

	sockAddr.sin_addr.s_addr = dwAddressUDP;
	sockAddr.sin_port = wPortUDP;

	printf("Opening UDP session to %s with port %d...\n\n", inet_ntoa(sockAddr.sin_addr), ntohs(wPortUDP));

	SOCKET hSocketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	bsSend.Write((WORD)0x0000); // ����������������� ����� X'0000'
	bsSend.Write((BYTE)0x00); // �������� X'00'
	bsSend.Write((BYTE)0x01); // ��� ������: IPv4 X'01'
	bsSend.Write((DWORD)inet_addr(SAMP_IP)); // ����� �������� ������� - 4 �����
	bsSend.Write((WORD)htons(SAMP_PORT)); // ���� �������� ������� - 4 �����
	bsSend.Write("SAMP", 4); // ������ - 11 ����
	bsSend.Write((DWORD)inet_addr(SAMP_IP));
	bsSend.Write((WORD)htons(SAMP_PORT));
	bsSend.Write((BYTE)'i');

	sendto(hSocketUDP, (char *)bsSend.GetData(), bsSend.GetNumberOfBytesUsed(), NULL, (SOCKADDR *)&sockAddr, sizeof(sockAddr));
	printf("< [SENDTO] len: %d\n%s\n\n", bsSend.GetNumberOfBytesUsed(), DumpMem(bsSend.GetData(), bsSend.GetNumberOfBytesUsed()));
	bsSend.Reset();

	// ������ ��� �� ��������, ��� ��� ��� ��������� UDP ������

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
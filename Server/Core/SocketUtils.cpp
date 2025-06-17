#include "pch.h"
#include "Core/SocketUtils.h"

SOCKET SocketUtils::createSocket()
{
    // TCP 소켓 생성
    SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle == INVALID_SOCKET)
    {
        printError("소켓 생성 실패");
    }

    return socketHandle;
}

void SocketUtils::printError(const std::string& message)
{
    // WinSock 오류 코드 가져오기
    int errorCode = WSAGetLastError();
    std::cerr << message << " (오류 코드: " << errorCode << ")" << std::endl;
}

#pragma once

// SocketUtils 클래스는 소켓 생성 및 에러 처리를 위한 유틸리티를 제공합니다.
class SocketUtils
{
public:
    // 소켓 생성 메서드
    static SOCKET createSocket();

    // 소켓 에러 메시지 출력
    static void printError(const std::string& message);
};

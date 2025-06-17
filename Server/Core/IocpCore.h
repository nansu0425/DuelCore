#pragma once

// IocpCore 클래스는 IOCP 핸들을 관리합니다.
class IocpCore
{
public:
    // IOCP 핸들 생성
    static HANDLE createCompletionPort();

    // 소켓을 IOCP에 바인딩
    static bool bindSocketToIocp(HANDLE iocpHandle, SOCKET socket);

private:
    IocpCore() = default; // 인스턴스 생성 방지
};

#include "pch.h"
#include "Core/IocpCore.h"

HANDLE IocpCore::createCompletionPort()
{
    HANDLE iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!iocpHandle)
    {
        std::cerr << "IOCP 핸들 생성 실패! 오류 코드: " << GetLastError() << std::endl;
    }

    return iocpHandle;
}

bool IocpCore::bindSocketToIocp(HANDLE iocpHandle, SOCKET socket)
{
    HANDLE result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), iocpHandle, 0, 0);
    if (!result)
    {
        std::cerr << "소켓을 IOCP에 바인딩 실패! 오류 코드: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

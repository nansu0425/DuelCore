#pragma once

#include "Core/IocpEvent.h"
#include "Session/Session.h"

// NetworkService 클래스는 서버의 진입점 역할을 수행합니다.
class NetworkService
{
public:
    static constexpr size_t AcceptDataSize = 0; // 클라이언트가 연결할 때 보낼 수 있는 데이터 크기
    static constexpr size_t AcceptBufferSize = AcceptDataSize + (sizeof(SOCKADDR_IN) + 16) * 2; // Accept 버퍼 크기

public:
    // 서버 시작 메서드
    bool start(int port);

    // 워커 스레드 실행
    void runWorkerThreads(int count);

private:
    // AcceptEx 초기화 메서드 선언
    bool initializeAcceptEx();

    // 이벤트 처리
    void handleEvent(IocpEvent* event);

    // AcceptEx 요청
    void postAccept();

    // Accept 이벤트 처리
    void onAcceptComplete();

private:
    SOCKET m_listenSocket = INVALID_SOCKET; // 리스닝 소켓
    HANDLE m_iocpHandle = nullptr; // IOCP 핸들

    AcceptEvent m_acceptEvent; // Accept 완료 IOCP 이벤트
    char m_acceptBuffer[AcceptBufferSize] = {}; // Accept 버퍼
};

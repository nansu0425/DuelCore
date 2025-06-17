#pragma once

#include "Core/IocpEvent.h"

// Session 상태를 나타내는 열거형
enum class SessionState
{
    Connected,
    Disconnected
};

// Session 클래스는 클라이언트와의 연결을 관리합니다.
class Session : public std::enable_shared_from_this<Session>
{
public:
    static const size_t RecvBufferSize = 1024; // 수신 버퍼 크기

public:
    explicit Session(SOCKET socket);
    ~Session();

    // 비동기 수신 요청
    void postRecv();

    // 비동기 송신 요청
    void postSend(const char* data, int len);

    // 수신 완료 처리
    void onRecvComplete();

    // 송신 완료 처리
    void onSendComplete(SendEvent* event);

    SOCKET getSocket() const;

private:
    void setDisconnected(); // 상태를 끊어진 상태로 변경

private:
    SOCKET m_socket = INVALID_SOCKET; // 클라이언트 소켓
    SessionState m_state = SessionState::Connected; // 세션 상태
    std::mutex m_stateMutex; // 상태 변경 보호를 위한 뮤텍스

    RecvEvent m_recvEvent; // 수신 이벤트
    char m_recvBuffer[RecvBufferSize] = {}; // 수신 데이터를 저장할 버퍼
};

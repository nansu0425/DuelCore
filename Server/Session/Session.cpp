#include "pch.h"
#include "Session/Session.h"
#include "Session/SessionManager.h"

Session::Session(SOCKET socket)
{
    m_socket = socket;
    std::cout << "Session 생성: 소켓 핸들 = " << m_socket << std::endl;
}

Session::~Session()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        std::cout << "Session 소멸: 소켓 핸들 = " << m_socket << std::endl;
    }
}

void Session::postRecv()
{
    // 수신 이벤트 및 버퍼 설정
    m_recvEvent.clear();
    m_recvEvent.session = shared_from_this();
    memset(m_recvBuffer, 0, RecvBufferSize);

    WSABUF buffer;
    buffer.buf = m_recvBuffer;
    buffer.len = RecvBufferSize;
    DWORD flags = 0;
    DWORD bytesTransferred = 0;

    // 비동기 수신 요청
    int result = WSARecv(
        m_socket,
        &buffer,
        1,
        &bytesTransferred,
        &flags,
        reinterpret_cast<LPWSAOVERLAPPED>(&m_recvEvent),
        nullptr
    );

    if (result == SOCKET_ERROR)
    {
        int errorCode = WSAGetLastError();
        // 오류 코드가 WSA_IO_PENDING이 아닌 경우 수신 요청 실패
        if (errorCode != WSA_IO_PENDING)
        {
            std::cerr << "WSARecv 실패! 오류 코드: " << errorCode << std::endl;
            setDisconnected();
            m_recvEvent.session = nullptr;
            return;
        }
    }
    else
    {
        // 수신 요청 즉시 완료
        m_recvEvent.bytesTransferred = bytesTransferred;
        onRecvComplete();
    }
}

void Session::postSend(const char* data, int len)
{
    // 비동기 송신 요청을 위한 SendEvent 생성
    SendEvent* sendEvent = new SendEvent(data, len);
    sendEvent->session = shared_from_this();

    WSABUF buffer;
    buffer.buf = sendEvent->buffer.data();
    buffer.len = sendEvent->buffer.size();
    DWORD bytesTransferred = 0;

    // 비동기 송신 요청
    int result = WSASend(
        m_socket,
        &buffer,
        1,
        &bytesTransferred,
        0,
        reinterpret_cast<LPWSAOVERLAPPED>(sendEvent),
        nullptr
    );

    if (result == SOCKET_ERROR)
    {
        int errorCode = WSAGetLastError();
        // 오류 코드가 WSA_IO_PENDING이 아닌 경우 송신 요청 실패
        if (errorCode != WSA_IO_PENDING)
        {
            std::cerr << "WSASend 실패! 오류 코드: " << errorCode << std::endl;
            setDisconnected();
            delete sendEvent;
            return;
        }
    }
    else
    {
        // 송신 요청 즉시 완료
        sendEvent->bytesTransferred = bytesTransferred;
        onSendComplete(sendEvent);
    }
}

void Session::onRecvComplete()
{
    m_recvEvent.session = nullptr;

    // graceful shutdown 확인
    if (m_recvEvent.bytesTransferred == 0)
    {
        std::cerr << "Session::onRecvComplete: 0 바이트 수신. 연결이 종료되었습니다." << std::endl;
        setDisconnected();
        return;
    }

    std::cout << "Session::onRecvComplete 호출: 수신된 데이터 길이 = " << m_recvEvent.bytesTransferred << std::endl;

    // 수신된 데이터를 처리
    std::cout << "수신된 데이터: " << m_recvBuffer << std::endl;

    // 다음 수신 요청
    postRecv();
}

void Session::onSendComplete(SendEvent* event)
{
    std::cout << "Session::onSendComplete 호출: 송신된 데이터 길이 = " << event->bytesTransferred << std::endl;

    // 송신 완료 후 추가 작업이 필요한 경우 처리

    // 이벤트 객체 삭제
    delete event;
}

void Session::setDisconnected()
{
    {
        std::lock_guard<std::mutex> lock(m_stateMutex); // 상태 변경 보호를 위한 뮤텍스

        if (m_state == SessionState::Disconnected)
        {
            return; // 이미 끊어진 상태라면 아무 작업도 하지 않음
        }

        m_state = SessionState::Disconnected;
        std::cout << "Session 상태 변경: Disconnected" << std::endl;
    }

    // 최초로 끊어진 상태로 변경될 때만 SessionManager::destroySession 호출
    SessionManager::destroySession(shared_from_this());
}

SOCKET Session::getSocket() const
{
    return m_socket;
}

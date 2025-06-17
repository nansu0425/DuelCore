#include "pch.h"
#include "Core/NetworkService.h"
#include "Core/IocpCore.h"
#include "Core/IocpEvent.h"
#include "Core/SocketUtils.h"
#include "Session/SessionManager.h"

// AcceptEx 함수 포인터 선언
LPFN_ACCEPTEX g_acceptEx = nullptr;

bool NetworkService::start(int port)
{
    // WinSock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup 실패!" << std::endl;
        return false;
    }

    // 리스닝 소켓 생성
    m_listenSocket = SocketUtils::createSocket();
    if (m_listenSocket == INVALID_SOCKET)
    {
        std::cerr << "리스닝 소켓 생성 실패!" << std::endl;
        return false;
    }

    // 소켓을 지정된 포트에 바인딩
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET; // IPv4 사용
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 모든 IP 주소 허용
    serverAddr.sin_port = htons(port); // 포트를 네트워크 바이트 순서로 변환

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "소켓 바인딩 실패!" << std::endl;
        return false;
    }

    // 클라이언트 연결 대기 시작
    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "소켓 리스닝 실패!" << std::endl;
        return false;
    }

    // IOCP 핸들 생성
    m_iocpHandle = IocpCore::createCompletionPort();
    if (!m_iocpHandle)
    {
        std::cerr << "IOCP 핸들 생성 실패!" << std::endl;
        return false;
    }

    // AcceptEx 초기화
    if (!initializeAcceptEx())
    {
        return false;
    }

    // 첫 번째 Accept 요청
    postAccept();

    std::cout << "서버가 포트 " << port << "에서 성공적으로 시작되었습니다." << std::endl;
    return true;
}

bool NetworkService::initializeAcceptEx()
{
    DWORD bytesReturned = 0;
    GUID acceptExGuid = WSAID_ACCEPTEX;

    // AcceptEx 함수 주소를 가져옴
    if (WSAIoctl(
        m_listenSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &acceptExGuid,
        sizeof(acceptExGuid),
        &g_acceptEx,
        sizeof(g_acceptEx),
        &bytesReturned,
        nullptr,
        nullptr
    ) == SOCKET_ERROR)
    {
        std::cerr << "AcceptEx 초기화 실패! 오류 코드: " << WSAGetLastError() << std::endl;
        return false;
    }

    std::cout << "AcceptEx 초기화 성공!" << std::endl;
    return true;
}

void NetworkService::runWorkerThreads(int count)
{
    for (int i = 0; i < count; ++i)
    {
        std::thread([this]()
                    {
                        while (true)
                        {
                            DWORD bytesTransferred = 0;
                            ULONG_PTR completionKey = 0;
                            LPOVERLAPPED overlapped = nullptr;

                            // IOCP 이벤트 수신
                            BOOL result = GetQueuedCompletionStatus(
                                m_iocpHandle,
                                &bytesTransferred,
                                &completionKey,
                                &overlapped,
                                INFINITE
                            );

                            if (!result)
                            {
                                std::cerr << "GetQueuedCompletionStatus 실패! 오류 코드: " << GetLastError() << std::endl;
                                continue;
                            }

                            IocpEvent* event = reinterpret_cast<IocpEvent*>(overlapped);

                            if (event)
                            {
                                event->bytesTransferred = bytesTransferred;
                                handleEvent(event);
                            }
                            else
                            {
                                std::cerr << "이벤트가 null입니다!" << std::endl;
                            }
                        }
                    }).detach();
    }

    std::cout << "워커 스레드가 시작되었습니다: " << count << "개" << std::endl;
}

void NetworkService::handleEvent(IocpEvent* event)
{
    switch (event->type)
    {
    case EventType::Recv:
        static_cast<RecvEvent*>(event)->session->onRecvComplete();
        break;
    case EventType::Send:
        SendEvent* sendEvent = static_cast<SendEvent*>(event);
        sendEvent->session->onSendComplete(sendEvent);
        break;
    case EventType::Accept:
        onAcceptComplete();
        break;
    default:
        std::cerr << "알 수 없는 이벤트 타입!" << std::endl;
        break;
    }
}

void NetworkService::postAccept()
{
    // 클라이언트 소켓 생성
    SOCKET clientSocket = SocketUtils::createSocket();
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "클라이언트 소켓 생성 실패!" << std::endl;
        return;
    }

    // IOCP에 클라이언트 소켓 바인딩
    if (!IocpCore::bindSocketToIocp(m_iocpHandle, clientSocket))
    {
        std::cerr << "Accept 이벤트 처리 실패: IOCP 바인딩 실패!" << std::endl;
        closesocket(clientSocket);
        return;
    }

    // Accpet 이벤트 및 버퍼 설정
    m_acceptEvent.clear();
    m_acceptEvent.clientSocket = clientSocket;
    memset(m_acceptBuffer, 0, AcceptBufferSize);

    DWORD bytesTransferred = 0;

    // AcceptEx 호출
    BOOL result = g_acceptEx(
        m_listenSocket,
        clientSocket,
        m_acceptBuffer,
        0,
        sizeof(sockaddr_in) + 16,
        sizeof(sockaddr_in) + 16,
        &bytesTransferred,
        reinterpret_cast<LPWSAOVERLAPPED>(&m_acceptEvent)
    );

    if (result == TRUE)
    {
        // AcceptEx가 즉시 완료된 경우
        m_acceptEvent.bytesTransferred = bytesTransferred;
        onAcceptComplete();
    }
    else
    {
        // 오류 코드가 WSA_IO_PENDING이 아닌 경우 AcceptEx 실패 
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            std::cerr << "AcceptEx 실패! 오류 코드: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            return;
        }
    }
}

void NetworkService::onAcceptComplete()
{
    SOCKET clientSocket = m_acceptEvent.clientSocket;

    // 세션 생성
    std::shared_ptr<Session> newSession = SessionManager::createSession(clientSocket);
    if (!newSession)
    {
        std::cerr << "Accept 이벤트 처리 실패: 세션 생성 실패!" << std::endl;
        closesocket(clientSocket);
        return;
    }    

    // 초기 수신 요청
    newSession->postRecv();
    std::cout << "Accept 이벤트 처리 성공: 세션 생성 완료." << std::endl;

    // 다음 AcceptEx 요청
    postAccept();
}

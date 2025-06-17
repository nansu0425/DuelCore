# 🧱 DuelCore 1주차: IOCP 서버 틀 설계 문서

> 🎯 목적: 기본 IOCP 서버 구조 설계를 완료하고, VS 기반 프로젝트의 뼈대를 구성한다.

---

## ✅ 설계 목표

- IOCP 기반 비동기 소켓 구조 설계
- `Session`, `NetworkService`, `IocpEvent` 클래스 설계
- Accept, Recv, Worker 스레드 구조 초안 작성

---

## 📦 클래스 구성

| 클래스명 | 설명 |
|----------|------|
| `NetworkService` | 서버의 진입점. 소켓 Listen 및 IOCP 등록 처리 |
| `IocpCore` | IOCP 핸들 및 CompletionPort 제어 |
| `IocpEvent` | OVERLAPPED 기반 이벤트 구조체 확장 |
| `Session` | 클라이언트 세션 객체. Recv/Send 처리 |
| `SessionManager` | 세션 풀 및 ID 매핑 관리 |
| `SocketUtils` | WinSock 유틸리티 래퍼 모음 (socket 생성, 에러 처리 등) |

---

## 🔄 기본 구조 흐름

```
main
└── NetworkService::Start()
    ├── WSAStartup + Listen 소켓 초기화
    ├── AcceptThread 시작
    └── WorkerThreadPool 생성

WorkerThread
└── GetQueuedCompletionStatus()
    ├── 이벤트 타입 확인
    └── Session의 OnRecv / OnSend / OnConnect 호출

AcceptThread
└── AcceptEx() 로 신규 소켓 수신
    └── Session 객체에 소켓 바인딩
```

---

## 🧩 주요 클래스 예시

### 🔹 `IocpEvent`

```cpp
enum class EventType     
{ 
    Recv, 
    Send, 
    Accept 
};

struct IocpEvent : public OVERLAPPED 
{
    EventType type;
    Session* owner;
    WSABUF buffer;
};
```

---

### 🔹 `Session`

```cpp
class Session     
{
public:
    void postRecv();
    void postSend(const char* data, int len);
    void onRecvComplete(int len);
    void onSendComplete(int len);

private:
    SOCKET m_socket;
};
```

---

### 🔹 `NetworkService`

```cpp
class NetworkService     
{
public:
    bool start(int port);
    void runAcceptLoop();
    void runWorkerThreads(int count);
    
private:
    SOCKET m_listenSocket;
    HANDLE m_iocpHandle;
};
```

---

## 🚦 1주차 작업 우선순위

| 단계 | 내용 |
|------|------|
| 1 | `NetworkService` 틀 작성 (Listen 소켓 + 포트 바인딩) |
| 2 | `IocpCore`, `IocpEvent` 구조 정의 |
| 3 | `Session` 추상 클래스 인터페이스 설계 |
| 4 | Accept → Recv 이벤트 처리 흐름 설계 |
| 5 | WorkerThread 루프 설계 및 이벤트 분기 처리 |

---

## ⚠️ 예외 처리 고려사항

- `GetQueuedCompletionStatus()` 실패 시 루프 유지 및 로깅
- `WSARecv`, `WSASend` 0 바이트 송수신 처리
- 세션 종료 시 `PostQueuedCompletionStatus()` 통한 안전한 루프 종료 처리

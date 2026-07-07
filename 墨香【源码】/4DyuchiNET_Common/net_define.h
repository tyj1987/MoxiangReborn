#pragma once
//
// 4DyuchiNET — connection action / status enums and bit masks used by
// the per-connection handle ID and the main thread event dispatcher.
//

enum ACTOR_TYPE
{
    ACTOR_TYPE_SERVER = 0x80000000,   // server-side accept / connect
    ACTOR_TYPE_USER   = 0x00000000,   // client-side user connection
};

enum ACTION_TYPE
{
    ACTION_TYPE_TCP        = 0x00000000,
    ACTION_TYPE_UDP        = 0x10000000,
    ACTION_TYPE_DISCONNECT = 0x20000000,
    ACTION_TYPE_SWITCH_QUE = 0x30000000,
    ACTION_TYPE_DESTROY    = 0x40000000,
};

enum NETWORK_ID
{
    ID_NETWORK_FOR_USER   = ACTOR_TYPE_USER,
    ID_NETWORK_FOR_SERVER = ACTOR_TYPE_SERVER,
    ID_NETWORK_FOR_UDP    = ACTION_TYPE_UDP,
};

#define ACTION_TYPE_BIT_MASK          0x70000000
#define ACTOR_TYPE_BIT_MASK           0x80000000
#define HEADER_BIT_MASK               0xf0000000
#define CONNECTION_INDEX_BIT_MASK     0x0fffffff

#define INVERSE_ACTION_TYPE_BIT_MASK  0x8fffffff
#define INVERSE_ACTOR_TYPE_BIT_MASK   0x7fffffff


enum CONNECTION_STATUS
{
    CONNECTION_STATUS_DISCONNECTED = 0,
    CONNECTION_STATUS_CONNECTED    = 1,
    CONNECTION_STATUS_CRASHED      = 1011,
    CONNECTION_STATUS_CANNOT_SEND  = 1012,
    CONNECTION_STATUS_CAN_SEND     = 1013,
};


enum MAIN_THREAD_EVENT_INDEX
{
    EVENT_INDEX_DESTROY        = 0,
    EVENT_INDEX_MSG_EVENT      = 1,
    EVENT_INDEX_BREAK          = 2,
    EVENT_INDEX_RESUME         = 3,
    EVENT_INDEX_USER_DEFINE_0  = 4,
    EVENT_INDEX_USER_DEFINE_1  = 5,
    EVENT_INDEX_USER_DEFINE_2  = 6,
    EVENT_INDEX_USER_DEFINE_3  = 7,
    EVENT_INDEX_USER_DEFINE_4  = 8,
    EVENT_INDEX_PRE_CONNECT    = 9,
};

#define MIM_MAIN_THREAD_EVENT_NUM                  5
#define MAX_MAIN_THREAD_EVENT_NUM                  64
#define MAX_MAIN_THREAD_USER_DEFINE_EVENT_NUM     (MAX_MAIN_THREAD_EVENT_NUM - MIM_MAIN_THREAD_EVENT_NUM)


// Per-send maximum number of WSABUF entries (overlapped_send.cpp).
// Sized for a typical multi-packet send without overflow. NOT a
// configurable gameplay parameter — only the in-engine upper bound.
#define MAX_WSABUF_NUM_IN_SEND_FUNC                8

// Maximum number of timers registered with the main thread event
// loop (timer.cpp). 64 concurrent timers is more than any mokgam
// (room) needs (each spawn uses one). Legacy sources reference this
// constant by name; the original numeric value was lost when
// 4DyuchiNET_Common/ disappeared. Tracked in KNOWN_BUGS.md Bug C-22.
#define MAX_TIMER_NUM                              64

// Number of WSARecv / WSASend worker threads servicing the IOCP
// completion queue. Sized to (current host CPU count, max 8) at
// runtime via cpio.cpp — see g_dwWorkerThreadNum. The compile-time
// array bound is the upper limit. Legacy sources reference this
// constant by name; the original value was lost when
// 4DyuchiNET_Common/ disappeared. Tracked in KNOWN_BUGS.md Bug C-23.
#define MAX_WORKER_THREAD_NUM                      8

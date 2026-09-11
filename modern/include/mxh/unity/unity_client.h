#pragma once

#include <stdint.h>
#ifdef __cplusplus
#include <type_traits>
#endif

#if defined(_WIN32)
#  if defined(MXH_UNITY_CORE_BUILD)
#    define MXH_UNITY_API __declspec(dllexport)
#  else
#    define MXH_UNITY_API __declspec(dllimport)
#  endif
#  define MXH_UNITY_CALL __cdecl
#else
#  define MXH_UNITY_API __attribute__((visibility("default")))
#  define MXH_UNITY_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MXH_UNITY_API_VERSION UINT32_C(0x00010003)
#define MXH_UNITY_MAX_HOST_BYTES UINT32_C(255)
#define MXH_UNITY_MAX_CREDENTIAL_BYTES UINT32_C(17)
#define MXH_UNITY_MAX_NAME_BYTES UINT32_C(64)
#define MXH_UNITY_MAX_EVENT_TEXT_BYTES UINT32_C(255)
#define MXH_UNITY_MAX_CHARACTER_SLOTS UINT32_C(5)
/* Bounded versioned timed-movement wire payload cap: hello is 8 bytes; a
   15-point command is 24 + 4*15 = 84 bytes. The native core rejects anything
   outside this range. */
#define MXH_UNITY_TIMED_MOVEMENT_MAX_PAYLOAD UINT32_C(84)
#define MXH_UNITY_TIMED_MOVEMENT_HELLO_PAYLOAD UINT32_C(8)

typedef uint64_t mxh_unity_handle;

typedef enum mxh_unity_result {
    MXH_UNITY_OK = 0,
    MXH_UNITY_INVALID_ARGUMENT = 1,
    MXH_UNITY_INVALID_HANDLE = 2,
    MXH_UNITY_WRONG_STATE = 3,
    MXH_UNITY_NETWORK_ERROR = 4,
    MXH_UNITY_PROTOCOL_ERROR = 5,
    MXH_UNITY_BUFFER_TOO_SMALL = 6,
    MXH_UNITY_UNSUPPORTED = 7,
    MXH_UNITY_NOT_READY = 8,
    MXH_UNITY_INTERNAL_ERROR = 9,
    /* The server understood a non-terminal request but refused it. */
    MXH_UNITY_REJECTED = 10
} mxh_unity_result;

typedef enum mxh_unity_state {
    MXH_UNITY_STATE_IDLE = 0,
    MXH_UNITY_STATE_LOGIN_CONNECTING = 1,
    MXH_UNITY_STATE_AWAIT_LOGIN_ACK = 2,
    MXH_UNITY_STATE_AGENT_CONNECTING = 3,
    MXH_UNITY_STATE_AWAIT_CHARACTER_LIST = 4,
    MXH_UNITY_STATE_CHARACTER_LIST_READY = 5,
    MXH_UNITY_STATE_AWAIT_CHARACTER_SELECT = 6,
    MXH_UNITY_STATE_AWAIT_GAME_IN = 7,
    MXH_UNITY_STATE_IN_GAME = 8,
    MXH_UNITY_STATE_FAILED = 9,
    MXH_UNITY_STATE_SHUTTING_DOWN = 10,
    MXH_UNITY_STATE_AWAIT_CHARACTER_CREATE = 11
} mxh_unity_state;

typedef enum mxh_unity_event_type {
    MXH_UNITY_EVENT_STATE_CHANGED = 1,
    MXH_UNITY_EVENT_CHARACTER_LIST = 2,
    MXH_UNITY_EVENT_GAME_IN = 3,
    MXH_UNITY_EVENT_DISCONNECTED = 4,
    MXH_UNITY_EVENT_ERROR = 5,
    MXH_UNITY_EVENT_CHARACTER_CREATE = 6,
    /* Movement events: argument0=object ID, argument1=x|(z<<16), reserved0=
       wire MoveProtocol. SUBMITTED is local prediction, never server acceptance.
       CORRECTION has request_id=0: the legacy wire has no request correlation. */
    MXH_UNITY_EVENT_MOVEMENT_SUBMITTED = 7,
    MXH_UNITY_EVENT_POSITION_CORRECTION = 8,
    MXH_UNITY_EVENT_OBJECT_MOVEMENT = 9,
    /* Timed-movement owner broadcast: argument0=object ID, argument1=(x|(z<<16)),
       reserved0=Move owner_state_protocol (130). request_id carries the
       related command sequence (lower 32 bits) so the Unity side can match
       submitted routes. sequence carries the server state sequence. */
    MXH_UNITY_EVENT_TIMED_MOVEMENT_OWNER_STATE = 10,
    /* Timed-movement observer broadcast: same packing as owner state but
       reserved0=Move observer_state_protocol (131); text carries the encoded
       wire payload verbatim, length = text_length. */
    MXH_UNITY_EVENT_TIMED_MOVEMENT_OBSERVER_STATE = 11
    ,MXH_UNITY_EVENT_MONSTER_ADDED = 12
    ,MXH_UNITY_EVENT_NPC_ADDED = 13
    ,MXH_UNITY_EVENT_ENTITY_REMOVED = 14
    /* Monster LifeNotify: argument0=object ID, argument1=current life. */
    ,MXH_UNITY_EVENT_ENTITY_LIFE = 15
    /* Monster LifeNotify shield component: argument0=object ID,
       argument1=current shield. */
    ,MXH_UNITY_EVENT_ENTITY_SHIELD = 16
    ,MXH_UNITY_EVENT_GROUND_DROP = 17
} mxh_unity_event_type;

typedef enum mxh_unity_command_type {
    MXH_UNITY_COMMAND_SELECT_CHARACTER = 1,
    MXH_UNITY_COMMAND_CREATE_CHARACTER = 2,
    /* argument0=x, argument1=z in legacy uint16 game units; payload_size=0. */
    MXH_UNITY_COMMAND_MOVE = 3,
    MXH_UNITY_COMMAND_STOP = 4,
    /* Bounded versioned timed-movement wire (only valid when the server was
       started with --experimental-timed-movement). payload carries the literal
       MXMH (8 bytes) or MXMC (24 + 4N bytes, N in 1..15) bytes, and is sent as
       a Move subprotocol. Use mxh_unity_submit_extended_command to send these;
       mxh_unity_submit_command with type 5/6/7 is rejected. */
    MXH_UNITY_COMMAND_HELLO_TIMED = 5,
    MXH_UNITY_COMMAND_TIMED_ROUTE = 6,
    MXH_UNITY_COMMAND_TIMED_STOP = 7,
    /* Skill StartSyn: head.argument0=skill index, argument1=target object;
       payload is exactly target_x/target_z as little-endian float32. */
    MXH_UNITY_COMMAND_SKILL = 8
} mxh_unity_command_type;

#define MXH_UNITY_SKILL_PAYLOAD_SIZE UINT32_C(8)

/* Bytes after expected_map_generation used by CREATE_CHARACTER. */
#define MXH_UNITY_CREATE_COMMAND_PAYLOAD_SIZE UINT32_C(80)

enum {
    MXH_UNITY_CONNECT_USE_HSEL = 1u << 0,
    MXH_UNITY_CONNECT_LEGACY_TEXT_CP949 = 1u << 1,
    MXH_UNITY_CONNECT_LEGACY_TEXT_CP936 = 1u << 2
};

#pragma pack(push, 1)
typedef struct mxh_unity_connect_args {
    uint32_t struct_size; /* sizeof(mxh_unity_connect_args) */
    uint32_t flags;
    uint32_t timeout_ms;
    uint16_t login_port;
    uint16_t reserved0;
    uint32_t host_length;
    uint32_t user_id_length;
    uint32_t password_length;
    char login_host[256];
    char user_id[64];
    char password[64];
} mxh_unity_connect_args;

typedef struct mxh_unity_command {
    uint32_t struct_size; /* sizeof(mxh_unity_command) */
    uint32_t type;
    uint32_t payload_size;
    uint32_t reserved0;
    uint32_t argument0;
    uint32_t argument1;
    uint64_t request_id;
    uint64_t expected_session_generation; /* reject commands from an old login */
    uint64_t expected_map_generation;     /* reject commands from an old map */
    /* CREATE_CHARACTER semantic payload. name is UTF-8 and is never a raw
       legacy wire field. The native core validates/transcodes it and maps the
       option indices to the locked China baseline values. */
    uint32_t name_length;
    char name[65];
    uint8_t sex_type;      /* 0..1 */
    uint8_t hair_type;     /* 0..4 */
    uint8_t face_type;     /* 0..4 */
    uint8_t cloth_option;  /* 0..1 */
    uint8_t boot_option;   /* 0..1 */
    uint8_t weapon_option; /* 0..5 */
    uint8_t reserved1[5];
} mxh_unity_command;

/* Variable-length payload carrier for the bounded timed-movement wire.
   The payload bytes are the literal MXMH or MXMC frame; the native core wraps
   them as a Move subprotocol message and forwards via the same agent path as
   the legacy Move command. payload is read but not retained, so the caller can
   reuse the buffer once the function returns. */
typedef struct mxh_unity_extended_command {
    mxh_unity_command head;
    const uint8_t* payload;
    uint32_t payload_size;
} mxh_unity_extended_command;

typedef struct mxh_unity_event {
    uint32_t struct_size;
    uint32_t type;
    uint32_t result;
    uint32_t state;
    uint64_t sequence;
    uint64_t request_id;
    uint64_t session_generation;
    uint64_t map_generation;
    uint32_t argument0;
    uint32_t argument1;
    uint32_t text_length;
    uint32_t reserved0;
    char text[256];
} mxh_unity_event;

typedef struct mxh_unity_character_slot {
    uint32_t character_id;
    uint32_t valid;
    uint16_t level;
    uint16_t map_number;
    uint8_t gender;
    uint8_t face_type;
    uint8_t hair_type;
    uint8_t reserved0;
    uint16_t worn_item_index[10];
    uint32_t name_length;
    char name[65];
    uint8_t reserved1[3];
} mxh_unity_character_slot;

typedef struct mxh_unity_game_snapshot {
    uint32_t player_id;
    uint32_t user_id;
    uint32_t name_length;
    char name[65];
    uint8_t reserved0[3];
    uint16_t level;
    uint16_t map_number;
    uint32_t life;
    uint32_t max_life;
    uint32_t mp;
    uint32_t max_mp;
    uint64_t experience;
    uint32_t money;
    uint16_t gen_gol;
    uint16_t min_chub;
    uint16_t che_ryuk;
    uint16_t sim_mek;
    uint16_t position_x; /* predicted after a submitted move; server corrections replace it */
    uint16_t position_z;
    uint16_t server_year;
    uint16_t server_month;
    uint16_t server_day;
    uint16_t server_hour;
} mxh_unity_game_snapshot;

typedef struct mxh_unity_snapshot {
    uint32_t struct_size;
    uint32_t api_version;
    uint32_t state;
    uint32_t last_result;
    uint64_t revision;
    uint64_t session_generation;
    uint64_t map_generation;
    uint32_t user_id;
    uint32_t selected_character_id;
    uint16_t selected_map_number;
    uint16_t character_count;
    uint64_t dropped_event_count;
    mxh_unity_character_slot characters[5];
    mxh_unity_game_snapshot game;
    uint32_t error_length;
    char error[256];
} mxh_unity_snapshot;
#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(mxh_unity_connect_args) == 412,
              "mxh_unity_connect_args ABI changed");
static_assert(sizeof(mxh_unity_command) == 128,
              "mxh_unity_command ABI changed");
static_assert(sizeof(mxh_unity_extended_command) == 140,
              "mxh_unity_extended_command ABI changed");
static_assert(sizeof(mxh_unity_character_slot) == 108,
              "mxh_unity_character_slot ABI changed");
static_assert(sizeof(mxh_unity_game_snapshot) == 132,
              "mxh_unity_game_snapshot ABI changed");
static_assert(sizeof(mxh_unity_event) == 320,
              "mxh_unity_event ABI changed");
static_assert(sizeof(mxh_unity_snapshot) == 992,
              "mxh_unity_snapshot ABI changed");
static_assert(std::is_trivially_copyable_v<mxh_unity_connect_args> &&
              std::is_trivially_copyable_v<mxh_unity_command> &&
              std::is_trivially_copyable_v<mxh_unity_extended_command> &&
              std::is_trivially_copyable_v<mxh_unity_event> &&
              std::is_trivially_copyable_v<mxh_unity_snapshot>,
              "Unity ABI structs must remain POD-compatible");
#endif

MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_get_api_version(void);
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_create(
    mxh_unity_handle* out_handle);
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_destroy(
    mxh_unity_handle handle);
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_connect(
    mxh_unity_handle handle, const mxh_unity_connect_args* args);
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_disconnect(
    mxh_unity_handle handle);
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_tick(
    mxh_unity_handle handle);
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_submit_command(
    mxh_unity_handle handle, const mxh_unity_command* command);
/* Bounded versioned timed-movement wire entry. The native core validates
   payload_size, sends the bytes as a Move subprotocol (hello=128 or
   command=129 depending on head.type), and updates the per-session epoch and
   command sequence atomically. Returns MXH_UNITY_INVALID_ARGUMENT for
   unsupported types or out-of-range sizes; MXH_UNITY_WRONG_STATE if the
   session is not IN_GAME; MXH_UNITY_NOT_READY when the underlying network
   send fails. */
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_submit_extended_command(
    mxh_unity_handle handle, const mxh_unity_extended_command* command);
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_poll_event(
    mxh_unity_handle handle, void* event_buffer, uint32_t event_buffer_size,
    uint32_t* out_required_size);
/* copy_snapshot and poll_event always publish the required byte size when the
   out_required_size pointer is valid. A short poll buffer does not consume the
   pending event. Empty event queues return MXH_UNITY_NOT_READY. */
MXH_UNITY_API uint32_t MXH_UNITY_CALL mxh_unity_copy_snapshot(
    mxh_unity_handle handle, void* snapshot_buffer,
    uint32_t snapshot_buffer_size, uint32_t* out_required_size);

#ifdef __cplusplus
}
#endif

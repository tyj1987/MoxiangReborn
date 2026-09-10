#include "mxh/unity/unity_client.h"

typedef char assert_connect_args_abi[(sizeof(mxh_unity_connect_args) == 412) ? 1 : -1];
typedef char assert_command_abi[(sizeof(mxh_unity_command) == 48) ? 1 : -1];
typedef char assert_character_abi[(sizeof(mxh_unity_character_slot) == 108) ? 1 : -1];
typedef char assert_game_abi[(sizeof(mxh_unity_game_snapshot) == 132) ? 1 : -1];
typedef char assert_event_abi[(sizeof(mxh_unity_event) == 320) ? 1 : -1];
typedef char assert_snapshot_abi[(sizeof(mxh_unity_snapshot) == 992) ? 1 : -1];

int main(void) {
    mxh_unity_handle handle = 0;
    if (mxh_unity_get_api_version() != MXH_UNITY_API_VERSION) return 1;
    if (mxh_unity_create(&handle) != MXH_UNITY_OK || handle == 0) return 2;
    if (mxh_unity_destroy(handle) != MXH_UNITY_OK) return 3;
    return 0;
}

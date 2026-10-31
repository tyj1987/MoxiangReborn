add_executable(mxh_client_tests
    cmain_game_test.cpp
    game_state_stubs_test.cpp
    cmain_title_test.cpp
    clogin_state_test.cpp
    ccharselect_state_test.cpp
    cchar_make_state_test.cpp
    cingame_state_test.cpp
    cingame_input_test.cpp
    ingame_hud_binding_test.cpp
    monster_life_authority_test.cpp
    protocol_burst_test.cpp
    client_e2e_flow_test.cpp
    network_event_queue_test.cpp
    state_transfer_test.cpp
    sprite_render_geometry_test.cpp
    logical_viewport_test.cpp
    world_screen_projection_test.cpp
    client_ui_runtime_test.cpp
    effect_runtime_test.cpp
    char_make_options_test.cpp
    character_preview_controller_test.cpp
    map10_smoke.cpp
    map10_smoke_endpoint_test.cpp
)

target_include_directories(mxh_client_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)
target_link_libraries(mxh_client_tests
    PRIVATE
    mxh_client_lib
    mxh_log
    mxh_render
    gtest_main
)
target_compile_definitions(mxh_client_tests PRIVATE MXH_CLIENT_TEST_BUILD=1)

if(MSVC)
    target_compile_options(mxh_client_tests PRIVATE /W3 /utf-8)
endif()

include(GoogleTest)

# Login/session retry contract. Header-only policy plus the connect-retry
# clock. The Win32 client (objbase.h / DX11) and MoxianClient/main.cpp host
# loop are not linked, and no sockets are opened.
add_executable(mxh_session_retry_contract_tests
    session_retry_contract_test.cpp)
target_include_directories(mxh_session_retry_contract_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(mxh_session_retry_contract_tests PRIVATE gtest_main)
target_compile_features(mxh_session_retry_contract_tests PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(mxh_session_retry_contract_tests PRIVATE /W3 /utf-8)
endif()
gtest_add_tests(TARGET mxh_session_retry_contract_tests)
# Client tests that exercise the shipped InterfaceScript and sprite atlas use
# the source tree's `data/PlayDH` fixture.  Pin both enumerated and aggregate
# invocations to the source root so the fixture-only resource fallback in
# CInGameState::Init is deterministic when CTest runs from an out-of-tree
# build directory.
gtest_add_tests(TARGET mxh_client_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(
    ClientUiRuntime.DispatchesRealLegacyFunctionButton
    ClientUiRuntime.DispatchesRealPushupCharacterSlot
    ClientUiRuntime.TabAndShiftTabCycleRealLoginControls
    ClientUiRuntime.KeyboardConfirmProducesButtonActivation
    ClientUiRuntime.HidingActiveSetClearsStaleKeyboardFocus
    ClientUiRuntime.RoutesCharactersOnlyToFocusedEditBox
    ClientUiRuntime.ClearReleasesStateOwnedDialogTree
    ClientUiRuntime.LoadsGameDialogsWithoutOverwritingLegacyActiveFlags
    ClientUiRuntime.FailedMultiLoadKeepsPreviousTreeIntact
    ClientUiRuntime.ActiveChildAlsoActivatesOwningDialog
    ClientUiRuntime.HidingActiveChildClearsKeyboardFocus
    ClientUiRuntime.LoadsLegacyChinaGameInCoreDialogSet
    ClientUiRuntime.LoadsGuildBinAsConcreteGuildDialog
    ClientUiRuntime.LoadsFriendBinAsConcreteFriendDialog
    ClientUiRuntime.LoadsMiniFriendBinAsConcreteDialog
    ClientUiRuntime.LoadsOptionBinAsConcreteOptionDialog
    ClientUiRuntime.ResolutionChangeUpdatesWindowManagerMode
    InGameUiRuntime.OptionOkActivationDispatchesConcreteDialogAction
    InGameUiRuntime.PartyInviteCreatesInteractiveConfirmationModal
    InGameUiRuntime.GuildInviteCreatesInteractiveConfirmationModal
    InGameUiRuntime.FriendInviteCreatesInteractiveConfirmationModal
    InGameUiRuntime.InventoryTabButtonsSwitchRealGridDialog
    InGameUiRuntime.CharacterInfoHudButtonTogglesLiveDialog
    InGameUiRuntime.ChatOptionButtonOpensAndClosesRealOptionDialog
    InGameUiRuntime.EnterAndEscapeOwnRealChatDialog
    InGameUiRuntime.FocusedChatEditboxSynchronizesAndEnterSubmits
    InGameUiRuntime.ChatEditingUsesUtf8CodepointBoundaries
    InGameUiRuntime.InventoryCloseActivatesHandleUiActivation
    InGameUiRuntime.MainBarButtonConsumesClickInActiveDialog
    ClientUiRuntime.ConfirmationOwnsModalInputAndCleansUpAfterEnter
    ClientUiRuntime.MessageBoxClosesOnEscapeAndRunsCallbackOnce
    ClientUiRuntime.MessageBoxCentersInCommittedResolutionMode
    ClientUiRuntime.CharSelectActivateAllMakesRootActiveAndCreateClickable
    ClientUiRuntime.CharMakeActivateAllMakesEveryRootActive
    ClientUiRuntime.CharSelectRootBasicImageBoundWhenSpriteHookRegistered
    ClientUiRuntime.CharSelectSlotThenEnterHitboxesDispatchShippedCommands
    ClientUiRuntime.CharSelectCreateAndEnterHitboxesDispatchShippedCommands
    ClientUiRuntime.CharMakeSubmitCancelAndNameHitboxesDispatchShippedCommands
    InGameUiRuntime.DefaultHudActiveAndMissClickIsNotConsumed
    InGameUiRuntime.IKeyTogglesInventoryOnShippedGameInTree
    InGameUiRuntime.DefaultHudRootsBindNonNullSprites
    ClientUiRuntime.LoginDlgOkAndExitHitboxesDispatchShippedCommands
    ClientUiRuntime.LoginDlgBindsShippedChatMessageText
    ClientUiRuntime.LoginIdAndPasswordHitboxesAcceptTypedText
    CMainTitle.StartLoadsIdDlgAndOkSubmitsCredentials
    CMainTitle.ClearPasswordPreservesAccountAndEditState
    CMainTitle.LoginErrorOpensVisibleRetryMessage
    InGameDisplay.UsesEngineResolutionDuringInitialUiLoad
    CharacterMakeFormModel.UsesResourceDefaultsAndWrapsSelections
    CharMakeOptions.LoadsExactChinaResourceValues
    EffectRuntime.RunsDecodedRealEffectWithObjectContext
    EffectRuntime.RunsAuthoritativeCombatEffectFromSkillList
    EffectRuntime.OverlappingInstancesReceiveDistinctIds
    EffectRuntime.StopObjectRemovesMatchingSourceAndTargetInstances
    EffectRuntime.CapsBurstInstancesWithoutDroppingExistingTimelines
    PROPERTIES LABELS "rog")

# The aggregate runs every case in the binary, including the PlayDH-backed
# ones labeled rog. Label the aggregate rog so `ctest -LE rog` does not
# execute those cases. Each non-PlayDH case stays registered on its own
# and still runs outside rog.
add_test(NAME mxh_client_all_tests
    COMMAND mxh_client_tests --gtest_brief=1)
set_tests_properties(mxh_client_all_tests PROPERTIES
    LABELS "rog"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

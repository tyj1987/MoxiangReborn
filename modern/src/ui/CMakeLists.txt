# mxh_ui: Modern C++ UI framework (Phase 6)
# -----------------------------------------------------------------------------
# Skeleton build. Phase 6.0 ships the framework + unit tests only; the GPU
# render path, control widgets, and integration with mxh_render land in 6.1+.
# -----------------------------------------------------------------------------
add_library(mxh_ui STATIC
    cWindow.cpp
    cButton.cpp
    cEditBox.cpp
    cDialog.cpp
    cImage.cpp
    cListCtrl.cpp
    cWindowManager.cpp
    cMsgBox.cpp
    cDivideBox.cpp
    cIconDialog.cpp
    cIconGridDialog.cpp
    cStatic.cpp
    cPushupButton.cpp
    cListDialog.cpp
    cGuildDialog.cpp
    cExitDialog.cpp
    cMultiLineText.cpp
    cGuagen.cpp
    cListDialogEx.cpp
    cMacroDialog.cpp
    cmakdial.cpp
    guildjoindialog.cpp
    charstatedialog.cpp
    sosdialog.cpp
    wearedexdialog.cpp
    minifrienddialog.cpp
    revivedialog.cpp
    ctextarea.cpp
    mpnoticedialog.cpp
    eventnotifydialog.cpp
    guildcreatedialog.cpp
    chaseinputdialog.cpp
    chasedialog.cpp
    baildialog.cpp
    petwearedexdialog.cpp
    guildnoticedlg.cpp
    chinaadvicedlg.cpp
    introreplaydlg.cpp
    keysettingtipdlg.cpp
    loadingdlg.cpp
    namechangenotifydlg.cpp
    guildinvitationkindselectiondialog.cpp
    tipbrowserdlg.cpp
    guildnicknamedialog.cpp
    shoutdialog.cpp
    guildinvitedialog.cpp
    stallkindselectdlg.cpp
    partyinvitedlg.cpp
    namechangedialog.cpp
    changejobdialog.cpp
    gtregistcanceldialog.cpp
    gtregistdialog.cpp
    reinforcedataguidedlg.cpp
    wanteddialog.cpp
    wantregistdialog.cpp
    maindialog.cpp
    guildmarkdialog.cpp
    mpmissiondialog.cpp
    alertdlg.cpp
    mpguagedialog.cpp
    unionnotedlg.cpp
    autonotedlg.cpp
    mpregistdialog.cpp
    pklootingdialog.cpp
    skinselectdialog.cpp
    ccombobox.cpp
    stallfinddlg.cpp
    cpage.cpp
    cobjectguagen.cpp
    progressbardlg.cpp
    titanmixprogressbardlg.cpp
    titanpartsprogressbardlg.cpp
    uniqueitemmixprogressbardlg.cpp
    titanrecalldlg.cpp
    guildnotedlg.cpp
    pointsavedialog.cpp
    survivalcountdialog.cpp
    debugdlg.cpp
    shoutchatdialog.cpp
    guildlevelupdialog.cpp
    mnchanneldialog.cpp
    fortwartimedialog.cpp
    skillpointnotify.cpp
    cdialoguelist.cpp
    chypertextlist.cpp
    helpdialog.cpp
    ime.cpp
    cspin.cpp
    moneydlg.cpp
    petstateminidlg.cpp
    guagedialog.cpp
    munpamarkdialog.cpp
    titanguagedlg.cpp
    skilloptioncleardlg.cpp
    titanrepairdlg.cpp
    ctabdialog.cpp
    mallnoticedialog.cpp
    mugongsuryundialog.cpp
    ccheckbox.cpp
    cnumberpaddialog.cpp
    cmininotedialog.cpp
    cnotedialog.cpp
    cautoanswerdlg.cpp
    cstatusicondlg.cpp
    cscreenshotdlg.cpp
    coptiondialog.cpp
    cchanneldialog.cpp
    cjournaldialog.cpp
    chelperspeechdlg.cpp
    ccostumeskinselectdialog.cpp
    cchatdialog.cpp
    ccharacterdialog.cpp
)

if(WIN32)
    target_sources(mxh_ui PRIVATE ime_win32_imm.cpp)
    target_link_libraries(mxh_ui PRIVATE imm32)
endif()

target_include_directories(mxh_ui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(mxh_ui PUBLIC cxx_std_20)

if(MSVC)
    target_compile_definitions(mxh_ui PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_ui PRIVATE /FS /MP)
endif()

install(TARGETS mxh_ui ARCHIVE DESTINATION lib)

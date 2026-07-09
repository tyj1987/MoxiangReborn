add_executable(mxh_render_demo main.cpp)
target_link_libraries(mxh_render_demo PRIVATE mxh_render)
if(MSVC)
    target_compile_options(mxh_render_demo PRIVATE /W3 /utf-8)
endif()

# Phase 6.8 headless UI smoke: exercises the modern UI framework
# (cWindow / cButton / cEditBox / cListCtrl / cDialog / cWindowManager)
# end-to-end with a stub render adapter (no GPU). Independent from the
# 3D demo (mxh_render_demo) so it can run on headless hosts.
add_executable(mxh_ui_smoke phase6_smoke.cpp)
target_link_libraries(mxh_ui_smoke PRIVATE mxh_ui)
if(MSVC)
    target_compile_options(mxh_ui_smoke PRIVATE /W3 /utf-8)
endif()

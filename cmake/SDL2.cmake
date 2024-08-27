set (SDL2_SOURCE_DIR  external/SDL/src)
set (SDL2_INCLUDE_DIR external/SDL/include)

set(SDL2_SOURCE_FILES
  ${SDL2_SOURCE_DIR}/SDL_guid.c
  ${SDL2_SOURCE_DIR}/atomic/SDL_atomic.c
  ${SDL2_SOURCE_DIR}/atomic/SDL_spinlock.c
  ${SDL2_SOURCE_DIR}/audio/directsound/SDL_directsound.c
  ${SDL2_SOURCE_DIR}/audio/disk/SDL_diskaudio.c
  ${SDL2_SOURCE_DIR}/audio/dummy/SDL_dummyaudio.c
  ${SDL2_SOURCE_DIR}/audio/SDL_audio.c
  ${SDL2_SOURCE_DIR}/audio/SDL_audiocvt.c
  ${SDL2_SOURCE_DIR}/audio/SDL_audiodev.c
  ${SDL2_SOURCE_DIR}/audio/SDL_audiotypecvt.c
  ${SDL2_SOURCE_DIR}/audio/SDL_mixer.c
  ${SDL2_SOURCE_DIR}/audio/SDL_wave.c
  ${SDL2_SOURCE_DIR}/audio/winmm/SDL_winmm.c
  ${SDL2_SOURCE_DIR}/audio/wasapi/SDL_wasapi.c
  ${SDL2_SOURCE_DIR}/audio/wasapi/SDL_wasapi_win32.c
  ${SDL2_SOURCE_DIR}/core/windows/SDL_hid.c
  ${SDL2_SOURCE_DIR}/core/windows/SDL_immdevice.c
  ${SDL2_SOURCE_DIR}/core/windows/SDL_windows.c
  ${SDL2_SOURCE_DIR}/core/windows/SDL_xinput.c
  ${SDL2_SOURCE_DIR}/cpuinfo/SDL_cpuinfo.c
  ${SDL2_SOURCE_DIR}/dynapi/SDL_dynapi.c
  ${SDL2_SOURCE_DIR}/events/SDL_clipboardevents.c
  ${SDL2_SOURCE_DIR}/events/SDL_displayevents.c
  ${SDL2_SOURCE_DIR}/events/SDL_dropevents.c
  ${SDL2_SOURCE_DIR}/events/SDL_events.c
  ${SDL2_SOURCE_DIR}/events/SDL_gesture.c
  ${SDL2_SOURCE_DIR}/events/SDL_keyboard.c
  ${SDL2_SOURCE_DIR}/events/SDL_mouse.c
  ${SDL2_SOURCE_DIR}/events/SDL_quit.c
  ${SDL2_SOURCE_DIR}/events/SDL_touch.c
  ${SDL2_SOURCE_DIR}/events/SDL_windowevents.c
  ${SDL2_SOURCE_DIR}/file/SDL_rwops.c
  ${SDL2_SOURCE_DIR}/filesystem/windows/SDL_sysfilesystem.c
  ${SDL2_SOURCE_DIR}/haptic/dummy/SDL_syshaptic.c
  ${SDL2_SOURCE_DIR}/haptic/SDL_haptic.c
  ${SDL2_SOURCE_DIR}/haptic/windows/SDL_dinputhaptic.c
  ${SDL2_SOURCE_DIR}/haptic/windows/SDL_windowshaptic.c
  ${SDL2_SOURCE_DIR}/haptic/windows/SDL_xinputhaptic.c
  ${SDL2_SOURCE_DIR}/hidapi/SDL_hidapi.c
  ${SDL2_SOURCE_DIR}/joystick/controller_type.c
  ${SDL2_SOURCE_DIR}/joystick/dummy/SDL_sysjoystick.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapijoystick.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_combined.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_gamecube.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_luna.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_ps3.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_ps4.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_ps5.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_rumble.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_shield.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_stadia.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_steam.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_steamdeck.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_switch.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_wii.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_xbox360.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_xbox360w.c
  ${SDL2_SOURCE_DIR}/joystick/hidapi/SDL_hidapi_xboxone.c
  ${SDL2_SOURCE_DIR}/joystick/SDL_gamecontroller.c
  ${SDL2_SOURCE_DIR}/joystick/SDL_joystick.c
  ${SDL2_SOURCE_DIR}/joystick/SDL_steam_virtual_gamepad.c
  ${SDL2_SOURCE_DIR}/joystick/virtual/SDL_virtualjoystick.c
  ${SDL2_SOURCE_DIR}/joystick/windows/SDL_dinputjoystick.c
  ${SDL2_SOURCE_DIR}/joystick/windows/SDL_rawinputjoystick.c
  ${SDL2_SOURCE_DIR}/joystick/windows/SDL_windowsjoystick.c
  ${SDL2_SOURCE_DIR}/joystick/windows/SDL_windows_gaming_input.c
  ${SDL2_SOURCE_DIR}/joystick/windows/SDL_xinputjoystick.c
  ${SDL2_SOURCE_DIR}/libm/e_atan2.c
  ${SDL2_SOURCE_DIR}/libm/e_exp.c
  ${SDL2_SOURCE_DIR}/libm/e_fmod.c
  ${SDL2_SOURCE_DIR}/libm/e_log.c
  ${SDL2_SOURCE_DIR}/libm/e_log10.c
  ${SDL2_SOURCE_DIR}/libm/e_pow.c
  ${SDL2_SOURCE_DIR}/libm/e_rem_pio2.c
  ${SDL2_SOURCE_DIR}/libm/e_sqrt.c
  ${SDL2_SOURCE_DIR}/libm/k_cos.c
  ${SDL2_SOURCE_DIR}/libm/k_rem_pio2.c
  ${SDL2_SOURCE_DIR}/libm/k_sin.c
  ${SDL2_SOURCE_DIR}/libm/k_tan.c
  ${SDL2_SOURCE_DIR}/libm/s_atan.c
  ${SDL2_SOURCE_DIR}/libm/s_copysign.c
  ${SDL2_SOURCE_DIR}/libm/s_cos.c
  ${SDL2_SOURCE_DIR}/libm/s_fabs.c
  ${SDL2_SOURCE_DIR}/libm/s_floor.c
  ${SDL2_SOURCE_DIR}/libm/s_scalbn.c
  ${SDL2_SOURCE_DIR}/libm/s_sin.c
  ${SDL2_SOURCE_DIR}/libm/s_tan.c
  ${SDL2_SOURCE_DIR}/loadso/windows/SDL_sysloadso.c
  ${SDL2_SOURCE_DIR}/locale/SDL_locale.c
  ${SDL2_SOURCE_DIR}/locale/windows/SDL_syslocale.c
  ${SDL2_SOURCE_DIR}/misc/SDL_url.c
  ${SDL2_SOURCE_DIR}/misc/windows/SDL_sysurl.c
  ${SDL2_SOURCE_DIR}/power/SDL_power.c
  ${SDL2_SOURCE_DIR}/power/windows/SDL_syspower.c
  ${SDL2_SOURCE_DIR}/render/direct3d11/SDL_shaders_d3d11.c
  ${SDL2_SOURCE_DIR}/render/direct3d12/SDL_render_d3d12.c
  ${SDL2_SOURCE_DIR}/render/direct3d12/SDL_shaders_d3d12.c
  ${SDL2_SOURCE_DIR}/render/direct3d/SDL_render_d3d.c
  ${SDL2_SOURCE_DIR}/render/direct3d11/SDL_render_d3d11.c
  ${SDL2_SOURCE_DIR}/render/direct3d/SDL_shaders_d3d.c
  ${SDL2_SOURCE_DIR}/render/opengl/SDL_render_gl.c
  ${SDL2_SOURCE_DIR}/render/opengl/SDL_shaders_gl.c
  ${SDL2_SOURCE_DIR}/render/opengles2/SDL_render_gles2.c
  ${SDL2_SOURCE_DIR}/render/opengles2/SDL_shaders_gles2.c
  ${SDL2_SOURCE_DIR}/render/SDL_d3dmath.c
  ${SDL2_SOURCE_DIR}/render/SDL_render.c
  ${SDL2_SOURCE_DIR}/render/SDL_yuv_sw.c
  ${SDL2_SOURCE_DIR}/render/software/SDL_blendfillrect.c
  ${SDL2_SOURCE_DIR}/render/software/SDL_blendline.c
  ${SDL2_SOURCE_DIR}/render/software/SDL_blendpoint.c
  ${SDL2_SOURCE_DIR}/render/software/SDL_drawline.c
  ${SDL2_SOURCE_DIR}/render/software/SDL_drawpoint.c
  ${SDL2_SOURCE_DIR}/render/software/SDL_render_sw.c
  ${SDL2_SOURCE_DIR}/render/software/SDL_rotate.c
  ${SDL2_SOURCE_DIR}/render/software/SDL_triangle.c
  ${SDL2_SOURCE_DIR}/SDL.c
  ${SDL2_SOURCE_DIR}/SDL_assert.c
  ${SDL2_SOURCE_DIR}/SDL_dataqueue.c
  ${SDL2_SOURCE_DIR}/SDL_list.c
  ${SDL2_SOURCE_DIR}/SDL_error.c
  ${SDL2_SOURCE_DIR}/SDL_hints.c
  ${SDL2_SOURCE_DIR}/SDL_log.c
  ${SDL2_SOURCE_DIR}/SDL_utils.c
  ${SDL2_SOURCE_DIR}/sensor/dummy/SDL_dummysensor.c
  ${SDL2_SOURCE_DIR}/sensor/SDL_sensor.c
  ${SDL2_SOURCE_DIR}/sensor/windows/SDL_windowssensor.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_crc16.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_crc32.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_getenv.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_iconv.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_malloc.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_mslibc.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_qsort.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_stdlib.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_string.c
  ${SDL2_SOURCE_DIR}/stdlib/SDL_strtokr.c
  ${SDL2_SOURCE_DIR}/thread/generic/SDL_syscond.c
  ${SDL2_SOURCE_DIR}/thread/SDL_thread.c
  ${SDL2_SOURCE_DIR}/thread/windows/SDL_syscond_cv.c
  ${SDL2_SOURCE_DIR}/thread/windows/SDL_sysmutex.c
  ${SDL2_SOURCE_DIR}/thread/windows/SDL_syssem.c
  ${SDL2_SOURCE_DIR}/thread/windows/SDL_systhread.c
  ${SDL2_SOURCE_DIR}/thread/windows/SDL_systls.c
  ${SDL2_SOURCE_DIR}/timer/SDL_timer.c
  ${SDL2_SOURCE_DIR}/timer/windows/SDL_systimer.c
  ${SDL2_SOURCE_DIR}/video/dummy/SDL_nullevents.c
  ${SDL2_SOURCE_DIR}/video/dummy/SDL_nullframebuffer.c
  ${SDL2_SOURCE_DIR}/video/dummy/SDL_nullvideo.c
  ${SDL2_SOURCE_DIR}/video/SDL_blit.c
  ${SDL2_SOURCE_DIR}/video/SDL_blit_0.c
  ${SDL2_SOURCE_DIR}/video/SDL_blit_1.c
  ${SDL2_SOURCE_DIR}/video/SDL_blit_A.c
  ${SDL2_SOURCE_DIR}/video/SDL_blit_auto.c
  ${SDL2_SOURCE_DIR}/video/SDL_blit_copy.c
  ${SDL2_SOURCE_DIR}/video/SDL_blit_N.c
  ${SDL2_SOURCE_DIR}/video/SDL_blit_slow.c
  ${SDL2_SOURCE_DIR}/video/SDL_bmp.c
  ${SDL2_SOURCE_DIR}/video/SDL_clipboard.c
  ${SDL2_SOURCE_DIR}/video/SDL_egl.c
  ${SDL2_SOURCE_DIR}/video/SDL_fillrect.c
  ${SDL2_SOURCE_DIR}/video/SDL_pixels.c
  ${SDL2_SOURCE_DIR}/video/SDL_rect.c
  ${SDL2_SOURCE_DIR}/video/SDL_RLEaccel.c
  ${SDL2_SOURCE_DIR}/video/SDL_shape.c
  ${SDL2_SOURCE_DIR}/video/SDL_stretch.c
  ${SDL2_SOURCE_DIR}/video/SDL_surface.c
  ${SDL2_SOURCE_DIR}/video/SDL_video.c
  ${SDL2_SOURCE_DIR}/video/SDL_vulkan_utils.c
  ${SDL2_SOURCE_DIR}/video/SDL_yuv.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsclipboard.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsevents.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsframebuffer.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowskeyboard.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsmessagebox.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsmodes.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsmouse.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsopengl.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsopengles.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsshape.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsvideo.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowsvulkan.c
  ${SDL2_SOURCE_DIR}/video/windows/SDL_windowswindow.c
  ${SDL2_SOURCE_DIR}/video/yuv2rgb/yuv_rgb_lsx.c
  ${SDL2_SOURCE_DIR}/video/yuv2rgb/yuv_rgb_sse.c
  ${SDL2_SOURCE_DIR}/video/yuv2rgb/yuv_rgb_std.c
)

add_library(SDL2 ${SDL2_SOURCE_FILES})

target_include_directories(SDL2
    PRIVATE
        ${SDL2_INCLUDE_DIR}
)

target_compile_definitions(SDL2
    PRIVATE
        DLL_EXPORT
)

# CMakeList.txt: cmake 的 CMake 專案，在此包含來源及定義
# 專案專屬邏輯。
#
project(sdlpal_win32)
cmake_minimum_required(VERSION 3.1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)


add_library(libbinio
  ../external/libbinio/src/binfile.cpp
  ../external/libbinio/src/binio.cpp
  ../external/libbinio/src/binstr.cpp
  ../external/libbinio/src/binwrap.cpp
)
target_include_directories(libbinio PRIVATE ../external/libbinio/contrib/vs2015/libbinio)


add_library(adplug
  ../adplug/emuopls.cpp
  ../adplug/src/fprovide.cpp
  ../adplug/src/player.cpp
  ../adplug/src/rix.cpp
  ../adplug/src/surroundopl.cpp
  ../adplug/dosbox/dbopl.cpp
  ../adplug/dosbox/dosbox_opls.cpp
  ../adplug/dosbox/opl.cpp
  ../adplug/mame/fmopl.cpp
  ../adplug/mame/mame_opls.cpp
  ../adplug/mame/ymf262.cpp
  ../external/nuked/opl3.c
  )
target_include_directories(adplug
  PRIVATE
    ../adplug
    ../external/SDL/include
    ../external/libbinio/src
    ../external/libbinio/contrib/vs2015/libbinio
    ../external/nuked
    )


add_library(libmad
  ../external/libmad/bit.c
  ../external/libmad/decoder.c
  ../external/libmad/fixed.c
  ../external/libmad/frame.c
  ../external/libmad/huffman.c
  ../external/libmad/layer12.c
  ../external/libmad/layer3.c
  ../external/libmad/stream.c
  ../external/libmad/synth.c
  ../external/libmad/timer.c
  )
target_include_directories(libmad PRIVATE ../)
target_compile_definitions(libmad PRIVATE HAVE_CONFIG_H;FPM_DEFAULT;WIN32;_DEBUG;_LIB)

add_library(libogg
  ../external/ogg/src/bitwise.c
  ../external/ogg/src/framing.c
  )
target_include_directories(libogg PRIVATE ../external/ogg/include)


add_library(libvorbis
  ../external/vorbis/lib/analysis.c
  ../external/vorbis/lib/bitrate.c
  ../external/vorbis/lib/block.c
  ../external/vorbis/lib/codebook.c
  ../external/vorbis/lib/envelope.c
  ../external/vorbis/lib/floor0.c
  ../external/vorbis/lib/floor1.c
  ../external/vorbis/lib/info.c
  ../external/vorbis/lib/lookup.c
  ../external/vorbis/lib/lpc.c
  ../external/vorbis/lib/lsp.c
  ../external/vorbis/lib/mapping0.c
  ../external/vorbis/lib/mdct.c
  ../external/vorbis/lib/psy.c
  ../external/vorbis/lib/registry.c
  ../external/vorbis/lib/res0.c
  ../external/vorbis/lib/sharedbook.c
  ../external/vorbis/lib/smallft.c
  ../external/vorbis/lib/synthesis.c
  ../external/vorbis/lib/vorbisenc.c
  ../external/vorbis/lib/window.c
  )
target_include_directories(libvorbis PRIVATE ../external/vorbis/include;../external/ogg/include)


add_library(native_midi
  ../external/SDL_mixer/src/codecs/native_midi/native_midi_common.c
  ../external/SDL_mixer/src/codecs/native_midi/native_midi_win32.c
)
target_include_directories(native_midi PRIVATE ../external/SDL/include;../external/SDL_mixer/include;../external/SDL_mixer/src/codecs/native_midi;)


add_library(opus
  ../external/opus/celt/bands.c
  ../external/opus/celt/celt.c
  ../external/opus/celt/celt_decoder.c
  ../external/opus/celt/celt_encoder.c
  ../external/opus/celt/celt_lpc.c
  ../external/opus/celt/cwrs.c
  ../external/opus/celt/entcode.c
  ../external/opus/celt/entdec.c
  ../external/opus/celt/entenc.c
  ../external/opus/celt/kiss_fft.c
  ../external/opus/celt/laplace.c
  ../external/opus/celt/mathops.c
  ../external/opus/celt/mdct.c
  ../external/opus/celt/modes.c
  ../external/opus/celt/pitch.c
  ../external/opus/celt/quant_bands.c
  ../external/opus/celt/rate.c
  ../external/opus/celt/vq.c
  ../external/opus/celt/x86/celt_lpc_sse4_1.c
  ../external/opus/celt/x86/pitch_sse.c
  ../external/opus/celt/x86/pitch_sse2.c
  ../external/opus/celt/x86/pitch_sse4_1.c
  ../external/opus/celt/x86/vq_sse2.c
  ../external/opus/celt/x86/x86cpu.c
  ../external/opus/celt/x86/x86_celt_map.c
  ../external/opus/silk/A2NLSF.c
  ../external/opus/silk/ana_filt_bank_1.c
  ../external/opus/silk/biquad_alt.c
  ../external/opus/silk/bwexpander.c
  ../external/opus/silk/bwexpander_32.c
  ../external/opus/silk/check_control_input.c
  ../external/opus/silk/CNG.c
  ../external/opus/silk/code_signs.c
  ../external/opus/silk/control_audio_bandwidth.c
  ../external/opus/silk/control_codec.c
  ../external/opus/silk/control_SNR.c
  ../external/opus/silk/debug.c
  ../external/opus/silk/decoder_set_fs.c
  ../external/opus/silk/decode_core.c
  ../external/opus/silk/decode_frame.c
  ../external/opus/silk/decode_indices.c
  ../external/opus/silk/decode_parameters.c
  ../external/opus/silk/decode_pitch.c
  ../external/opus/silk/decode_pulses.c
  ../external/opus/silk/dec_API.c
  ../external/opus/silk/encode_indices.c
  ../external/opus/silk/encode_pulses.c
  ../external/opus/silk/enc_API.c
  ../external/opus/silk/gain_quant.c
  ../external/opus/silk/HP_variable_cutoff.c
  ../external/opus/silk/init_decoder.c
  ../external/opus/silk/init_encoder.c
  ../external/opus/silk/inner_prod_aligned.c
  ../external/opus/silk/interpolate.c
  ../external/opus/silk/lin2log.c
  ../external/opus/silk/log2lin.c
  ../external/opus/silk/LPC_analysis_filter.c
  ../external/opus/silk/LPC_fit.c
  ../external/opus/silk/LPC_inv_pred_gain.c
  ../external/opus/silk/LP_variable_cutoff.c
  ../external/opus/silk/NLSF2A.c
  ../external/opus/silk/NLSF_decode.c
  ../external/opus/silk/NLSF_del_dec_quant.c
  ../external/opus/silk/NLSF_encode.c
  ../external/opus/silk/NLSF_stabilize.c
  ../external/opus/silk/NLSF_unpack.c
  ../external/opus/silk/NLSF_VQ.c
  ../external/opus/silk/NLSF_VQ_weights_laroia.c
  ../external/opus/silk/NSQ.c
  ../external/opus/silk/NSQ_del_dec.c
  ../external/opus/silk/pitch_est_tables.c
  ../external/opus/silk/PLC.c
  ../external/opus/silk/process_NLSFs.c
  ../external/opus/silk/quant_LTP_gains.c
  ../external/opus/silk/resampler.c
  ../external/opus/silk/resampler_down2.c
  ../external/opus/silk/resampler_down2_3.c
  ../external/opus/silk/resampler_private_AR2.c
  ../external/opus/silk/resampler_private_down_FIR.c
  ../external/opus/silk/resampler_private_IIR_FIR.c
  ../external/opus/silk/resampler_private_up2_HQ.c
  ../external/opus/silk/resampler_rom.c
  ../external/opus/silk/shell_coder.c
  ../external/opus/silk/sigm_Q15.c
  ../external/opus/silk/sort.c
  ../external/opus/silk/stereo_decode_pred.c
  ../external/opus/silk/stereo_encode_pred.c
  ../external/opus/silk/stereo_find_predictor.c
  ../external/opus/silk/stereo_LR_to_MS.c
  ../external/opus/silk/stereo_MS_to_LR.c
  ../external/opus/silk/stereo_quant_pred.c
  ../external/opus/silk/sum_sqr_shift.c
  ../external/opus/silk/tables_gain.c
  ../external/opus/silk/tables_LTP.c
  ../external/opus/silk/tables_NLSF_CB_NB_MB.c
  ../external/opus/silk/tables_NLSF_CB_WB.c
  ../external/opus/silk/tables_other.c
  ../external/opus/silk/tables_pitch_lag.c
  ../external/opus/silk/tables_pulses_per_block.c
  ../external/opus/silk/table_LSF_cos.c
  ../external/opus/silk/VAD.c
  ../external/opus/silk/VQ_WMat_EC.c
  ../external/opus/silk/x86/NSQ_del_dec_sse4_1.c
  ../external/opus/silk/x86/NSQ_sse4_1.c
  ../external/opus/silk/x86/VAD_sse4_1.c
  ../external/opus/silk/x86/VQ_WMat_EC_sse4_1.c
  ../external/opus/silk/x86/x86_silk_map.c
  ../external/opus/src/analysis.c
  ../external/opus/src/mapping_matrix.c
  ../external/opus/src/mlp.c
  ../external/opus/src/mlp_data.c
  ../external/opus/src/opus.c
  ../external/opus/src/opus_compare.c
  ../external/opus/src/opus_decoder.c
  ../external/opus/src/opus_encoder.c
  ../external/opus/src/opus_multistream.c
  ../external/opus/src/opus_multistream_decoder.c
  ../external/opus/src/opus_multistream_encoder.c
  ../external/opus/src/opus_projection_decoder.c
  ../external/opus/src/opus_projection_encoder.c
  ../external/opus/src/repacketizer.c
)
target_include_directories(opus PRIVATE ../external/opus;../external/opus/include;../external/opus/celt;../external/opus/silk;../external/opus/silk/float;../external/opus/win32)
target_compile_definitions(opus PRIVATE HAVE_CONFIG_H)


add_library(opusfile
  ../external/opusfile/src/http.c
  ../external/opusfile/src/info.c
  ../external/opusfile/src/internal.c
  ../external/opusfile/src/opusfile.c
  ../external/opusfile/src/stream.c
  ../external/opusfile/src/wincerts.c
)
target_include_directories(opusfile PRIVATE ../external/opus/include;../external/opusfile/include;../external/ogg/include)


# 將來源新增到此專案的可執行檔。
message("CMAKE_CURRENT_SOURCE_DIR:${CMAKE_CURRENT_SOURCE_DIR}")
add_executable (${PROJECT_NAME} win32.cpp)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Debug)
target_link_libraries(${PROJECT_NAME} 
  opengl32
  imm32
  winmm
  comctl32
  setupapi
  version
  sdl2
  sdl2main
  native_midi
  libvorbis
  libogg
  timidity
  opus
  opusfile
  libmad
  libpal
  libbinio
  adplug
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ../sdlpal/include
    ../external/SDL/include
    ../adplug
    )

# TODO: 新增測試，並視需要安裝目標。

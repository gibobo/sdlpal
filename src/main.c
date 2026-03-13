//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "main.h"
#include "audio.h"
#include "driver.h"
#include "ending.h"
#include "font.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "palette.h"
#include "play.h"
#include "res.h"
#include "resource.h"
#include "rngplay.h"
#include "scene.h"
#include "text.h"
#include "ui.h"
#include "uigame.h"
#include "util.h"
#include "video.h"
#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>

static jmp_buf g_exit_jmp_buf;
static int g_exit_code = 0;

void PAL_Init(void)
/*++
  Purpose:

    Initialize everything needed by the game.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    printf("Initializing game...\n");
    // PAL_ConsolidateExtractedResources();
    if (PAL_LoadConsolidatedResources() != 0)
        TerminateOnError("%s() failed: load consolidated resources error\n", __func__);
    fprintf(stdout, "==> Consolidated resources loaded successfully.\n");

    // Initialize subsystems.
    if (DRIVER_Init() != 0)
        TerminateOnError("%s() failed: driver initialization error\n", __func__);
    fprintf(stdout, "==> Driver initialized successfully.\n");

    if (PAL_InitGlobals() != 0)
        TerminateOnError("%s() failed: global data initialization error\n", __func__);
    fprintf(stdout, "==> Global data initialized successfully.\n");

    if (PAL_InitUI() != 0)
        TerminateOnError("%s() failed: UI subsystem initialization error\n", __func__);
    fprintf(stdout, "==> UI subsystem initialized successfully.\n");

    if (PAL_InitText() != 0)
        TerminateOnError("%s() failed: text subsystem initialization error\n", __func__);
    fprintf(stdout, "==> Text subsystem initialized successfully.\n");

    PAL_InitFont();
    fprintf(stdout, "==> Font subsystem initialized successfully.\n");

    if (PAL_InitResources() != 0)
        TerminateOnError("%s() failed: resource manager initialization error\n", __func__);
    fprintf(stdout, "==> Resource manager initialized successfully.\n");

    if (VIDEO_Startup() != 0)
        TerminateOnError("%s() failed: video subsystem initialization error\n", __func__);
    fprintf(stdout, "==> Video subsystem initialized successfully.\n");

    if (AUDIO_Startup() != 0)
    {
        // Audio initialization failure is not fatal - game can run without sound
        fprintf(stdout, "==> Warning: %s() failed: audio subsystem initialization error\n", __func__);
        fprintf(stdout, "==> Game will continue without audio.\n");
    }
    fprintf(stdout, "==> Audio subsystem initialized successfully.\n");

    PAL_InitInput();
    fprintf(stdout, "==> Input subsystem initialized successfully.\n");

    fprintf(stdout, "Initialization completed successfully.\n");
}

void PAL_Shutdown(int exit_code)
/*++
  Purpose:

    Free everything needed by the game.

  Parameters:

    exit_code -  The exit code return to OS.

  Return value:

    None.

--*/
{
    PAL_FreeResources();
    PAL_FreeUI();
    PAL_FreeText();
    PAL_ShutdownInput();
    AUDIO_CloseDevice();
    VIDEO_Shutdown();
    DRIVER_DeInit();

    // global needs be free in last
    // since subsystems may needs config content during destroy
    // which also cleared here
    PAL_DeInitFont();
    PAL_FreeGlobals();
    PAL_FreeResourceIndex();
    PAL_FreeSceneResources();

    g_exit_code = exit_code;
    longjmp(g_exit_jmp_buf, 1);
}

void PAL_TrademarkScreen(void)
/*++
  Purpose:

    Show the trademark screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    PAL_SetPalette(3, false);
    PAL_RNGPlay(6, 0, -1, 25);
    UTIL_Delay(1000);
    PAL_FadeOut(1);
}

void PAL_MusicPlayer(void)
/*++
  Purpose:

    Interactive music player - allows user to select and play music tracks.
    Enter 0 to exit.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    if (PAL_LoadConsolidatedResources() != 0)
        TerminateOnError("%s() failed: load consolidated resources error\n", __func__);

    if (DRIVER_Init() != 0)
        TerminateOnError("%s() failed: driver initialization error\n", __func__);

    if (AUDIO_Startup() != 0)
        TerminateOnError("%s() failed: audio subsystem initialization error\n", __func__);

    const char *anim = "|/-\\";
    while (1)
    {
        int32_t animIdx = 0;
        int32_t musicNum = 0;
        printf("Enter music number (1-87, 0 to exit): ");
        fflush(stdout);

        if (scanf("%d", &musicNum) != 1)
        {
            // Clear invalid input
            int32_t c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            printf("Invalid input! Please enter a number.\n");
            continue;
        }

        // Clear the input buffer
        int32_t c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;

        // Exit if user enters 0
        if (musicNum == 0)
        {
            break;
        }

        // Validate music number
        if (musicNum < 0 || musicNum > 87)
        {
            printf("Invalid music number! Please enter 0-87.\n");
            continue;
        }

        // Play the selected music
        AUDIO_PlayMusic(musicNum, false, 0.0);

        uint32_t startTime = UTIL_GetMilliseconds();
        uint32_t lastUpdateTime = startTime;
        int32_t lastMusicNum = musicNum;

        // Playback monitoring loop
        while (1)
        {
            UTIL_Delay(100);
            int32_t currentMusic = AUDIO_GetCurrentMusic();
            uint32_t currentTime = UTIL_GetMilliseconds();
            uint32_t elapsed = (currentTime - startTime) / 1000;

            // Check if music has stopped (music number changed to -1 or different number)
            if (currentMusic != lastMusicNum)
            {
                if (currentMusic == -1)
                {
                    printf("\rMusic playback completed. Duration: %02lu:%02lu          \n",
                           elapsed / 60, elapsed % 60);
                    break;
                }
                lastMusicNum = currentMusic;
            }

            // Update progress every 500ms
            if (currentTime - lastUpdateTime >= 500)
            {
                uint32_t minutes = elapsed / 60;
                uint32_t seconds = elapsed % 60;

                // Simple animated progress indicator

                printf("\rPlaying #%d [%c] Time: %02lu:%02lu     ",
                       lastMusicNum, anim[animIdx % 4], minutes, seconds);
                fflush(stdout);
                animIdx++;
                lastUpdateTime = currentTime;
            }
        }
    }
    printf("\nExiting music player...\n");
    AUDIO_CloseDevice();
    DRIVER_DeInit();
    PAL_FreeResourceIndex();
}

void PAL_SplashScreen(void)
/*++
  Purpose:

    Show the splash screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    uint8_t *palette = PAL_GetPalette(1, false);
    uint8_t rgCurrentPalette[PALETTE_SIZE];
    VIDEO_Surface *lpBitmapUp = VIDEO_GetBackupSurface(0);
    VIDEO_Surface *lpBitmapDown = VIDEO_GetBackupSurface(1);
    VIDEO_Rect srcrect;
    VIDEO_Rect dstrect;
    uint8_t *lpTitleBuf = NULL;
    uint8_t *lpSpriteCrane = NULL;
    uint8_t *lpBitmapTitle = NULL;
    const uint8_t crane_num = 9;
    int32_t *cranepos = NULL;
    uint32_t i;
    uint16_t iImgPos = SCREEN_H;
    uint16_t iTitleHeight;
    uint32_t dwTime = 0;

    if (palette == NULL)
    {
        TerminateOnError("%s() failed: palette data is NULL (unable to load palette)\n", __func__);
        return;
    }

    // Read the bitmaps
    RES_MKFDecompressChunk((void **)&lpBitmapUp->pixels, SCREEN_SIZE, 0x03, Res_FBP);
    RES_MKFDecompressChunk((void **)&lpBitmapDown->pixels, SCREEN_SIZE, 0x04, Res_FBP);
    RES_MKFDecompressChunk((void **)&lpTitleBuf, 0, 0x47, Res_MGO);
    RES_MKFDecompressChunk((void **)&lpSpriteCrane, 0, 0x49, Res_MGO);

    lpBitmapTitle = (uint8_t *)PAL_SpriteGetFrame(lpTitleBuf, 0);
    iTitleHeight = lpBitmapTitle[2] | ((uint16_t)lpBitmapTitle[3] << 8);
    lpBitmapTitle[2] = 0;
    lpBitmapTitle[3] = 0; // HACKHACK

    // Generate the positions of the cranes
    cranepos = (int32_t *)UTIL_calloc(crane_num * 3, sizeof(int32_t));
    for (i = 0; i < crane_num; i++)
    {
        cranepos[i * 3 + 0] = RandomLong(300, 600);
        cranepos[i * 3 + 1] = RandomLong(0, 80);
        cranepos[i * 3 + 2] = RandomLong(0, 8);
    }

    // Play the title music
    AUDIO_PlayMusic(-1, false, 0);
    AUDIO_PlayMusic(0x05, true, 2);

    // Clear all of the events and key states
    PAL_ClearKeyState();

    srcrect.x = 0;
    srcrect.w = SCREEN_W;
    dstrect.x = 0;
    dstrect.w = SCREEN_W;

    while (true)
    {
        dwTime++;
        // Set the palette
        if (dwTime <= iTitleHeight)
        {
            for (i = 0; i < PALETTE_SIZE; i++)
                rgCurrentPalette[i] = (uint8_t)(palette[i] * dwTime / iTitleHeight);
            DRIVER_UpdatePalette(rgCurrentPalette);
        }

        // Draw the screen
        if (iImgPos)
            iImgPos--;

        // The lower part...
        srcrect.y = 0;
        dstrect.y = SCREEN_H - iImgPos;
        srcrect.h = (short)iImgPos;
        dstrect.h = srcrect.h;
        if (srcrect.h && dstrect.h)
            VIDEO_CopySurface(lpBitmapDown, &srcrect, gpScreen, &dstrect);
        // The upper part...
        srcrect.y = iImgPos;
        dstrect.y = 0;
        srcrect.h = SCREEN_H - iImgPos;
        dstrect.h = srcrect.h;
        if (srcrect.h && dstrect.h)
            VIDEO_CopySurface(lpBitmapUp, &srcrect, gpScreen, &dstrect);

        // Draw the cranes...
        for (i = 0; i < crane_num; i++)
        {
            if (cranepos[i * 3 + 0] > -35)
            {
                const uint8_t *lpFrame = PAL_SpriteGetFrame(lpSpriteCrane, cranepos[i * 3 + 2]);
                PAL_RLEBlitToSurface(lpFrame, gpScreen, PAL_XY(cranepos[i * 3 + 0], cranepos[i * 3 + 1]));
                cranepos[i * 3 + 0]--;
                cranepos[i * 3 + 1] += (iImgPos & 1) ? 1 : 0;
                cranepos[i * 3 + 2] += (dwTime & 1);
                cranepos[i * 3 + 2] %= 8;
            }
        }

        // Draw the title...
        if (dwTime < iTitleHeight)
        {
            lpBitmapTitle[2] = (dwTime & 0xFF);
            lpBitmapTitle[3] = (dwTime >> 8) & 0xFF;
        }

        PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));
        VIDEO_UpdateScreen(NULL);

        // Check for keypress...
        if (UTIL_WaitKeys(85, kKeyMenu | kKeySearch))
            break;
    }
    // Quit the splash screen
    lpBitmapTitle[2] = (uint8_t)(iTitleHeight & 0xFF);
    lpBitmapTitle[3] = (uint8_t)((iTitleHeight >> 8) & 0xFF); // HACKHACK
    PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));

    // If the picture has not completed fading in, complete the rest
    while (dwTime < iTitleHeight)
    {
        for (i = 0; i < PALETTE_SIZE; i++)
            rgCurrentPalette[i] = (uint8_t)(palette[i] * dwTime / iTitleHeight);
        DRIVER_UpdatePalette(rgCurrentPalette);
        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(8);
        dwTime += 4;
    }
    DRIVER_UpdatePalette(palette);
    VIDEO_UpdateScreen(NULL);

    UTIL_free(cranepos);
    UTIL_free(lpTitleBuf);
    UTIL_free(lpSpriteCrane);
    UTIL_Delay(500);

    AUDIO_PlayMusic(0x00, false, 1);
    PAL_FadeOut(1);
}

int main(int argc, char *argv[])
/*++
  Purpose:

    Program entry.

  Parameters:

    argc - Number of arguments.

    argv - Array of arguments.

  Return value:

    Integer value.

--*/
{
    (void)argc; // Unused parameter
    (void)argv; // Unused parameter

    if (setjmp(g_exit_jmp_buf) != 0)
    {
        // A longjmp is made, should exit here
        return g_exit_code;
    }

    // Run interactive music player
    // PAL_MusicPlayer();

    // Initialize everything
    PAL_Init();

    // Show the trademark screen and splash screen
    // PAL_EndingScreen();
    PAL_TrademarkScreen();
    PAL_SplashScreen();

    // Show the opening menu.
    PAL_OpeningMenu();

    // Run the main game routine
    while (1)
    {
        // Run the main frame routine.
        PAL_StartFrame();
    }

    // Should not really reach here...
    assert(false);
    return 255;
}

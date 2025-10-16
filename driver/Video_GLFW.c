#include "../src/driver.h"
#include "../src/util.h"
#include "../src/video.h"
#include "DrvIf_internal.h"
#include "utils/video_glsl.h"
#include <GLFW/glfw3.h>
#include <string.h>

static int window_width = 320;
static int window_height = 200;
static unsigned char *framebuffer = NULL; // RGB888
static const unsigned char *palette = NULL;
GLFWwindow *window = NULL;

extern void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag)
{
    if (window == NULL || framebuffer == NULL || palette == NULL)
        return;

    unsigned short x, y;
    unsigned short roi_x2 = roi_x + roi_w;
    unsigned short roi_y2 = roi_y + roi_h;
    unsigned char *src = frame;
    unsigned char *dst = framebuffer;

    for (y = 0; y < SCREEN_H; y++)
    {
        if ((y >= roi_y) && (y < roi_y2))
        {
            for (x = 0; x < SCREEN_W; x++)
            {
                if ((x >= roi_x) && (x < roi_x2))
                {
                    dst[x * 3 + 0] = palette[src[x] * 3 + 0];
                    dst[x * 3 + 1] = palette[src[x] * 3 + 1];
                    dst[x * 3 + 2] = palette[src[x] * 3 + 2];
                }
                else if (padding_flag)
                {
                    dst[x * 3 + 0] = 0;
                    dst[x * 3 + 1] = 0;
                    dst[x * 3 + 2] = 0;
                }
            }
            src += SCREEN_W;
        }
        else if (padding_flag)
            memset(dst, 0, SCREEN_W * 3);
        else
            src += SCREEN_W;
        dst += SCREEN_W * 3;
    }

    // Try to use OpenGL rendering if available, otherwise use software fallback
    if (glfwGetCurrentContext() != NULL)
    {
        VIDEO_GLSL_RenderCopy(framebuffer);
        glfwSwapBuffers(window);
    }
    else
    {
        // Software fallback - just poll events to keep window responsive
        glfwPollEvents();
    }
}

void DRIVER_UpdatePalette(const unsigned char *rgPalette) { palette = rgPalette; }

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    (void)window;
    window_width = width;
    window_height = height;
    // Only initialize OpenGL if we have a valid context
    if (glfwGetCurrentContext() != NULL)
    {
        VIDEO_GLSL_Initialize(window_width, window_height);
    }
}

int DRIVER_Init_Video(void)
{
    // Verify GLFW is initialized
    if (!glfwGetVersionString())
    {
        return -1;
    }

    // Try OpenGL ES first for better compatibility
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    /* Create a windowed mode window with OpenGL context */
    window = glfwCreateWindow(window_width, window_height, "GLFWPAL", NULL, NULL);

    if (window == NULL)
    {
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    VIDEO_GLSL_Initialize(window_width, window_height);

    glfwGetWindowSize(window, &window_width, &window_height);
    framebuffer = (unsigned char *)UTIL_malloc(SCREEN_SIZE * 3);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    return 0;
}

void DRIVER_DeInit_Video(void)
{
    // Only destroy OpenGL resources if we have a valid context
    if (glfwGetCurrentContext() != NULL)
    {
        VIDEO_GLSL_Destroy();
    }
    UTIL_free(framebuffer);
    glfwTerminate();
    window = NULL;
    framebuffer = NULL;
}

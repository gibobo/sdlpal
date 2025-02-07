#include "video.h"
#include "video_glsl.h"
#include <GLFW/glfw3.h>

static int window_width = 320;
static int window_height = 200;
GLFWwindow *window = NULL;

void DRIVER_FrameShow(unsigned char *frame_rgb) {
    VIDEO_GLSL_RenderCopy(frame_rgb);
    glfwSwapBuffers(window);
    /* Poll for and process events */
    glfwPollEvents();
}

void DRIVER_FrameResize(unsigned int width, unsigned int height) {
    window_width = width;
    window_height = height;
    VIDEO_GLSL_Setup(window_width, window_height);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    DRIVER_FrameResize(width, height);
}

int DRIVER_Init_Video(void) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(window_width, window_height, "Shadertoy", NULL, NULL);
    if (window == NULL)
        return -1;

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwGetWindowSize(window, &window_width, &window_height);
    VIDEO_GLSL_Setup(window_width, window_height);
    return 0;
}

void DRIVER_DeInit_Video(void) {
    glfwTerminate();
}
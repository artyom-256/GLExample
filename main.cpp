#include "glwindow.h"
#include "object.h"

#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 32);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    {
        GLWindow window(800, 600, "GLTest");

        while (!window.shouldBeClosed())
        {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            window.processInput();
            window.render();

            glfwPollEvents();
        }
    }
    glfwTerminate();

    return 0;
}

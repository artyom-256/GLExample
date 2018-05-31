#include "glapp.h"
#include "glwindow.h"

int main()
{
    GLApp app;
    auto w = std::make_shared< GLWindow >(800, 600, "GLTest");
    app.registerWindow(w);
    app.run();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
//void processInput(GLFWwindow *window)
//{
//    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
//        objectX -= .02;
//    } else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
//        objectX += .02;
//    } else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
//        objectY += .02;
//    } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
//        objectY -= .02;
//    } else if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
//        objectZ += .02;
//    } else if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
//        objectZ -= .02;
//    }
//}

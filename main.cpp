#include "object.h"
#include "bitmap_image.h"

#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * Default window width.
 */
const int WINDOW_WIDTH = 800;
/**
 * Default window height.
 */
const int WINDOW_HEIGHT = 800;
/**
 * If the shader compilation fails, the error message will be printed out.
 * The constant defines maximal size of the error message buffer.
 */
const int ERROR_BUFFER_SIZE = 2048;

/**
 * Structure attached to the window object.
 */
struct window_user_struct
{
    int width;
    int height;
};

/**
 * Callback for window size changed event.
 * @param window Window that has triggered the event.
 * @param width New width.
 * @param height New height.
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Update size in the window struct object to use the new values during rendering.
    window_user_struct* windowStruct = reinterpret_cast< window_user_struct* >(glfwGetWindowUserPointer(window));
    windowStruct->width = width;
    windowStruct->height = height;
}

int main()
{
    // Initalize GLFW.
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 32);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Define structure that will be attached to the window object.
    window_user_struct windowStruct = { WINDOW_WIDTH, WINDOW_HEIGHT };
    // Create a window.
    GLFWwindow* window = glfwCreateWindow(windowStruct.width, windowStruct.height, "GLExample", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create a window!" << std::endl;
        abort();
    }
    // Attach the structure to the window so we can always extract it in GLFW callbacks.
    glfwSetWindowUserPointer(window, &windowStruct);
    // Attach window resize listener.
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // Select OpenGL context.
    glfwMakeContextCurrent(window);

    // Initialize OpenGL.
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize OpenGL!" << std::endl;
        abort();
    }

    // Load and compile a vertex shader.
    std::ifstream vertexShaderStream("main.vs");
    if (!vertexShaderStream) {
        std::cout << "Failed to read the fragment shader file!" << std::endl;
        abort();
    }
    std::string vertexShaderCode((std::istreambuf_iterator< char >(vertexShaderStream)), std::istreambuf_iterator< char >());
    auto vertexShaderCodeStr = vertexShaderCode.c_str();
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCodeStr, NULL);
    glCompileShader(vertexShader);
    // Check for shader compilation errors.
    int vertexShaderSuccess;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexShaderSuccess);
    if (!vertexShaderSuccess) {
        char infoLog[ERROR_BUFFER_SIZE];
        glGetShaderInfoLog(vertexShader, ERROR_BUFFER_SIZE, NULL, infoLog);
        std::cout << "Vertex shader compilation failed:" << std::endl << infoLog << std::endl;
        abort();
    }

    // Load and compile a fragment shader.
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::ifstream fragmentShaderStream("main.fs");
    if (!vertexShaderStream) {
        std::cout << "Failed to read the fragment shader file!" << std::endl;
        abort();
    }
    std::string fragmentShaderCode((std::istreambuf_iterator< char >(fragmentShaderStream)), std::istreambuf_iterator< char >());
    auto fragmentShaderCodeStr = fragmentShaderCode.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderCodeStr, NULL);
    glCompileShader(fragmentShader);
    // Check for shader compilation errors.
    int fragmentShaderSuccess;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentShaderSuccess);
    if (!fragmentShaderSuccess) {
        char infoLog[ERROR_BUFFER_SIZE];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment shader compilation failed:" << std::endl << infoLog << std::endl;
        abort();
    }

    // Link shaders into a shader program.
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);
    // check for linking errors
    int programSuccess;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &programSuccess);
    if (!programSuccess) {
        char infoLog[ERROR_BUFFER_SIZE];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Program linking failed:" << std::endl << infoLog << std::endl;
        abort();
    }

    // Delete shaders after the program is linked.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Read a 3D object.
    object obj("box.obj");
    if (obj.vertexes().size() == 0) {
        std::cout << "Cannot read a 3D model from the file!" << std::endl;
        abort();
    }

    // Create a vertex array object that is a collection of attribute buffers describing each vertex.
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create a buffer that contains vertex coordinates.
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, obj.vertexes().size() * sizeof(glm::vec3), obj.vertexes().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create a buffer that contains texture coordinates per vertex.
    GLuint uvBuffer;
    glGenBuffers(1, &uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, obj.uvs().size() * sizeof(glm::vec2), obj.uvs().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create a buffer that contains normals per vertex.
    GLuint normalBuffer;
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, obj.normals().size() * sizeof(glm::vec3), obj.normals().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create a buffer that contains tangents per vertex.
    GLuint tangentBuffer;
    glGenBuffers(1, &tangentBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
    glBufferData(GL_ARRAY_BUFFER, obj.tangents().size() * sizeof(glm::vec3), &obj.tangents()[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create a buffer that contains bitangents per vertex.
    GLuint bitangentBuffer;
    glGenBuffers(1, &bitangentBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, bitangentBuffer);
    glBufferData(GL_ARRAY_BUFFER, obj.tangents().size() * sizeof(glm::vec3), &obj.bitangents()[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind vertex attribute array to not accidentaly make changes to it.
    glBindVertexArray(0);

    // Read an image texure file.
    bitmap_image imageTexture("texture.bmp");
    if (imageTexture.data().size() == 0) {
        std::cout << "Failed to open image texture!" << std::endl;
        abort();
    }
    // Allocate the texture.
    glActiveTexture(GL_TEXTURE0);
    GLuint colorTextureId;
    glGenTextures(1, &colorTextureId);
    glBindTexture(GL_TEXTURE_2D, colorTextureId);
    // Write image data to the texture.
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, imageTexture.width(), imageTexture.height(), 0, GL_BGR, GL_UNSIGNED_BYTE, &imageTexture.data()[0]);
    // Set texture scaling parameters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // Generate a mipmap for scaling.
    glGenerateMipmap(GL_TEXTURE_2D);;

    // Read a normal texure file.
    bitmap_image normalTexture("normal.bmp");
    if (normalTexture.data().size() == 0) {
        std::cout << "Failed to open normal texture!" << std::endl;
        abort();
    }
    // Allocate the texture.
    glActiveTexture(GL_TEXTURE1);
    GLuint normalTextureId;
    glGenTextures(1, &normalTextureId);
    glBindTexture(GL_TEXTURE_2D, normalTextureId);
    // Write image data to the texture.
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, normalTexture.width(), normalTexture.height(), 0, GL_BGR, GL_UNSIGNED_BYTE, &normalTexture.data()[0]);
    // Set texture scaling parameters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // Generate a mipmap for scaling.
    glGenerateMipmap(GL_TEXTURE_2D);

    // Read a displacement texure file.
    bitmap_image displacementTexture("displacement.bmp");
    if (displacementTexture.data().size() == 0) {
        std::cout << "Failed to open displacement texture!" << std::endl;
        abort();
    }
    // Allocate the texture.
    glActiveTexture(GL_TEXTURE2);
    GLuint displacementTextureId;
    glGenTextures(1, &displacementTextureId);
    glBindTexture(GL_TEXTURE_2D, displacementTextureId);
    // Write image data to the texture.
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, displacementTexture.width(), displacementTexture.height(), 0, GL_BGR, GL_UNSIGNED_BYTE, &displacementTexture.data()[0]);
    // Set texture scaling parameters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // Generate a mipmap for scaling.
    glGenerateMipmap(GL_TEXTURE_2D);

    // Enable depth test and removing of back-facing triangles.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    // Create a camera matrix.
    glm::vec3 cameraPosition(0, 0, -7.0);
    glm::mat4 cameraMatrix = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
    cameraMatrix = glm::translate(cameraMatrix, -cameraPosition);

    // Define light source.
    glm::vec3 lightPosition(0, 0, -10.0);

    // Render loop.
    while (!glfwWindowShouldClose(window))
    {
        // Clean up color and depth buffers.
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Select the shader program.
        glUseProgram(shaderProgram);

        // Create a model matrix that combines rotations around x and y axis with different speed.
        float time = float(glfwGetTime()) / 2;
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix *= glm::rotate(glm::mat4(1.0f), time, glm::vec3(0, 1, 0));
        modelMatrix *= glm::rotate(glm::mat4(1.0f), time / 2, glm::vec3(1, 0, 0));

        // Create a projection matrix.
        float aspectRatio = float(windowStruct.width) / windowStruct.height;
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(30.0f), aspectRatio, 0.1f, 100.0f);

        // Add uniform values for matrices and positions.
        GLint modelMatrixUniform = glGetUniformLocation(shaderProgram, "modelMatrix");
        glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        GLint cameraMatrixUniform = glGetUniformLocation(shaderProgram, "cameraMatrix");
        glUniformMatrix4fv(cameraMatrixUniform, 1, GL_FALSE, glm::value_ptr(cameraMatrix));
        GLint projectionMatrixUniform = glGetUniformLocation(shaderProgram, "projectionMatrix");
        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        GLint lightPositionUniform = glGetUniformLocation(shaderProgram, "lightPosition");
        glUniform3fv(lightPositionUniform, 1, glm::value_ptr(lightPosition));
        GLint cameraPositionUniform = glGetUniformLocation(shaderProgram, "cameraPosition");
        glUniform3fv(cameraPositionUniform, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -10.0f)));

        // Add uniform values for textures.
        GLuint colorTextureUniform  = glGetUniformLocation(shaderProgram, "colorTexture");
        glUniform1i(colorTextureUniform, 0);
        GLuint normalTextureUniform  = glGetUniformLocation(shaderProgram, "normalTexture");
        glUniform1i(normalTextureUniform, 1);
        GLuint displacementTextureUniform  = glGetUniformLocation(shaderProgram, "displacementTexture");
        glUniform1i(displacementTextureUniform, 2);

        // Bind the vertex array object we are going to draw and define how buffer values are split per vertices.
        glBindVertexArray(vao);

        // Describe vertex coordinates buffer.
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Describe texture coordinates buffer.
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Describe normal buffer.
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Describe tangent buffer.
        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Describe bitangent buffer.
        glEnableVertexAttribArray(4);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentBuffer);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Set up viewport.
        glViewport(0, 0, windowStruct.width, windowStruct.height);

        // Draw the vertex attribute buffer.
        glDrawArrays(GL_TRIANGLES, 0, obj.vertexes().size());

        // Swap buffers.
        glfwSwapBuffers(window);

        // Poll system events.
        glfwPollEvents();
    }

    // Delete textures.
    GLuint textures[] = {colorTextureId, normalTextureId, displacementTextureId};
    glDeleteTextures(sizeof(textures) / sizeof(textures[0]), textures);

    // Delete vetrex buffers.
    GLuint buffers[] = { vertexBuffer, uvBuffer, normalBuffer, tangentBuffer, bitangentBuffer };
    glDeleteBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);

    // Delete vertex attribute array.
    glDeleteVertexArrays(1, &vao);

    // Delete shader program.
    glDeleteProgram(shaderProgram);

    // Destroy the window.
    glfwDestroyWindow(window);

    // Clean up GLFW.
    glfwTerminate();

    return 0;
}

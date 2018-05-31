#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#include <glm/glm.hpp> // glm::vec3, glm::vec4, glm::ivec4, glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include <iostream>

class GLWindow : public std::enable_shared_from_this< GLWindow >
{
public:
    GLWindow(int width, int height, const char* title) {
        m_window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (m_window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
        }
        glfwMakeContextCurrent(m_window);
        glfwSetFramebufferSizeCallback(m_window, GLWindow::framebufferSizeCallback);

        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        }
        fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

        // build and compile our shader program
        // ------------------------------------
        // vertex shader
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // check for shader compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // fragment shader
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // check for shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // link shaders
        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vertexShader);
        glAttachShader(m_shaderProgram, fragmentShader);

        glLinkProgram(m_shaderProgram);
        // check for linking errors
        glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // set up vertex data (and buffer(s)) and configure vertex attributes
        // ------------------------------------------------------------------


        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        float vertices[] = {
            -1.0f, -1.0f, 0.2f, // left
            0.0f, -1.0f, 0.2f, // right
            -0.5f,  1.0f, 0.2f,  // top

            1.0f, 1.0f, 0.5f, // left
            0.0f, 1.0f, 0.5f, // right
            0.5f,  -1.0f, 0.5f  // top
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);
    }
    void render() {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw our first triangle
        glUseProgram(m_shaderProgram);

//        GLfloat matrix[16] { scale, 0, 0, objectX,
//                             0, scale, 0, objectY,
//                             0, 0, scale, objectZ,
//                             0, 0, 0, 1 };
        float camX = sin(glfwGetTime()) * 10;
        float camZ = cos(glfwGetTime()) * 10;

        glm::mat4 cameraMatrix4 = glm::lookAt(
            glm::vec3(objectX, objectY, objectZ), // Позиция камеры в мировом пространстве
            glm::vec3(0, 0, 1),   // Указывает куда вы смотрите в мировом пространстве
            glm::vec3(0, 1, 0)        // Вектор, указывающий направление вверх. Обычно (0, 1, 0)
        );

        glm::mat4 modelMatrix4 = glm::rotate(glm::mat4(1.0f), float(glfwGetTime()), glm::vec3(0, 1, 0));

        glm::mat4 projectionMatrix4 = glm::perspective(
            glm::radians(30.0f), // Вертикальное поле зрения в радианах. Обычно между 90&deg; (очень широкое) и 30&deg; (узкое)
            4.0f / 3.0f,       // Отношение сторон. Зависит от размеров вашего окна. Заметьте, что 4/3 == 800/600 == 1280/960
            0.1f,              // Ближняя плоскость отсечения. Должна быть больше 0.
            100.0f             // Дальняя плоскость отсечения.
        );

        GLint modelMatrix = glGetUniformLocation(m_shaderProgram, "modelMatrix");
        glUniformMatrix4fv(modelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix4));

        GLint cameraMatrix = glGetUniformLocation(m_shaderProgram, "cameraMatrix");
        glUniformMatrix4fv(cameraMatrix, 1, GL_FALSE, glm::value_ptr(cameraMatrix4));

        GLint projectionMatrix = glGetUniformLocation(m_shaderProgram, "projectionMatrix");
        glUniformMatrix4fv(projectionMatrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix4));

        glBindVertexArray(m_VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // glBindVertexArray(0); // no need to unbind it every time

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(m_window);
    }
    void clear() {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
    }
    int shouldBeClosed() {
        return glfwWindowShouldClose(m_window);
    }
private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }
    GLFWwindow* m_window;
    int m_shaderProgram;
    unsigned int m_VAO;
    unsigned int m_VBO;


    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 modelMatrix;
        uniform mat4 cameraMatrix;
        uniform mat4 projectionMatrix;

        out vec4 colorOutput;


        void main()
        {
            //vec4 position = vec4(aPos.x, aPos.y, aPos.z, 1.0) * cameraMatrix * modelMatrix * projectionMatrix;
            mat4 MVP = projectionMatrix * cameraMatrix * modelMatrix;
            vec4 position = MVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);
            gl_Position = position;
            //colorOutput = vec4(normalize(aPos), 1.0);
            if (aPos.z == 0.2f) {
                colorOutput = vec4(1, 0, 0, 1);
            } else {
                colorOutput = vec4(0, 1, 0, 1);
            }
        }
    )";
    const char *fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec4 colorOutput;
        void main()
        {
           FragColor = colorOutput;
        }
    )";

    GLfloat objectX = 0;
    GLfloat objectY = 0;
    GLfloat objectZ = -10.0;
    GLfloat scale = 1;
};

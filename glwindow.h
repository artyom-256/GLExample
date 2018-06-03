#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#include <glm/glm.hpp> // glm::vec3, glm::vec4, glm::ivec4, glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include <iostream>

#include "object.h"
#include "bitmapimage.h"

class GLWindow : public std::enable_shared_from_this< GLWindow >
{
public:
    GLWindow(int width, int height, const char* title)
        : obj("D://box.obj") {
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

        // Enable depth test
            glEnable(GL_DEPTH_TEST);
            // Accept fragment if it closer to the camera than the former one
            glDepthFunc(GL_LESS);

            glEnable(GL_CULL_FACE);


        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

//        float vertices[] = {
//            -1.0f, -1.0f, 0.2f, // left
//            0.0f, -1.0f, 0.2f, // right
//            -0.5f,  1.0f, 0.2f,  // top

//            1.0f, 1.0f, 0.5f, // left
//            0.0f, 1.0f, 0.5f, // right
//            0.5f,  -1.0f, 0.5f  // top
//        };

        float *vertices = obj.getVertexes();

        glBufferData(GL_ARRAY_BUFFER, obj.numVertexes() * sizeof(float), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);


        BitmapImage img("D://10.bmp");

        glActiveTexture(GL_TEXTURE0);
        // Создадим одну текстуру OpenGL
        glGenTextures(1, &textureID);
        // Сделаем созданную текстуру текущий, таким образом все следующие функции будут работать именно с этой текстурой
        glBindTexture(GL_TEXTURE_2D, textureID);
        // Передадим изображение OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, img.getWidth(), img.getHeight(), 0, GL_BGR, GL_UNSIGNED_BYTE, img.getData());
        int g_nMaxAnisotropy;
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&g_nMaxAnisotropy);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,
                            g_nMaxAnisotropy-0.1);
        // Когда изображение увеличивается, то мы используем обычную линейную фильтрацию
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Когда изображение уменьшается, то мы используем линейной смешивание 2х мипмапов, к которым также применяется линейная фильтрация
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        // И генерируем мипмап
        glGenerateMipmap(GL_TEXTURE_2D);;

        BitmapImage img2("D://10nm.bmp");

        glActiveTexture(GL_TEXTURE1);
        // Создадим одну текстуру OpenGL
        glGenTextures(1, &textureNormID);
        // Сделаем созданную текстуру текущий, таким образом все следующие функции будут работать именно с этой текстурой
        glBindTexture(GL_TEXTURE_2D, textureNormID);
        // Передадим изображение OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, img2.getWidth(), img2.getHeight(), 0, GL_BGR, GL_UNSIGNED_BYTE, img2.getData());
        // Когда изображение увеличивается, то мы используем обычную линейную фильтрацию
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            // ... which requires mipmaps. Generate them automatically.
            glGenerateMipmap(GL_TEXTURE_2D);



        glGenBuffers(1, &uvbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, obj.numUVs() * sizeof(float), obj.getUVs(), GL_STATIC_DRAW);

        glGenBuffers(1, &normbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, normbuffer);
        glBufferData(GL_ARRAY_BUFFER, obj.numNormales() * sizeof(float), obj.getNormales(), GL_STATIC_DRAW);

        glGenBuffers(1, &tangentsBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, tangentsBuffer);
        glBufferData(GL_ARRAY_BUFFER, obj.getTangents().size() * sizeof(glm::vec3), &obj.getTangents()[0], GL_STATIC_DRAW);

        glGenBuffers(1, &bitangentsBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentsBuffer);
        glBufferData(GL_ARRAY_BUFFER, obj.getTangents().size() * sizeof(glm::vec3), &obj.getBitangents()[0], GL_STATIC_DRAW);
    }
    void render() {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw our first triangle
        glUseProgram(m_shaderProgram);

//        GLfloat matrix[16] { scale, 0, 0, objectX,
//                             0, scale, 0, objectY,
//                             0, 0, scale, objectZ,
//                             0, 0, 0, 1 };
        float camX = sin(glfwGetTime()) * 10;
        float camZ = cos(glfwGetTime()) * 10;

        glm::mat4 cameraMatrix4 = glm::lookAt(
            glm::vec3(0, 0, 0), // Позиция камеры в мировом пространстве
            glm::vec3(0, 0, 1),   // Указывает куда вы смотрите в мировом пространстве
            glm::vec3(0, 1, 0)        // Вектор, указывающий направление вверх. Обычно (0, 1, 0)
        );

        cameraMatrix4 = m_cameraRotation * cameraMatrix4;
        cameraMatrix4 = glm::translate(cameraMatrix4, -m_cameraPosition);

        glm::mat4 modelMatrix4 = glm::rotate(glm::mat4(1.0f), float(glfwGetTime()), glm::vec3(0, 1, 0));

        glm::mat4 projectionMatrix4 = glm::perspective(
            glm::radians(30.0f), // Вертикальное поле зрения в радианах. Обычно между 90&deg; (очень широкое) и 30&deg; (узкое)
            4.0f / 3.0f,       // Отношение сторон. Зависит от размеров вашего окна. Заметьте, что 4/3 == 800/600 == 1280/960
            0.1f,              // Ближняя плоскость отсечения. Должна быть больше 0.
            100.0f             // Дальняя плоскость отсечения.
        );

        glm::vec3 lightPosition3{0.0, 0.0, -10.0};

        //glm::mat4 lightModelMatrix = glm::rotate(glm::mat4(1.0f), float(-glfwGetTime() * 3), glm::vec3(0, 1, 0));
        //lightPosition4 = lightModelMatrix * lightPosition4;

        GLint modelMatrix = glGetUniformLocation(m_shaderProgram, "modelMatrix");
        glUniformMatrix4fv(modelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix4));

        GLint cameraMatrix = glGetUniformLocation(m_shaderProgram, "cameraMatrix");
        glUniformMatrix4fv(cameraMatrix, 1, GL_FALSE, glm::value_ptr(cameraMatrix4));

        GLint projectionMatrix = glGetUniformLocation(m_shaderProgram, "projectionMatrix");
        glUniformMatrix4fv(projectionMatrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix4));

        GLint lightPosition = glGetUniformLocation(m_shaderProgram, "lightPosition");
        glUniform3fv(lightPosition, 1, glm::value_ptr(lightPosition3));

        GLint cameraPosition = glGetUniformLocation(m_shaderProgram, "cameraPosition");
        glUniform3fv(cameraPosition, 1, glm::value_ptr(glm::vec3(objectX, objectY, objectZ)));

        GLuint myTextureSampler  = glGetUniformLocation(m_shaderProgram, "myTextureSampler");
        glUniform1i(myTextureSampler, 0);
        GLuint myTextureSampler2  = glGetUniformLocation(m_shaderProgram, "myTextureSampler2");
        glUniform1i(myTextureSampler2, 1);

//        GLint myTextureSampler = glGetUniformLocation(m_shaderProgram, "myTextureSampler");
//        glUniformMatrix4fv(myTextureSampler, 1, GL_FALSE, glm::value_ptr(myTextureSampler));

        glBindVertexArray(m_VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        std::cout << "NUM VERT " << obj.numVertexes();

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(
                    1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    2,                                // size : U+V => 2
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normbuffer);
        glVertexAttribPointer(
                    2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    3,                                // size : U+V => 2
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );

        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, tangentsBuffer);
        glVertexAttribPointer(
                    3,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );

        glEnableVertexAttribArray(4);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentsBuffer);
        glVertexAttribPointer(
                    4,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );


        glDrawArrays(GL_TRIANGLES, 0, obj.numVertexes());
        // glBindVertexArray(0); // no need to unbind it every time

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(m_window);
    }
    void processInput()
    {
        if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            m_cameraRotation = glm::rotate(m_cameraRotation, -0.01f, glm::vec3(0, 1, 0));
        } else if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            m_cameraRotation = glm::rotate(m_cameraRotation, 0.01f, glm::vec3(0, 1, 0));
        } else if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) {
            m_cameraRotation = glm::rotate(m_cameraRotation, -0.01f, glm::vec3(1, 0, 0));
        } else if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            m_cameraRotation = glm::rotate(m_cameraRotation, 0.01f, glm::vec3(1, 0, 0));
        } else if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
            m_cameraPosition += glm::vec3(m_cameraRotation * glm::vec4(0.0, 0.0, 0.1, 1.0));
        } else if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
            m_cameraPosition += glm::vec3(m_cameraRotation * glm::vec4(0.0, 0.0, -0.1, 1.0));
        } else if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
        } else if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
        }
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

    GLuint uvbuffer;
    GLuint normbuffer;
    GLuint tangentsBuffer;
    GLuint bitangentsBuffer;
    GLuint rotationMatrixes;
    GLuint textureID;
    GLuint textureNormID;

    Object obj;


    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 vertexPosition_modelspace;
        layout (location = 1) in vec2 vertexUV;
        layout (location = 2) in vec3 vertexNormal_modelspace;
        layout (location = 3) in vec3 vertexTangent_modelspace;
        layout (location = 4) in vec3 vertexBitangent_modelspace;
        uniform mat4 modelMatrix;
        uniform mat4 cameraMatrix;
        uniform mat4 projectionMatrix;

        out vec3 colorOutput;
        out vec2 UV;
        out vec4 lightPos;
        out vec4 norm;
        out vec3 lightDirection;
        out vec3 cameraDirection;
        out mat3 TBNout;

        uniform vec3 lightPosition;
        uniform vec3 cameraPosition;


        void main()
        {
            mat4 MVP = projectionMatrix * cameraMatrix * modelMatrix;

            // Output position of the vertex, in clip space : MVP * position
             gl_Position =  MVP * vec4(vertexPosition_modelspace,1);

             // Vector that goes from the vertex to the camera, in camera space.
             // In camera space, the camera is at the origin (0,0,0).
             vec3 vertexPosition_cameraspace = ( cameraMatrix * modelMatrix * vec4(vertexPosition_modelspace,1)).xyz;
             vec3 EyeDirection_cameraspace = vec3(0,0,0) - vertexPosition_cameraspace;

             // Vector that goes from the vertex to the light, in camera space. M is ommited because it's identity.
             vec3 LightPosition_cameraspace = ( cameraMatrix * vec4(lightPosition,1)).xyz;
             vec3 LightDirection_cameraspace = LightPosition_cameraspace + EyeDirection_cameraspace;

             // UV of the vertex. No special space for this one.
             UV = vertexUV;

             mat3 MV3x3 = mat3(cameraMatrix) * mat3(modelMatrix);

             // model to camera = ModelView
             vec3 vertexTangent_cameraspace = MV3x3 * vertexTangent_modelspace;
             vec3 vertexBitangent_cameraspace = MV3x3 * vertexBitangent_modelspace;
             vec3 vertexNormal_cameraspace = MV3x3 * vertexNormal_modelspace;

             mat3 TBN = transpose(mat3(
                 vertexTangent_cameraspace,
                 vertexBitangent_cameraspace,
                 vertexNormal_cameraspace
             )); // You can use dot products instead of building this matrix and transposing it. See References for details.

            //TBN = mat3(modelMatrix);

            float res = length(cross(cross(vertexTangent_cameraspace, vertexBitangent_cameraspace), vertexNormal_cameraspace));

            if (abs(res) < 0.0000001) {
                colorOutput = vec3(0,1,0);
            } else {
                colorOutput = vec3(1, 0, 1);
            }

            //lightDirection = normalize(LightDirection_cameraspace);
            //cameraDirection =  normalize(EyeDirection_cameraspace);
            //norm = normalize(vec4(MV3x3 * vertexNormal_modelspace, 1.0));
            lightDirection = normalize(TBN * LightDirection_cameraspace);
            cameraDirection =  normalize(TBN * EyeDirection_cameraspace);
            norm = normalize(vec4(TBN * MV3x3 * vertexNormal_modelspace, 1.0));

//            if (length(cross(vertexTangent_modelspace, vertexBitangent_modelspace)) < 0.1) {
//                colorOutput = vec3(1,0,0);
//            } else {
//                colorOutput = vec3(0,1,0);
//            }
            //colorOutput = vec3(dot(lightDirection, mat3(inverse(TBN) * TBN) * lightDirection),0,0);
        }
    )";
    const char *fragmentShaderSource = R"(
        #version 330 core
        out vec3 FragColor;
        in vec3 colorOutput;

        in vec3 lightDirection;
        in vec3 cameraDirection;
        in vec2 UV;
        in vec4 norm;

        uniform mat4 modelMatrix;
        in mat3 TBNout;


        uniform sampler2D myTextureSampler;
        uniform sampler2D myTextureSampler2;

        void main()
        {
            //FragColor = colorOutput;
            //return;
           vec3 texNormVec = normalize(texture( myTextureSampler2, UV ).xyz * 2.0 - 1.0);
           //vec3 texNormVec = normalize(vec3(0.0, 0.0, 1.0));
           vec4 normVec = vec4(texNormVec, 1.0);
            //vec4 normVec = norm;
            //vec4 normVec = vec4(0, 0, 1, 1);

           //FragColor = vec3(dot(normVec.xyz, lightDirection), 0.0, 0.0);
           vec3 textureColor = texture( myTextureSampler, UV ).rgb;

           // Ambient
           vec3 ambientColor = textureColor * 0.2;

           // Diffuse
           float diffuseFactor = max(dot(normVec.xyz, lightDirection), 0);
           vec3 diffuseColor = textureColor * diffuseFactor;

           // Specular
           vec3 R = 2*dot(normVec.xyz, lightDirection) * normVec.xyz - lightDirection;
           float specularFactor = max((dot(R, cameraDirection) * .5), 0) * .5;
           vec3 specularColor = textureColor * specularFactor;

           if (dot(norm.xyz, lightDirection) > 0) {

             FragColor = ambientColor +
                       diffuseColor +
                       specularColor +
                       0;

            } else {
               FragColor = ambientColor;
            }
        }
    )";

    GLfloat objectX = 0;
    GLfloat objectY = 0;
    GLfloat objectZ = -10.0;
    GLfloat scale = 1;
    GLfloat rotationY = 0.0;

    glm::vec3 m_cameraPosition{0, 0, -10.0};
    glm::mat4 m_cameraRotation{1};
};

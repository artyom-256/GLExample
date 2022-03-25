#include "object.h"
#include "bitmap_image.h"

#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLFWwindow* m_window;
int m_shaderProgram;
unsigned int m_VAO;
unsigned int m_VBO;

GLuint uvbuffer;
GLuint normbuffer;
GLuint tangentsBuffer;
GLuint bitangentsBuffer;
GLuint textureID;
GLuint textureNormID;
GLuint textureDSPID;

object obj("box.obj");

glm::vec3 m_cameraPosition{0, 0, -10.0};
glm::mat4 m_cameraRotation{1};


const char* vertexShaderSource = R"(
    #version 330 core

    layout (location = 0) in vec3 vertexPosition;
    layout (location = 1) in vec2 vertexUV;
    layout (location = 2) in vec3 vertexNormal;
    layout (location = 3) in vec3 vertexTangent;
    layout (location = 4) in vec3 vertexBitangent;

    out vec2 UV;
    out vec3 lightDirection;
    out vec3 cameraDirection;
    out vec3 viewDirection;

    uniform mat4 modelMatrix;
    uniform mat4 cameraMatrix;
    uniform mat4 projectionMatrix;
    uniform vec3 lightPosition;
    uniform vec3 cameraPosition;

    void main()
    {
        // Calculate full transformation matrix.
        mat4 MVP = projectionMatrix * cameraMatrix * modelMatrix;

        // Calculate direction from the vertex to the camera.
        vec3 vertexPositionCameraSpace = (cameraMatrix * modelMatrix * vec4(vertexPosition, 1.0)).xyz;
        vec3 eyeDirectionCameraSpace = vec3(0, 0, 0) - vertexPositionCameraSpace;

        // Calculation direction of the light.
        vec3 lightPositionCameraSpace = (cameraMatrix * vec4(lightPosition,1)).xyz;
        vec3 lightDirectionCameraSpace = lightPositionCameraSpace + eyeDirectionCameraSpace;

        // Rotation matrix to transform from model space to camera space.
        mat3 MV3x3 = mat3(cameraMatrix) * mat3(modelMatrix);

        // Transform vectors to camera space.
        vec3 vertexTangentCameraSpace = MV3x3 * vertexTangent;
        vec3 vertexBitangentCameraSpace = MV3x3 * vertexBitangent;
        vec3 vertexNormalCameraSpace = MV3x3 * vertexNormal;

        // Inverse TBN matrix to transform from camera space to texture coordinates.
        mat3 invTBN = transpose(mat3(
            vertexTangentCameraSpace,
            vertexBitangentCameraSpace,
            vertexNormalCameraSpace
        ));

        // Output position of the vertex.
        gl_Position =  MVP * vec4(vertexPosition, 1);

        // Output texture coordinate.
        UV = vertexUV;

        // Output direction vectors in tangent coordinate system.
        lightDirection = normalize(invTBN * lightDirectionCameraSpace);
        cameraDirection =  normalize(invTBN * eyeDirectionCameraSpace);

        // Output view direction in tangent space.
        viewDirection = invTBN * vertexPosition - invTBN * cameraPosition;
    }
)";
const char *fragmentShaderSource = R"(
    #version 330 core

    in vec2 UV;
    in vec3 lightDirection;
    in vec3 cameraDirection;
    in vec3 viewDirection;

    out vec3 FragColor;

    uniform mat4 modelMatrix;
    uniform sampler2D colorTexture;
    uniform sampler2D normalTexture;
    uniform sampler2D parallaxTexture;

    // Shift texture coordinates according to the parallax map.
    // @param texCoords Original texture coordinates.
    // @param viewDir Vector from camera to the point.
    // @return New texture coordinates taking into account parallax.
    vec2 parallaxMap(vec2 texCoords, vec3 viewDir)
    {
        // Define how deep the parallax map is.
        const float HEIGHT_SCALE = 0.07;

        // Number of depth layers
        const float NUM_LAYERS = 20;

        // Calculate the size of each layer.
        float layerDepth = 1.0 / NUM_LAYERS;
        // Depth of current layer.
        float currentLayerDepth = 0.0;
        // Amount to shift the texture coordinates per layer (from vector P).
        vec2 P = viewDir.xy * HEIGHT_SCALE;
        vec2 deltaTexCoords = P / NUM_LAYERS;

        // Get initial values.
        vec2  currentTexCoords     = texCoords;
        float currentDepthMapValue = texture(parallaxTexture, currentTexCoords).r;
        vec2 shift = vec2(0.0, 0.0);

        while(currentLayerDepth < currentDepthMapValue)
        {
            // shift texture coordinates along direction of P
            shift -= deltaTexCoords;
            // get depthmap value at current texture coordinates
            currentDepthMapValue = texture(parallaxTexture, currentTexCoords + shift).r;
            // get depth of next layer
            currentLayerDepth += layerDepth;
        }

        // get texture coordinates before collision (reverse operations)
        vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

        currentTexCoords += shift;

        // get depth after and before collision for linear interpolation
        float afterDepth  = currentDepthMapValue - currentLayerDepth;
        float beforeDepth = texture(parallaxTexture, prevTexCoords).r - currentLayerDepth + layerDepth;

        // interpolation of texture coordinates
        float weight = afterDepth / (afterDepth - beforeDepth);
        vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

        return currentTexCoords;
    }

    void main()
    {
        // Normalize input parameters after interpolation.
        vec3 viewDirectionNormalized = normalize(viewDirection);
        vec3 lightDirectionNormalized = normalize(lightDirection);
        vec3 cameraDirectionNormalized = normalize(cameraDirection);

        // Adjust the texture coordinate according to the parallax map.
        vec2 texCoords = parallaxMap(UV,  viewDirectionNormalized);

        // Discard fragments that are moved out from the texture after applying the parallax transformation.
        if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        {
            discard;
        }

        // Read normal vector in texture coordinate system.
        vec3 texNormalVector = normalize(texture(normalTexture, texCoords).xyz * 2.0 - 1.0);
        vec4 normalVec = vec4(texNormalVector, 1.0);

        // Read fragment color.
        vec3 textureColor = texture(colorTexture, texCoords).rgb;

        // Ambient component of the color.
        vec3 ambientColor = textureColor * 0.2;

        // Diffuse component of the color.
        float diffuseFactor = max(dot(normalVec.xyz, lightDirectionNormalized), 0);
        vec3 diffuseColor = textureColor * diffuseFactor;

        // Specular component of the color.
        vec3 R = 2 * dot(normalVec.xyz, lightDirectionNormalized) * normalVec.xyz - lightDirectionNormalized;
        float specularFactor = max((dot(R, cameraDirectionNormalized) * 0.5), 0) * 0.5;
        vec3 specularColor = textureColor * specularFactor;

        // Calculate fragment color.
        FragColor = ambientColor + diffuseColor + specularColor;
   }
)";

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 32);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    {
        m_window = glfwCreateWindow(800, 600, "GLTest", NULL, NULL);
        if (m_window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
        }
        glfwMakeContextCurrent(m_window);
        glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

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

        const auto& vertices = obj.vertexes();

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);


        bitmap_image img("texture.bmp");

        glActiveTexture(GL_TEXTURE0);
        // Создадим одну текстуру OpenGL
        glGenTextures(1, &textureID);
        // Сделаем созданную текстуру текущий, таким образом все следующие функции будут работать именно с этой текстурой
        glBindTexture(GL_TEXTURE_2D, textureID);
        // Передадим изображение OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, img.width(), img.height(), 0, GL_BGR, GL_UNSIGNED_BYTE, &img.data()[0]);
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

        bitmap_image img2("normal.bmp");

        glActiveTexture(GL_TEXTURE1);
        // Создадим одну текстуру OpenGL
        glGenTextures(1, &textureNormID);
        // Сделаем созданную текстуру текущий, таким образом все следующие функции будут работать именно с этой текстурой
        glBindTexture(GL_TEXTURE_2D, textureNormID);
        // Передадим изображение OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, img2.width(), img2.height(), 0, GL_BGR, GL_UNSIGNED_BYTE, &img2.data()[0]);
        // Когда изображение увеличивается, то мы используем обычную линейную фильтрацию
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            // ... which requires mipmaps. Generate them automatically.
            glGenerateMipmap(GL_TEXTURE_2D);


            bitmap_image img3("displacement.bmp");

        glActiveTexture(GL_TEXTURE2);
        // Создадим одну текстуру OpenGL
        glGenTextures(1, &textureDSPID);
        // Сделаем созданную текстуру текущий, таким образом все следующие функции будут работать именно с этой текстурой
        glBindTexture(GL_TEXTURE_2D, textureDSPID);
        // Передадим изображение OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, img3.width(), img3.height(), 0, GL_BGR, GL_UNSIGNED_BYTE, &img3.data()[0]);
        // Когда изображение увеличивается, то мы используем обычную линейную фильтрацию
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        // ... which requires mipmaps. Generate them automatically.
        glGenerateMipmap(GL_TEXTURE_2D);


        glGenBuffers(1, &uvbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, obj.uvs().size() * sizeof(glm::vec2), obj.uvs().data(), GL_STATIC_DRAW);

        glGenBuffers(1, &normbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, normbuffer);
        glBufferData(GL_ARRAY_BUFFER, obj.normals().size() * sizeof(glm::vec3), obj.normals().data(), GL_STATIC_DRAW);

        glGenBuffers(1, &tangentsBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, tangentsBuffer);
        glBufferData(GL_ARRAY_BUFFER, obj.tangents().size() * sizeof(glm::vec3), &obj.tangents()[0], GL_STATIC_DRAW);

        glGenBuffers(1, &bitangentsBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentsBuffer);
        glBufferData(GL_ARRAY_BUFFER, obj.tangents().size() * sizeof(glm::vec3), &obj.bitangents()[0], GL_STATIC_DRAW);

        while (!glfwWindowShouldClose(m_window))
        {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // draw our first triangle
            glUseProgram(m_shaderProgram);

            glm::mat4 cameraMatrix4 = glm::lookAt(
                glm::vec3(0, 0, 0), // Позиция камеры в мировом пространстве
                glm::vec3(0, 0, 1),   // Указывает куда вы смотрите в мировом пространстве
                glm::vec3(0, 1, 0)        // Вектор, указывающий направление вверх. Обычно (0, 1, 0)
            );

            cameraMatrix4 = m_cameraRotation * cameraMatrix4;
            cameraMatrix4 = glm::translate(cameraMatrix4, -m_cameraPosition);

            glm::mat4 modelMatrix4 = glm::rotate(glm::mat4(1.0f), float(glfwGetTime()) / 2, glm::vec3(0, 1, 0));
            //modelMatrix4 *= glm::rotate(glm::mat4(1.0f), float(glfwGetTime()) / 4, glm::vec3(1, 0, 0));

            glm::mat4 projectionMatrix4 = glm::perspective(
                glm::radians(30.0f), // Вертикальное поле зрения в радианах. Обычно между 90&deg; (очень широкое) и 30&deg; (узкое)
                4.0f / 3.0f,       // Отношение сторон. Зависит от размеров вашего окна. Заметьте, что 4/3 == 800/600 == 1280/960
                0.1f,              // Ближняя плоскость отсечения. Должна быть больше 0.
                100.0f             // Дальняя плоскость отсечения.
            );

            glm::vec3 lightPosition3{0.0, 0.0, -10.0};

            lightPosition3 = m_cameraPosition;

            GLint modelMatrix = glGetUniformLocation(m_shaderProgram, "modelMatrix");
            glUniformMatrix4fv(modelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix4));

            GLint cameraMatrix = glGetUniformLocation(m_shaderProgram, "cameraMatrix");
            glUniformMatrix4fv(cameraMatrix, 1, GL_FALSE, glm::value_ptr(cameraMatrix4));

            GLint projectionMatrix = glGetUniformLocation(m_shaderProgram, "projectionMatrix");
            glUniformMatrix4fv(projectionMatrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix4));

            GLint lightPosition = glGetUniformLocation(m_shaderProgram, "lightPosition");
            glUniform3fv(lightPosition, 1, glm::value_ptr(lightPosition3));

            GLint cameraPosition = glGetUniformLocation(m_shaderProgram, "cameraPosition");
            glUniform3fv(cameraPosition, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -10.0f)));

            GLuint myTextureSampler  = glGetUniformLocation(m_shaderProgram, "colorTexture");
            glUniform1i(myTextureSampler, 0);
            GLuint myTextureSampler2  = glGetUniformLocation(m_shaderProgram, "normalTexture");
            glUniform1i(myTextureSampler2, 1);
            GLuint myTextureSampler3  = glGetUniformLocation(m_shaderProgram, "parallaxTexture");
            glUniform1i(myTextureSampler3, 2);

            glBindVertexArray(m_VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

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


            glDrawArrays(GL_TRIANGLES, 0, obj.vertexes().size());

            glfwSwapBuffers(m_window);


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

            glfwPollEvents();
        }
    }
    glfwTerminate();

    return 0;
}

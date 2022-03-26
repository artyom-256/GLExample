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
const int ERROR_BUFFER_SIZE = 512;

/**
 * Vertex shader.
 */
const char* VERTEX_SHADER_SOURCE = R"(
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

/**
 * Fragment shader.
 */
const char* FRAGMENT_SHADER_SOURCE = R"(
    #version 330 core

    in vec2 UV;
    in vec3 lightDirection;
    in vec3 cameraDirection;
    in vec3 viewDirection;

    out vec3 FragColor;

    uniform mat4 modelMatrix;
    uniform sampler2D colorTexture;
    uniform sampler2D normalTexture;
    uniform sampler2D displacementTexture;

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
        float currentDepthMapValue = texture(displacementTexture, currentTexCoords).r;
        vec2 shift = vec2(0.0, 0.0);

        while(currentLayerDepth < currentDepthMapValue) {
            // shift texture coordinates along direction of P
            shift -= deltaTexCoords;
            // get depthmap value at current texture coordinates
            currentDepthMapValue = texture(displacementTexture, currentTexCoords + shift).r;
            // get depth of next layer
            currentLayerDepth += layerDepth;
        }

        // get texture coordinates before collision (reverse operations)
        vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

        currentTexCoords += shift;

        // get depth after and before collision for linear interpolation
        float afterDepth  = currentDepthMapValue - currentLayerDepth;
        float beforeDepth = texture(displacementTexture, prevTexCoords).r - currentLayerDepth + layerDepth;

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
        vec2 texCoords = parallaxMap(UV, viewDirectionNormalized);

        // Discard fragments that are moved out from the texture after applying the parallax transformation.
        if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0) {
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
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &VERTEX_SHADER_SOURCE, NULL);
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
    glShaderSource(fragmentShader, 1, &FRAGMENT_SHADER_SOURCE, NULL);
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
    auto shaderProgram = glCreateProgram();
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
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
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
    GLuint textureNormID;
    glGenTextures(1, &textureNormID);
    glBindTexture(GL_TEXTURE_2D, textureNormID);
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
    GLuint textureDSPID;
    glGenTextures(1, &textureDSPID);
    glBindTexture(GL_TEXTURE_2D, textureDSPID);
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

    // Render loop.
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw our first triangle
        glUseProgram(shaderProgram);

        glm::mat4 cameraMatrix4 = glm::lookAt(
            glm::vec3(0, 0, 0), // Позиция камеры в мировом пространстве
            glm::vec3(0, 0, 1),   // Указывает куда вы смотрите в мировом пространстве
            glm::vec3(0, 1, 0)        // Вектор, указывающий направление вверх. Обычно (0, 1, 0)
        );

        glm::vec3 cameraPosition(0, 0, -7.0);
        glm::mat4 m_cameraRotation(1);

        cameraMatrix4 = m_cameraRotation * cameraMatrix4;
        cameraMatrix4 = glm::translate(cameraMatrix4, -cameraPosition);

        glm::mat4 modelMatrix4 = glm::rotate(glm::mat4(1.0f), float(glfwGetTime()) / 2, glm::vec3(0, 1, 0));
        modelMatrix4 *= glm::rotate(glm::mat4(1.0f), float(glfwGetTime()) / 4, glm::vec3(1, 0, 0));

        glm::mat4 projectionMatrix4 = glm::perspective(
            glm::radians(30.0f), // Вертикальное поле зрения в радианах. Обычно между 90&deg; (очень широкое) и 30&deg; (узкое)
            float(windowStruct.width) / windowStruct.height,       // Отношение сторон. Зависит от размеров вашего окна. Заметьте, что 4/3 == 800/600 == 1280/960
            0.1f,              // Ближняя плоскость отсечения. Должна быть больше 0.
            100.0f             // Дальняя плоскость отсечения.
        );

        glm::vec3 lightPosition3(0.0, 0.0, -10.0);

        lightPosition3 = cameraPosition;

        GLint modelMatrix = glGetUniformLocation(shaderProgram, "modelMatrix");
        glUniformMatrix4fv(modelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix4));

        GLint cameraMatrix = glGetUniformLocation(shaderProgram, "cameraMatrix");
        glUniformMatrix4fv(cameraMatrix, 1, GL_FALSE, glm::value_ptr(cameraMatrix4));

        GLint projectionMatrix = glGetUniformLocation(shaderProgram, "projectionMatrix");
        glUniformMatrix4fv(projectionMatrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix4));

        GLint lightPosition = glGetUniformLocation(shaderProgram, "lightPosition");
        glUniform3fv(lightPosition, 1, glm::value_ptr(lightPosition3));

        GLint cameraPositionLocation = glGetUniformLocation(shaderProgram, "cameraPosition");
        glUniform3fv(cameraPositionLocation, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -10.0f)));

        GLuint myTextureSampler  = glGetUniformLocation(shaderProgram, "colorTexture");
        glUniform1i(myTextureSampler, 0);
        GLuint myTextureSampler2  = glGetUniformLocation(shaderProgram, "normalTexture");
        glUniform1i(myTextureSampler2, 1);
        GLuint myTextureSampler3  = glGetUniformLocation(shaderProgram, "displacementTexture");
        glUniform1i(myTextureSampler3, 2);

        glBindVertexArray(vao); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(
                    0,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    3,                                // size : U+V => 2
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );


        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glVertexAttribPointer(
                    1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    2,                                // size : U+V => 2
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glVertexAttribPointer(
                    2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    3,                                // size : U+V => 2
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );

        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
        glVertexAttribPointer(
                    3,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );

        glEnableVertexAttribArray(4);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentBuffer);
        glVertexAttribPointer(
                    4,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );


        glViewport(0, 0, windowStruct.width, windowStruct.height);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertexes().size());

        glfwSwapBuffers(window);

        glfwPollEvents();

        // TODO: CLEAN UP!!!!
        // TODO: move shaders to separate files!!!
    }
    glfwTerminate();

    return 0;
}

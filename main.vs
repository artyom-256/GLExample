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

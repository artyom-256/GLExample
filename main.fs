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

#include "object.h"

object::object(const char* fileName) {
    // Open the file.
    std::ifstream ifs(fileName);
    if (!ifs) {
        return;
    }
    // Prepare arrays for inexed vertices, uvs and normals.
    std::vector< glm::vec3 > vertices;
    std::vector< glm::vec2 > uvs;
    std::vector< glm::vec3 > normals;
    std::vector< int > vertexIndices;
    std::vector< int > uvIndices;
    std::vector< int > normalIndices;
    // Read the file line by line.
    std::string line;
    while (ifs) {
        getline(ifs, line);
        // Skip empty or invalid lines.
        if (line.length() < 3) {
            continue;
        }
        // Parse vertices (start with 'v ').
        if (line[0] == 'v' && line[1] == ' ') {
            // Vertex has the following format:
            // v [x] [y] [z] [w]
            // where [w] is optional.
            std::vector< std::string > tokens = split_string(line);
            if (tokens.size() != 4 && tokens.size() != 5) {
                // Skip malfomred lines.
                continue;
            }
            // Add the vertex.
            float x = std::stof(tokens[1].c_str());
            float y = std::stof(tokens[2].c_str());
            float z = std::stof(tokens[3].c_str());
            vertices.push_back({ x, y, z });
        }
        // Parse UV (start with 'vt ').
        if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
            // UV has the following format:
            // vt [u] [v] [w]
            // where [w] is optional.
            std::vector< std::string > tokens = split_string(line);
            if (tokens.size() != 3 && tokens.size() != 4) {
                // Skip malfomred lines.
                continue;
            }
            // Add the UV token.
            float u = std::stof(tokens[1].c_str());
            float v = std::stof(tokens[2].c_str());
            uvs.push_back({ u, v });
        }
        // Parse normals (start with 'vn ').
        if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
            // Normals have the following format:
            // vn [x] [y] [z]
            std::vector< std::string > tokens = split_string(line);
            if (tokens.size() != 4) {
                // Skip malfomred lines.
                continue;
            }
            // Add the normal.
            float x = std::stof(tokens[1].c_str());
            float y = std::stof(tokens[2].c_str());
            float z = std::stof(tokens[3].c_str());

            normals.push_back(glm::normalize(glm::vec3{ x, y, z }));
        }
        // Parse faces (start with 'f ').
        if (line[0] == 'f' && line[1] == ' ') {
            // Faces have the following format:
            // f [vertexIndex]/[uvIndex]/[normalIndex] [vertexIndex]/[uvIndex]/[normalIndex] [vertexIndex]/[uvIndex]/[normalIndex]
            std::vector< std::string > tokens = split_string(line);
            if (tokens.size() != 4) {
                // Skip malfomred lines.
                continue;
            }
            // Parce each vertex of the triangle.
            for (size_t i = 1; i < tokens.size(); i++) {
                std::string vertex = tokens[i];
                std::vector< std::string > entries = split_string(vertex, '/');
                // Save vertex index.
                int vertexIndex = std::stoi(entries[0]) - 1;
                vertexIndices.push_back(vertexIndex);
                // Save UV index.
                int uvIndex = std::stoi(entries[1]) - 1;
                uvIndices.push_back(uvIndex);
                // Save normal index.
                int normalIndex = std::stoi(entries[2]) - 1;
                normalIndices.push_back(normalIndex);
            }
        }
    }
    // Now we have arrays that represent unique vertices, uvs, normals and index arrays
    // that refer to the corresponding values for each point.
    // We should create vertex, uvs and normal arrays for each point in each face.
    // Note that although the same vertex can be used in different faces it does not mean
    // that tuples of (vertex, uv, normal) are the same.
    // For optimization purposes it makes sense to only keep unique tuples and address them using indices.
    // However in this example we use unindexed buffers.
    for (size_t index : vertexIndices) {
        m_vertices.push_back(vertices[index]);
    }
    for (auto index : uvIndices) {
        m_uvs.push_back(uvs[index]);
    }
    for (auto index : normalIndices) {
        m_normals.push_back(normals[index]);
    }
    // Calculate tangents and bitangents using vertex, uv and normal buffers.
    // Process each triangle, so 3 vertices.
    for (size_t i = 0; i < vertexIndices.size(); i += 3) {
        // Extract vertices.
        glm::vec3 v0 = vertices[vertexIndices[i]];
        glm::vec3 v1 = vertices[vertexIndices[i + 1]];
        glm::vec3 v2 = vertices[vertexIndices[i + 2]];
        // Extract UV coordinates.
        glm::vec2 uv0 = uvs[uvIndices[i]];
        glm::vec2 uv1 = uvs[uvIndices[i + 1]];
        glm::vec2 uv2 = uvs[uvIndices[i + 2]];
        // Calculate vectors from v0 along triangle edges in world coordinate system.
        glm::vec3 deltaPos1 = v1 - v0;
        glm::vec3 deltaPos2 = v2 - v0;
        // Calculate vectors from v0 along triangle edges in texture coordinate system.
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;
        // We should calcualte tangent (T) and bitangent (B) vectors in the world space that go along texture coordinates.
        // In other words we should solve a system of equations:
        //   deltaPos1 = deltaUV1.x * T + deltaUV1.y * B
        //   deltaPos2 = deltaUV2.x * T + deltaUV2.y * B
        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y)*r;
        glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x)*r;
        // Normalize vectors.
        tangent = glm::normalize(tangent);
        bitangent = glm::normalize(bitangent);
        // Tangent and bitangent are the same for all vertices in the same triangle, so simply add them 3 times.
        m_tangents.push_back(tangent);
        m_tangents.push_back(tangent);
        m_tangents.push_back(tangent);
        m_bitangents.push_back(bitangent);
        m_bitangents.push_back(bitangent);
        m_bitangents.push_back(bitangent);
    }
    // In order to be able to jump from the world to texture coordinate system we can use TBN matrix which is
    // | Tx Bx Nx |
    // | Ty By Ny |
    // | Tz Bz Nz |
    // where T - tangent, B - bitangent, N - normal.
    // TBN matrix is orthogonal because this is a rotation matrix, however during calculations we may
    // introduce precision errors and the particular TBN matrix may become not orthogonal anymore.
    // To fix this we should apply Gramm-Schmidt process for mathrix orthogonalization.
    // See https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
    for (size_t i = 0; i < vertexIndices.size(); i++)
    {
        glm::vec3 & n = normals[normalIndices[i]];
        glm::vec3 & t = m_tangents[i];
        glm::vec3 & b = m_bitangents[i];
        t = glm::normalize(t - n * glm::dot(n, t));
        if (glm::dot(glm::cross(n, t), b) < 0.0f){
            t = t * -1.0f;
        }
    }
}

const std::vector< glm::vec3 >& object::vertexes() const
{
    return m_vertices;
}

const std::vector< glm::vec2 >& object::uvs() const
{
    return m_uvs;
}

const std::vector< glm::vec3 >& object::normals() const
{
    return m_normals;
}

std::vector< glm::vec3 > object::tangents() const
{
    return m_tangents;
}

std::vector< glm::vec3 > object::bitangents() const
{
    return m_bitangents;
}

std::vector< std::string > object::split_string(const std::string& str, char delim) const
{
    std::vector< std::string > result;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        result.push_back(token);
    }
    return result;
}

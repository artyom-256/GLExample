#pragma once

#include <fstream>
#include <sstream>
#include <iterator>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

#include <glm/glm.hpp> // glm::vec3, glm::vec4, glm::ivec4, glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

class Object
{
public:
    Object(const char* file) {
        std::ifstream ifs(file);
        if (!ifs) {
            std::cout << "File not opened!";
            return;
        }
        std::string line;
        while (ifs) {
            getline(ifs, line);

            if (line.length() < 3) {
                continue;
            }

            std::cout << "LINE> " << line << std::endl;

            if (line[0] == 'v' && line[1] == ' ') {
                // Vertex.

                std::vector< std::string > tokens = splitString(line);
                if (tokens.size() != 4) {
                    continue;
                }

                float x = std::stof(tokens[1].c_str());
                float y = std::stof(tokens[2].c_str());
                float z = std::stof(tokens[3].c_str());

                m_vertexes.push_back( { x, y, z } );
            }

            if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
                // UV.

                std::vector< std::string > tokens = splitString(line);
                if (tokens.size() != 3) {
                    continue;
                }

                float u = std::stof(tokens[1].c_str());
                float v = std::stof(tokens[2].c_str());

                m_uvs.push_back( { u, v } );
            }

            if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
                // Normales.

                std::vector< std::string > tokens = splitString(line);
                if (tokens.size() != 4) {
                    continue;
                }

                float x = std::stof(tokens[1].c_str());
                float y = std::stof(tokens[2].c_str());
                float z = std::stof(tokens[3].c_str());

                m_normales.push_back( { x, y, z } );
            }

            if (line[0] == 'f' && line[1] == ' ') {
                std::vector< std::string > tokens = splitString(line);

                if (tokens.size() != 4) {
                    continue;
                }

                for (int i = 1; i < tokens.size(); i++) {
                    std::string vertex = tokens[i];
                    std::vector< std::string > entries = splitString(vertex, '/');

                    int index = std::stoi(entries[0]) - 1;
                    m_indices.push_back(index);

                    int index_uv = std::stoi(entries[1]) - 1;
                    m_indices_uv.push_back(index_uv);

                    int index_norm = std::stoi(entries[2]) - 1;
                    m_indices_norm.push_back(index_norm);
                }
            }
        }

        m_vertexes_f_size = m_indices.size() * 3;

        m_vertexes_f.reset(new float[m_vertexes_f_size]);

        int i = 0;
        for (int index : m_indices) {
            if (i >= m_vertexes_f_size) {
                break;
            }
            m_vertexes_f[i++] = m_vertexes[index].x;
            m_vertexes_f[i++] = m_vertexes[index].y;
            m_vertexes_f[i++] = m_vertexes[index].z;
        }

        m_uvs_f.reset(new float[m_indices_uv.size() * 2]);

        i = 0;
        for (auto index : m_indices_uv) {
            m_uvs_f[i++] = m_uvs[index].x;
            m_uvs_f[i++] = m_uvs[index].y;
        }

        m_uv_f_size = m_indices_uv.size() * 2;

        i = 0;

        std::cout << "&&& " << m_normales.size();

        m_normales_f.reset(new float[m_indices_norm.size() * 3]);

        for (auto index : m_indices_norm) {
            std::cout << "### " << index << " " << m_normales[index].x << " " << m_normales[index].y << " " << m_normales[index].z;
            m_normales_f[i++] = m_normales[index].x;
            m_normales_f[i++] = m_normales[index].y;
            m_normales_f[i++] = m_normales[index].z;
        }

        m_normales_size = m_indices_norm.size() * 3;


        // Calculate tangents and bitangents

        for (int i = 0; i < m_indices.size(); i+=3) {
            glm::vec3 v0 = m_vertexes[m_indices[i]];
            glm::vec3 v1 = m_vertexes[m_indices[i + 1]];
            glm::vec3 v2 = m_vertexes[m_indices[i + 2]];

            glm::vec2 uv0 = m_uvs[m_indices_uv[i]];
            glm::vec2 uv1 = m_uvs[m_indices_uv[i + 1]];
            glm::vec2 uv2 = m_uvs[m_indices_uv[i + 2]];

//            glm::vec3 nrm = m_normales[m_indices_norm[i]];

//            glm::vec3 c1 = cross(nrm, glm::vec3(0.0, 0.0, 1.0));
//            glm::vec3 c2 = cross(nrm, glm::vec3(0.0, 1.0, 0.0));
//            glm::vec3 tangent = v0 + v1 + v2;
//            glm::vec3 bitangent = glm::cross(nrm, tangent);

//            tangent = glm::normalize(tangent);
//            bitangent = glm::normalize(bitangent);

            // стороны треугольника
            glm::vec3 deltaPos1 = v1-v0;
            glm::vec3 deltaPos2 = v2-v0;
            // дельта UV
            glm::vec2 deltaUV1 = uv1-uv0;
            glm::vec2 deltaUV2 = uv2-uv0;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            if (i == 3 * 5 || i == 3 * 11) {
                r = -r;
            }
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y)*r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x)*r;

            m_tangents.push_back(tangent);
            m_tangents.push_back(tangent);
            m_tangents.push_back(tangent);

            m_bitangents.push_back(bitangent);
            m_bitangents.push_back(bitangent);
            m_bitangents.push_back(bitangent);
        }

        // See "Going Further"
        for (unsigned int i=0; i<m_indices.size(); i+=1 )
        {
            glm::vec3 & n = m_normales[m_indices_norm[i]];
            glm::vec3 & t = m_tangents[i];
            glm::vec3 & b = m_bitangents[i];

            // Gram-Schmidt orthogonalize
            t = glm::normalize(t - n * glm::dot(n, t));

            // Calculate handedness
            if (glm::dot(glm::cross(n, t), b) < 0.0f){
                t = t * -1.0f;
            }
        }

    }

    float* getVertexes() {
        return m_vertexes_f.get();
    }
    int numVertexes() {
        return m_vertexes_f_size;
    }
    float* getUVs() {
        return m_uvs_f.get();
    }
    int numUVs() {
        return m_uv_f_size;
    }
    float* getNormales() {
        return m_normales_f.get();
    }
    int numNormales() {
        return m_normales_size;
    }
    std::vector< glm::vec3 > getTangents() {
        return m_tangents;
    }
    std::vector< glm::vec3 > getBitangents() {
        return m_bitangents;
    }
private:
    struct vertex {
        float x, y, z;
    };

    std::vector< std::string > splitString(const std::string& str, char delim = ' ')
    {
        std::vector< std::string > result;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delim)) {
            result.push_back(token);
        }
        return result;
    }

    int m_vertexes_f_size;
    int m_uv_f_size;
    int m_normales_size;

    std::vector< glm::vec3 > m_vertexes;
    std::vector< glm::vec2 > m_uvs;
    std::vector< int > m_indices;
    std::vector< int > m_indices_uv;
    std::vector< int > m_indices_norm;
    std::vector< glm::vec3 > m_normales;
    std::vector< glm::vec3 > m_tangents;
    std::vector< glm::vec3 > m_bitangents;

    std::unique_ptr< float[] > m_vertexes_f;
    std::unique_ptr< float[] > m_uvs_f;
    std::unique_ptr< float[] > m_indices_f;
    std::unique_ptr< float[] > m_normales_f;
};

#pragma once

#include <fstream>
#include <sstream>
#include <iterator>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

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

            if (line.length() < 2) {
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
    }

    float* getVertexes() {
        return m_vertexes_f.get();
    }
    int numVertexes() {
        return m_vertexes_f_size;
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

    std::vector< vertex > m_vertexes;
    std::vector< int > m_indices;

    std::unique_ptr< float[] > m_vertexes_f;
    std::unique_ptr< float > m_indices_f;
};

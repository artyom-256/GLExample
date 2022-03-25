#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

/**
 * Parse wavefront .obj file and extracts vertices, UVs and normales.
 * See https://en.wikipedia.org/wiki/Wavefront_.obj_file
 */
class object
{
public:
    /**
     * Constructor. Read a 3D object from the provided file.
     * @param fileName File to read.
     */
    object(const char* fileName);
    /**
     * Return an array of vertices.
     * @return Array of vertices.
     */
    const std::vector< glm::vec3 >& vertexes() const;
    /**
     * Return an array of texture coordinates.
     * @return Array of texture coordinates.
     */
    const std::vector< glm::vec2 >& uvs() const;
    /**
     * Return an array of normales.
     * @return Array of vertices.
     */
    const std::vector< glm::vec3 >& normals() const;
    /**
     * Return an array of tangents.
     * @return Array of tangents.
     */
    std::vector< glm::vec3 > tangents() const;
    /**
     * Return an array of bitangents.
     * @return Array of bitangents.
     */
    std::vector< glm::vec3 > bitangents() const;

private:
    /**
     * Helper function to split a string into tokens using the given delimiter.
     * @param str String to split.
     * @param delim Delimiter.
     * @return Array of tokens.
     */
    std::vector< std::string > split_string(const std::string& str, char delim = ' ') const;

    /**
     * Array of vertices.
     */
    std::vector< glm::vec3 > m_vertices;
    /**
     * Array of texture coordinates.
     */
    std::vector< glm::vec2 > m_uvs;
    /**
     * Array of normales.
     */
    std::vector< glm::vec3 > m_normals;
    /**
     * Array of tangents.
     */
    std::vector< glm::vec3 > m_tangents;
    /**
     * Array of bitangents.
     */
    std::vector< glm::vec3 > m_bitangents;
};

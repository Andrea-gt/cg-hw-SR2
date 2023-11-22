#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <algorithm>
#include <glm/glm.hpp>
#include "Face.h"

bool loadOBJ(
        const char* path,
        std::vector<glm::vec3>& out_vertices,
        std::vector<glm::vec3>& out_normals,
        std::vector<glm::vec3>& out_texcoords,
        std::vector<Face>& out_faces
)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cout << "Failed to open the file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string lineHeader;
        iss >> lineHeader;

        if (lineHeader == "v")
        {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            out_vertices.push_back(vertex);
        }
        else if (lineHeader == "vn")
        {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            out_normals.push_back(normal);
        }
        else if (lineHeader == "vt")
        {
            glm::vec3 tex;
            iss >> tex.x >> tex.y >> tex.z;
            out_texcoords.push_back(tex);
        }
        else if (lineHeader == "f")
        {
            Face face;
            for (int i = 0; i < 3; ++i)
            {
                std::string faceData;
                iss >> faceData;

                std::replace(faceData.begin(), faceData.end(), '/', ' ');

                std::istringstream faceDataIss(faceData);
                faceDataIss >> face.vertexIndices[i] >> face.texIndices[i] >> face.normalIndices[i];

                // obj indices are 1-based, so convert to 0-based
                face.vertexIndices[i]--;
                face.normalIndices[i]--;
                face.texIndices[i]--;
            }
            out_faces.push_back(face);
        }
    }

    return true;
}

std::vector<Vertex> setupVertexArray(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& norms, const std::vector<Face>& faces)
{
    std::vector<Vertex> vertexArray;

    // For each face
    for (const auto& face : faces)
    {
        // For each vertex in the face
        for (int i = 0; i < 3; ++i)
        {
            int vertexIndex = face.vertexIndices[i];
            int normalIndex = face.normalIndices[i];

            // Ensure that the indices are within bounds
            if (vertexIndex < 0 || vertexIndex >= static_cast<int>(vertices.size()) ||
                normalIndex < 0 || normalIndex >= static_cast<int>(norms.size()))
            {
                // Handle out-of-bounds indices here (e.g., throw an error or skip the face)
                continue;
            }

            // Get the vertex position and normal from the input arrays using the indices from the face
            glm::vec3 vertexPosition = vertices[vertexIndex];
            glm::vec3 vertexNorm = norms[normalIndex];

            // Add the vertex position and normal to the vertex array
            vertexArray.push_back(Vertex{vertexPosition, Color(255,255,255), vertexNorm});
        }
    }

    return vertexArray;
}
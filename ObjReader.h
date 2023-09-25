#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <string>
#include <glm/glm.hpp>
#include "Face.h"

bool loadOBJ(const std::string& path, std::vector<glm::vec3>& out_vertices, std::vector<Face>& out_faces)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cout << "Failed to open the file: " << path << std::endl;
        return false;
    }

    std::string line;
    std::istringstream iss;
    std::string lineHeader;
    glm::vec3 vertex;
    Face face;

    while (std::getline(file, line))
    {
        iss.clear();
        iss.str(line);
        iss >> lineHeader;

        if (lineHeader == "v")
        {
            iss >> vertex.x >> vertex.y >> vertex.z;
            out_vertices.push_back(vertex);
        }
        else if (lineHeader == "f")
        {
            std::array<int, 3> vertexIndices;
            while (iss >> lineHeader)
            {
                std::istringstream tokenstream(lineHeader);
                std::string token;
                std::array<int, 3> vertexIndices;

                // Read all three values separated by '/'
                for (int i = 0; i < 3; ++i) {
                    std::getline(tokenstream, token, '/');
                    vertexIndices[i] = std::stoi(token) - 1;
                }

                face.vertexIndices.push_back(vertexIndices);
            }
            out_faces.push_back(face);
            face.vertexIndices.clear();
        }
    }

    return true;
}


std::vector<glm::vec3> setupVertexArray(const std::vector<glm::vec3>& vertices, const std::vector<Face>& faces)
{
    std::vector<glm::vec3> vertexArray;

    // For each face
    for (const auto& face : faces)
    {
        // For each vertex in the face
        for (const auto& vertexIndices : face.vertexIndices)
        {
            // Get the vertex position and normal from the input arrays using the indices from the face
            glm::vec3 vertexPosition = vertices[vertexIndices[0]];

            // Add the vertex position and normal to the vertex array
            vertexArray.push_back(vertexPosition);
        }
    }

    return vertexArray;
}
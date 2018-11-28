#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class UVCylinder {
private:
	unsigned int numSegments;
	float radius;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> textureCoords;
	std::vector<glm::vec3> normals;
	unsigned int numVertices;
	unsigned int numTriangles;
	unsigned int* triangleIndices;

public:
	UVCylinder(float radius, unsigned int numSegments) : radius(radius), numSegments(numSegments) {}

	glm::vec3* getPositions();
	glm::vec2* getTextureCoords();
	glm::vec3* getNormals();

	unsigned int getNumVertices();
	unsigned int getNumTriangles();

	unsigned int* getTriangleIndices();

	float getRadius();
	unsigned int getNumSegments();

	void save(const std::string filename);
};
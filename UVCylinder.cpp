#include "UVCylinder.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <fstream>

void UVCylinder::save(const std::string filename) {
	float deltaTheta = (M_PI * 2.0) / this->numSegments;

	std::cout << "Generating geometry and saving to " << filename << std::endl;

	std::vector<glm::vec3> positions;
	std::vector<unsigned int> vertexIndices;
	std::vector<glm::vec2> textureCoords;
	std::vector<glm::vec3> normals;
	

	for (float face = -1.0f; face <= 1.0f; face += 2.0f) {

		int initialPosition = positions.size();

		positions.push_back(glm::vec3(0.0f, 0.0f, face));
		normals.push_back(glm::normalize(glm::vec3(0.0f, 0.0f, face)));

		for (int i = 0; i < numSegments; i++) {
			glm::vec3 pos(cos(deltaTheta *  i), sin(deltaTheta * i), face);

			positions.push_back(pos);
			normals.push_back(glm::normalize(glm::vec3(0.0f, 0.0f, face)));
		}

		for (int i = 0; i < numSegments; i++) {
			vertexIndices.push_back(initialPosition);

			int point1 = i + 1;
			int point2 = i + 2;

			if (point1 > numSegments)
				point1 -= numSegments;
			if (point2 > numSegments)
				point2 -= numSegments;

			vertexIndices.push_back(initialPosition + point1);
			vertexIndices.push_back(initialPosition + point2);
		}
	}

	for (float face = -1.0f; face <= 1.0f; face += 2.0f) {

		for (int i = 0; i < numSegments; i++) {
			glm::vec3 pos(cos(deltaTheta *  i), sin(deltaTheta * i), face);

			positions.push_back(pos);
			normals.push_back(glm::normalize(pos));
		}
	}

	int offset = (numSegments + 1) * 2;
	int circle1 = offset;
	int circle2 = offset + numSegments;

	for (int i = 0; i < numSegments; i++) {
		int a = circle1 + i;
		int b = circle2 + i;
		int c = circle1 + i + 1;
		int d = circle2 + i + 1;

		if (c == circle1 + numSegments)
			c -= numSegments;

		if (d == circle2 + numSegments)
			d -= numSegments;

		vertexIndices.push_back(a);
		vertexIndices.push_back(b);
		vertexIndices.push_back(d);

		vertexIndices.push_back(a);
		vertexIndices.push_back(c);
		vertexIndices.push_back(d);
	}



	for (int i = 0; i < positions.size(); i++) {
		textureCoords.push_back(glm::vec2(0.0f, 0.0f));
	}
	std::ofstream fileOut(filename.c_str());

	if (!fileOut.is_open()) {
		return;
	}

	for (unsigned int i = 0; i < positions.size(); i++) {
		fileOut << "v " << positions[i].x << " " << positions[i].y << " " << positions[i].z << std::endl;
	}

	for (unsigned int i = 0; i < textureCoords.size(); i++) {
		fileOut << "vt " << textureCoords[i].s << " " << textureCoords[i].t << std::endl;
	}

	for (unsigned int i = 0; i < normals.size(); i++) {
		fileOut << "vn " << normals[i].x << " " << normals[i].y << " " << normals[i].z << std::endl;
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i += 3) {
		fileOut << "f " << vertexIndices[i] + 1 << "/" << vertexIndices[i] + 1 << "/" << vertexIndices[i] + 1 << " ";
		fileOut << vertexIndices[i + 1] + 1 << "/" << vertexIndices[i + 1] + 1 << "/" << vertexIndices[i + 1] + 1 << " ";
		fileOut << vertexIndices[i + 2] + 1 << "/" << vertexIndices[i + 2] + 1 << "/" << vertexIndices[i + 2] + 1 << std::endl;
	}

	fileOut.close();
}



glm::vec3* UVCylinder::getPositions() { return this->positions.data(); }
glm::vec2* UVCylinder::getTextureCoords() { return this->textureCoords.data(); }
glm::vec3* UVCylinder::getNormals() { return this->normals.data(); }
unsigned int UVCylinder::getNumVertices() { return this->numVertices; }
unsigned int UVCylinder::getNumTriangles() { return this->numTriangles; }
unsigned int* UVCylinder::getTriangleIndices() { return this->triangleIndices; }
float UVCylinder::getRadius() { return this->radius; }
unsigned int UVCylinder::getNumSegments() { return this->numSegments; }


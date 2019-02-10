#include "raytracer.h"

#include <glm/gtc/type_ptr.hpp>

#include "shaderloading.h"
#include "primitives.h"

Raytracer::Raytracer(int width, int height)
    : m_id(createComputeProgram("raytracer.comp"))
{
    glUseProgram(m_id);

    GLint workGroupSize[3];

    glGetProgramiv(m_id, GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize);
    m_workGroupSizeX = workGroupSize[0];
    m_workGroupSizeY = workGroupSize[1];
    m_width = width;
    m_height = height;

    getUniformLocations();

    glUniform1i(m_screenWidthLocation, m_width);
    glUniform1i(m_screenHeightLocation, m_height);

    for (int i = 0; i < (int)PrimitiveType::count; i++) {
        glGenBuffers(1, &m_buffers[i]);
    }

    glUseProgram(0);
}

Raytracer::~Raytracer()
{
    glDeleteProgram(m_id);
}

void Raytracer::dispatch(Camera& cam)
{
    glUseProgram(m_id);

    loadVec3(m_camPosLocation, cam.getPosition());
    loadVec3(m_p0Location, cam.getp0());
    loadVec3(m_p0p1Location, cam.getp0p1());
    loadVec3(m_p0p2Location, cam.getp0p2());

    glDispatchCompute(m_width / m_workGroupSizeX, m_height / m_workGroupSizeY, 1);
    glUseProgram(0);
}

void Raytracer::getUniformLocations()
{
    m_camPosLocation = glGetUniformLocation(m_id, "cam_pos");
    m_p0Location = glGetUniformLocation(m_id, "p0");
    m_p0p1Location = glGetUniformLocation(m_id, "p0p1");
    m_p0p2Location = glGetUniformLocation(m_id, "p0p2");
    m_screenWidthLocation = glGetUniformLocation(m_id, "screen_width");
    m_screenHeightLocation = glGetUniformLocation(m_id, "screen_height");
}

void* Raytracer::mapBuffer(GLuint bufID, GLuint layout,
                           size_t bufSize, size_t elemSize)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, layout, bufID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufSize*elemSize,
                 nullptr, GL_STATIC_DRAW);
    return glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufSize*elemSize,
                            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
}

void Raytracer::loadVec3(GLuint loc, const glm::vec3& vec)
{
    glUniform3f(loc, vec.x, vec.y, vec.z);
}

void Raytracer::loadScene(Scene& scene)
{
    glUseProgram(m_id);

    size_t primitiveCount = scene.getPrimitiveCount(PrimitiveType::primitive);
    if (primitiveCount > 0) {
        struct Primitive* primitives =
            (struct Primitive*) mapBuffer(m_buffers[(int)PrimitiveType::primitive], 1,
                                          primitiveCount, sizeof(Primitive));
        for (size_t i = 0; i < primitiveCount; i++) {
            primitives[i] = *(struct Primitive*) scene.getPrimitive(PrimitiveType::primitive, i);
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glUniform1i(glGetUniformLocation(m_id, "primitiveCount"), (GLuint)primitiveCount);
    }

    size_t sphereCount = scene.getPrimitiveCount(PrimitiveType::sphere);
    if (sphereCount > 0) {
        struct Sphere* spheres =
            (struct Sphere*) mapBuffer(m_buffers[(int)PrimitiveType::sphere], 2,
                                       sphereCount, sizeof(Sphere));
        for (size_t i = 0; i < sphereCount; i++) {
            spheres[i] = *(struct Sphere*) scene.getPrimitive(PrimitiveType::sphere, i);
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    size_t boxCount = scene.getPrimitiveCount(PrimitiveType::box);
    if (boxCount > 0) {
        struct Box* boxes =
            (struct Box*) mapBuffer(m_buffers[(int)PrimitiveType::box], 3,
                                      boxCount, sizeof(Box));
        for (size_t i = 0; i < boxCount; i++) {
            boxes[i] = *(struct Box*) scene.getPrimitive(PrimitiveType::box, i);
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    glUseProgram(0);
}

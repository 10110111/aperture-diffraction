#pragma once

#include <memory>
#include <QOpenGLShaderProgram>

class QOpenGLFunctions_3_3_Core;
class GLSLCosineQualityChecker
{
    static constexpr int width = 128, height = 1;

    QOpenGLFunctions_3_3_Core& gl;
    GLuint vao, vbo;
    GLuint texFBO;
    GLuint inputTexture;
    std::unique_ptr<QOpenGLShaderProgram> program;
    GLuint fbo;

    void loadShaders();
    void setupBuffers();
    void setupRenderTarget(int width, int height);
    void setupInputTextures(int width, int height);
public:
    GLSLCosineQualityChecker(QOpenGLFunctions_3_3_Core& gl);
    bool isGood();
    ~GLSLCosineQualityChecker();
};

#pragma once

#include <QVector4D>
#include <QOpenGLWindow>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_3_3_Core>

class Canvas : public QOpenGLWindow, public QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    Canvas(UpdateBehavior updateBehavior=NoPartialUpdate, QWindow* parent=nullptr);
    ~Canvas();

protected:
    void initializeGL() override;
    void paintGL() override;
private:
    void setupBuffers();
    void setupShaders();
    void setupWavelengths();
    void setupRenderTarget();
    QMatrix4x4 radianceToLuminance(unsigned texIndex) const;

private:
    GLuint vao_=0;
    GLuint vbo_=0;
    GLuint luminanceFBO_=0;
    GLuint luminanceTexture_=0;
    GLuint glareFBOs_[2] = {};
    GLuint glareTextures_[2] = {};
    int lastWidth_=0, lastHeight_=0;
    QOpenGLShaderProgram glareProgram_;
    QOpenGLShaderProgram radianceToLuminance_;
    QOpenGLShaderProgram luminanceToScreen_;
    std::vector<QVector4D> wavelengths_;
};

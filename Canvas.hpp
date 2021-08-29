#pragma once

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
    void resizeGL(int w, int h) override;
    void initializeGL() override;
    void paintGL() override;
private:
    void setupBuffers();
    void setupShaders();
    void makeImageTexture();
    void setupRenderTarget();

private:
    GLuint vao_=0;
    GLuint vbo_=0;
    GLuint mainFBO_;
    GLuint glareFBOs_[2] = {};
    GLuint glareTextures_[2] = {};
    QOpenGLShaderProgram glareProgram_;
    QOpenGLShaderProgram luminanceToScreenRGB_;
};

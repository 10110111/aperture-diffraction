#pragma once

#include <cmath>
#include <QVector4D>
#include <QOpenGLWindow>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_3_3_Core>

class ToolsWidget;
class Canvas : public QOpenGLWindow, public QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    Canvas(ToolsWidget* tools, UpdateBehavior updateBehavior=NoPartialUpdate, QWindow* parent=nullptr);
    ~Canvas();

protected:
    void initializeGL() override;
    void paintGL() override;
private:
    void setupBuffers();
    void setupShaders();
    void setupWavelengths();
    void setupRenderTarget();
    QVector4D radianceToLuminance(unsigned texIndex) const;

private:
    ToolsWidget* tools_=nullptr;
    double prevScale_=NAN;
    double prevRotationAngle_=NAN;
    int prevPointCount_=-1;
    int prevSampleCount_=-1;
    int prevArcPointCount_=-1;
    double prevCurvatureRadius_=NAN;
    GLuint vao_=0;
    GLuint vbo_=0;
    GLuint luminanceFBO_=0;
    GLuint luminanceTexture_=0;
    int lastWidth_=0, lastHeight_=0;
    QOpenGLShaderProgram glareProgram_;
    QOpenGLShaderProgram luminanceToScreen_;
    std::vector<float> wavelengths_;
    bool needRedraw_=true;
};

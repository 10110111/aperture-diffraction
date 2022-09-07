#pragma once

#include <memory>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_3_3_Core>

class ToolsWidget;
class ApertureOutline : public QOpenGLWidget, public QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    ApertureOutline(ToolsWidget* tools, QWidget* parent = nullptr);

protected:
    void paintGL() override;
    void initializeGL() override;

private:
    void setupBuffers();
    void setupShaders();

private:
    ToolsWidget* tools_;
    std::unique_ptr<QOpenGLShaderProgram> renderProgram_;
    GLuint vao_=0;
    GLuint vbo_=0;
};

#include "ApertureOutline.hpp"
#include <algorithm>
#include <QPainter>
#include <QMessageBox>
#include <QOpenGLFunctions_3_3_Core>
#include <glm/glm.hpp>
#include "ToolsWidget.hpp"
#include "common.hpp"

ApertureOutline::ApertureOutline(ToolsWidget* tools, QWidget* parent)
    : QOpenGLWidget(parent)
    , tools_(tools)
{
    auto format = makeGLSurfaceFormat();
    format.setSamples(8);
    setFormat(format);
}

void ApertureOutline::initializeGL()
{
    if(!initializeOpenGLFunctions())
    {
        QMessageBox::critical(nullptr, tr("Error initializing OpenGL"), tr("Failed to initialize OpenGL %1.%2 functions")
                                    .arg(OPENGL_MAJOR_VERSION)
                                    .arg(OPENGL_MINOR_VERSION));
        return;
    }

    setupBuffers();
    setupShaders();

    glFinish();
}

void ApertureOutline::setupBuffers()
{
    if(vao_)
        glDeleteVertexArrays(1, &vao_);
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    if(vbo_)
        glDeleteBuffers(1, &vbo_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    constexpr GLuint attribIndex=0;
    constexpr int coordsPerVertex=2;
    glVertexAttribPointer(attribIndex, coordsPerVertex, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(attribIndex);
    glBindVertexArray(0);
}

void ApertureOutline::setupShaders()
{
    renderProgram_ = std::make_unique<QOpenGLShaderProgram>();

    const char*const vertSrc = 1+R"(
#version 330
in vec3 vertex;
void main()
{
    gl_Position=vec4(vertex,1);
}
)";
    if(!renderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc))
       QMessageBox::critical(nullptr, tr("Shader compile failure"),
                             tr("Failed to compile %1:\n%2").arg("aperture outline vertex shader")
                                                            .arg(renderProgram_->log()));
    const char*const fragSrc = 1+R"(
#version 330
out vec4 color;
void main()
{
    color=vec4(0,0,0,1);
}
)";
    if(!renderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc))
       QMessageBox::critical(nullptr, tr("Error compiling shader"),
                             tr("Failed to compile %1:\n%2").arg("aperture outline fragment shader")
                                                            .arg(renderProgram_->log()));
    if(!renderProgram_->link())
        throw QMessageBox::critical(nullptr, tr("Error linking shader program"),
                                    tr("Failed to link %1:\n%2").arg("aperture outline shader program")
                                                                .arg(renderProgram_->log()));
}

void ApertureOutline::paintGL()
{
    const auto size = (std::min(width(), height()) & -2) - 1;
    const auto topLeft = QPoint((width()-size)/2,(height()-size)/2);
    const auto canvasRect = QRect(topLeft, QSize(size,size));

    const auto bg = palette().color(QPalette::Window);
    glClearColor(bg.red()/255.,bg.green()/255.,bg.blue()/255.,1);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(canvasRect.x(),canvasRect.y(),canvasRect.width(),canvasRect.height());
    glScissor(canvasRect.x(),canvasRect.y(),canvasRect.width(),canvasRect.height());
    glEnable(GL_SCISSOR_TEST);
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    using namespace glm;
    using namespace std;

    const auto sqr = [](auto x){ return x*x; };
    const float PI = acos(-1.);

    const int sampleCount = tools_->sampleCount();
    const int pointCount = tools_->pointCount();
    const float globalRotationAngle = tools_->globalRotationAngle();
    const float curvatureRadius = tools_->curvatureRadius();
    const float arcPointCount = tools_->arcPointCount();

    std::vector<vec2> vertices;
    for(int sampleNumY=0; sampleNumY<sampleCount; ++sampleNumY)
    {
        for(int sampleNumX=0; sampleNumX<sampleCount; ++sampleNumX)
        {
            for(int pointNum=1; pointNum<=pointCount; ++pointNum)
            {
                float phi1 = 2*PI*(pointNum-1)/pointCount + (pointCount%2==1 ? PI/2 : 0) + globalRotationAngle;
                float phi2 = 2*PI* pointNum   /pointCount + (pointCount%2==1 ? PI/2 : 0) + globalRotationAngle;
                vec2 p1=vec2(cos(phi1), sin(phi1));
                vec2 p2=vec2(cos(phi2), sin(phi2));
                vec2 midPoint = (p1+p2)/2.f;
                float arcCenterDistFromMid = sqrt(sqr(curvatureRadius)-dot(p1-p2,p1-p2)/4);
                vec2 arcCenter = midPoint * (1-arcCenterDistFromMid/length(midPoint));
                float arcPhi1 = atan(p1.y-arcCenter.y, p1.x-arcCenter.x);
                float arcPhi2 = atan(p2.y-arcCenter.y, p2.x-arcCenter.x);
                if(arcPhi2<arcPhi1) arcPhi2 += 2*PI;
                for(float arcPointNum=0; arcPointNum<=arcPointCount; ++arcPointNum)
                {
                    float angle1=arcPhi1+(arcPhi2-arcPhi1)* arcPointNum   /(arcPointCount+1);
                    float angle2=arcPhi1+(arcPhi2-arcPhi1)*(arcPointNum+1)/(arcPointCount+1);
                    vec2 arcP1 = arcCenter + curvatureRadius*vec2(cos(angle1), sin(angle1));
                    vec2 arcP2 = arcCenter + curvatureRadius*vec2(cos(angle2), sin(angle2));
                    vertices.push_back(arcP1);
                    vertices.push_back(arcP2);
                }
            }
        }
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof vertices[0], vertices.data(), GL_DYNAMIC_DRAW);

    renderProgram_->bind();
    glDrawArrays(GL_LINES, 0, vertices.size());
    renderProgram_->release();
    glBindVertexArray(0);
}

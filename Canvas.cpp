#include "Canvas.hpp"
#include <cmath>
#include <cstring>
#include <glm/glm.hpp>
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QtConcurrent>

constexpr double degree = M_PI/180;
constexpr int OPENGL_MAJOR_VERSION=3;
constexpr int OPENGL_MINOR_VERSION=3;
static QSurfaceFormat makeFormat()
{
    QSurfaceFormat format;
    format.setVersion(OPENGL_MAJOR_VERSION,OPENGL_MINOR_VERSION);
    format.setProfile(QSurfaceFormat::CoreProfile);
    return format;
}

Canvas::Canvas(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFormat(makeFormat());
    setFocusPolicy(Qt::StrongFocus);
}

void Canvas::setupBuffers()
{
    if(!vao_)
        glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    if(!vbo_)
        glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const GLfloat vertices[]=
    {
        -1, -1,
         1, -1,
        -1,  1,
         1,  1,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
    constexpr GLuint attribIndex=0;
    constexpr int coordsPerVertex=2;
    glVertexAttribPointer(attribIndex, coordsPerVertex, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(attribIndex);
    glBindVertexArray(0);
}

void Canvas::setupShaders()
{
    {
        const char*const vertSrc = 1+R"(
#version 330
in vec3 vertex;
void main()
{
    gl_Position=vec4(vertex,1);
}
)";
        if(!glareProgram_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc))
           QMessageBox::critical(this, tr("Shader compile failure"),
                                 tr("Failed to compile %1:\n%2").arg("glare vertex shader").arg(glareProgram_.log()));
        const char*const fragSrc = 1+R"(
#version 330
uniform sampler2D luminanceXYZW;
uniform vec2 stepDir;
out vec4 XYZW;

float weight(const float x)
{
    const float a=0.955491103831962;
    const float b=0.0111272240420095;
    return abs(x)<0.5 ? a : b/(x*x);
}

void main()
{
    vec2 size = textureSize(luminanceXYZW, 0);
    vec2 pos = gl_FragCoord.st-vec2(0.5);
    if(stepDir.x*stepDir.y >= 0)
    {
        vec2 dir = stepDir.x<0 || stepDir.y<0 ? -stepDir : stepDir;
        float stepCountBottomLeft = 1+ceil(min(pos.x/dir.x, pos.y/dir.y));
        float stepCountTopRight = 1+ceil(min((size.x-pos.x-1)/dir.x, (size.x-pos.y-1)/dir.y));

        XYZW = weight(0) * texture(luminanceXYZW, gl_FragCoord.st/size);
        for(float dist=1; dist<stepCountBottomLeft; ++dist)
            XYZW += weight(dist) * texture(luminanceXYZW, (gl_FragCoord.st-dir*dist)/size);
        for(float dist=1; dist<stepCountTopRight; ++dist)
            XYZW += weight(dist) * texture(luminanceXYZW, (gl_FragCoord.st+dir*dist)/size);
    }
    else
    {
        vec2 dir = stepDir.x<0 ? -stepDir : stepDir;
        float stepCountTopLeft = 1+ceil(min(pos.x/dir.x, (size.y-pos.y-1)/-dir.y));
        float stepCountBottomRight = 1+ceil(min((size.x-pos.x-1)/dir.x, pos.y/-dir.y));

        XYZW = weight(0) * texture(luminanceXYZW, gl_FragCoord.st/size);
        for(float dist=1; dist<stepCountTopLeft; ++dist)
            XYZW += weight(dist) * texture(luminanceXYZW, (gl_FragCoord.st-dir*dist)/size);
        for(float dist=1; dist<stepCountBottomRight; ++dist)
            XYZW += weight(dist) * texture(luminanceXYZW, (gl_FragCoord.st+dir*dist)/size);
    }
}
)";
        if(!glareProgram_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc))
           QMessageBox::critical(this, tr("Error compiling shader"),
                                 tr("Failed to compile %1:\n%2").arg("glare fragment shader").arg(glareProgram_.log()));
        if(!glareProgram_.link())
            throw QMessageBox::critical(this, tr("Error linking shader program"),
                                        tr("Failed to link %1:\n%2").arg("glare shader program").arg(glareProgram_.log()));
    }
    {
        const char*const vertSrc = 1+R"(
#version 330
in vec3 vertex;
out vec2 texCoord;
void main()
{
    texCoord=(vertex.xy+vec2(1))/2;
    gl_Position=vec4(vertex,1);
}
)";
        if(!luminanceToScreenRGB_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc))
            QMessageBox::critical(this, tr("Error compiling shader"),
                                  tr("Failed to compile %1:\n%2").arg("luminance-to-sRGB vertex shader").arg(glareProgram_.log()));

        const char*const fragSrc = 1+R"(
#version 330
uniform float exposure;
uniform sampler2D luminanceXYZW;
in vec2 texCoord;
out vec4 color;

vec3 clip(vec3 rgb)
{
    return sqrt(tanh(rgb*rgb));
}

vec3 sRGBTransferFunction(const vec3 c)
{
    return step(0.0031308,c)*(1.055*pow(c, vec3(1/2.4))-0.055)+step(-0.0031308,-c)*12.92*c;
}

void main()
{
    vec3 XYZ=texture(luminanceXYZW, texCoord).xyz;
    const mat3 XYZ2sRGBl=mat3(vec3(3.2406,-0.9689,0.0557),
                              vec3(-1.5372,1.8758,-0.204),
                              vec3(-0.4986,0.0415,1.057));
    vec3 rgb=XYZ2sRGBl*XYZ*exposure;
    vec3 clippedRGB = clip(rgb);
    vec3 srgb=sRGBTransferFunction(clippedRGB);
    color=vec4(srgb,1);
}
)";
        if(!luminanceToScreenRGB_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc))
            QMessageBox::critical(this, tr("Error compiling shader"),
                                  tr("Failed to compile %1:\n%2").arg("luminance-to-sRGB fragment shader").arg(luminanceToScreenRGB_.log()));
        if(!luminanceToScreenRGB_.link())
            throw QMessageBox::critical(this, tr("Error linking shader program"),
                                        tr("Failed to link %1:\n%2").arg("luminance-to-sRGB shader program").arg(luminanceToScreenRGB_.log()));
    }
}

void Canvas::makeImageTexture()
{
    const int w=width(), h=height();
    glBindTexture(GL_TEXTURE_2D, glareTextures_[1]);
    std::vector<glm::vec4> image(w*h);
    const int x=w/2, y=h/2;
    image[w*y+x]=glm::vec4(1e6);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, image.data());
}

void Canvas::setupRenderTarget()
{
    if(!glareTextures_[0])
        glGenTextures(std::size(glareTextures_), glareTextures_);
    for(unsigned n=0; n<std::size(glareTextures_); ++n)
    {
        glBindTexture(GL_TEXTURE_2D, glareTextures_[n]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width(), height(), 0, GL_RGBA, GL_FLOAT, nullptr);
        // This is needed to avoid aliasing when sampling along skewed lines
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // We want our convolution filter to sample zeros outside the texture, so clamp to _border_
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    }

    if(!glareFBOs_[0])
        glGenFramebuffers(std::size(glareFBOs_), glareFBOs_);
    for(unsigned n=0; n<std::size(glareFBOs_); ++n)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, glareFBOs_[n]);
        glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,glareTextures_[n],0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Canvas::resizeGL([[maybe_unused]] int w, [[maybe_unused]] int h)
{
    setupRenderTarget();
}

void Canvas::initializeGL()
{
    if(!initializeOpenGLFunctions())
    {
        QMessageBox::critical(this, tr("Error initializing OpenGL"), tr("Failed to initialize OpenGL %1.%2 functions")
                                    .arg(OPENGL_MAJOR_VERSION)
                                    .arg(OPENGL_MINOR_VERSION));
        return;
    }

    setupBuffers();
    setupRenderTarget();
    setupShaders();

    glFinish();
}

Canvas::~Canvas()
{
    makeCurrent();
    if(glareFBOs_[0])
        glDeleteFramebuffers(std::size(glareFBOs_), glareFBOs_);
    if(glareTextures_[0])
        glDeleteTextures(std::size(glareTextures_), glareTextures_);
}

void Canvas::paintGL()
{
    if(!isVisible()) return;

    GLint targetFBO=-1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &targetFBO);

    glViewport(0, 0, width(), height());
    glBindVertexArray(vao_);

    constexpr double angleMin=5*degree;
    constexpr int numAngleSteps=3;
    constexpr double angleStep=360*degree/numAngleSteps;

    makeImageTexture();
    glareProgram_.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glareTextures_[1]);
    glareProgram_.setUniformValue("luminanceXYZW", 0);
    for(int angleStepNum=0; angleStepNum<numAngleSteps; ++angleStepNum)
    {
        // This is needed to avoid aliasing when sampling along skewed lines
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        const auto angle = angleMin + angleStep*angleStepNum;
        glareProgram_.setUniformValue("stepDir", QVector2D(std::cos(angle),std::sin(angle)));
        glBindFramebuffer(GL_FRAMEBUFFER, glareFBOs_[angleStepNum%2]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        // Now use the result of this stage to feed the next stage
        glBindTexture(GL_TEXTURE_2D, glareTextures_[angleStepNum%2]);
    }

    glBindFramebuffer(GL_FRAMEBUFFER,targetFBO);
    luminanceToScreenRGB_.bind();
    luminanceToScreenRGB_.setUniformValue("luminanceXYZW", 0);
    const float exposure=1;
    luminanceToScreenRGB_.setUniformValue("exposure", exposure);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
}

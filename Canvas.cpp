#include "Canvas.hpp"
#include <cmath>
#include <cstring>
#include <glm/glm.hpp>
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QtConcurrent>
#include "cie-xyzw-functions.hpp"
#include "ToolsWidget.hpp"

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

Canvas::Canvas(ToolsWidget* tools, UpdateBehavior updateBehavior, QWindow* parent)
    : QOpenGLWindow(updateBehavior,parent)
    , tools_(tools)
{
    setFormat(makeFormat());
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
           QMessageBox::critical(nullptr, tr("Shader compile failure"),
                                 tr("Failed to compile %1:\n%2").arg("glare vertex shader").arg(glareProgram_.log()));
        const char*const fragSrc = 1+R"(
#version 330
uniform float scale;
uniform float wavelength;
uniform vec4 radianceToLuminance;
uniform vec2 imageSize;
out vec4 XYZW;
const float PI=3.14159265;

float triangleArea(vec2 p1, vec2 p2, vec2 p3)
{
    return p1.y*p2.x + p2.y*p3.x + p1.x*p3.y - p1.x*p2.y - p1.y*p3.x - p2.x*p3.y;
}

float sinc(float x) { return x==0 ? 1 : sin(x)/x; }
float sqr(float x) { return x*x; }

// Ref: Equations (4.1), (4.2), but altering the definition of sinc, in
//      R.M. Sillitto, W. Sillitto, "A Simple Fourier Approach to Fraunhofer Diffraction by Triangular Apertures"
//      http://dx.doi.org/10.1080/713819012
vec2 triangle(vec2 s1, vec2 s2, vec2 s3, vec2 k)
{
    if(k==0) return vec2(1,0);
    vec2 a1=s3-s2;
    vec2 b1=s1-s3;
    vec2 c1=s2-s1;
    float alpha1=dot(a1,k)/2;
    float beta1 =dot(b1,k)/2;

    vec2 a2=s1-s3;
    vec2 b2=s2-s1;
    vec2 c2=s3-s2;
    float alpha2=dot(a2,k)/2;
    float beta2 =dot(b2,k)/2;

    float alpha, beta;
    vec2 originShift;
    // Try to avoid denominator close to zero
    if(abs(alpha1+beta1) > abs(alpha2+beta2))
    {
        alpha=alpha1;
        beta =beta1;
        originShift=s3;
    }
    else
    {
        alpha=alpha2;
        beta =beta2;
        originShift=s1;
    }
    float reY=(alpha*sqr(sinc(alpha))+beta*sqr(sinc(beta)))/(alpha+beta);
    float imY=(sinc(2*beta)-sinc(2*alpha))/(alpha+beta);
    float reShiftExp =  cos(dot(k,originShift));
    float imShiftExp = -sin(dot(k,originShift));
    return vec2(reY*reShiftExp-imY*imShiftExp,
                imY*reShiftExp+reY*imShiftExp);
}

void main()
{
    const vec2 points[]=vec2[](vec2(1., 0.),
                               vec2(0.5, 0.866025403784439),
                               vec2(-0.5, 0.866025403784439),
                               vec2(-1., 0.),
                               vec2(-0.5, -0.866025403784439),
                               vec2(0.5, -0.866025403784439));
    const int pointCount = points.length();
    float XYZW_re=0, XYZW_im=0;
    for(int pointNum=2; pointNum<pointCount; ++pointNum)
    {
        vec2 k = (gl_FragCoord.st - imageSize/2)*scale/wavelength;
        vec2 p1=points[0],
             p2=points[pointNum-1],
             p3=points[pointNum];
        float area = triangleArea(p1,p2,p3);
        vec2 tri = triangle(p1,p2,p3,k);
        XYZW_re += area*tri.x;
        XYZW_im += area*tri.y;
    }
    XYZW = radianceToLuminance*(XYZW_re*XYZW_re+XYZW_im*XYZW_im);
}
)";
        if(!glareProgram_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc))
           QMessageBox::critical(nullptr, tr("Error compiling shader"),
                                 tr("Failed to compile %1:\n%2").arg("glare fragment shader").arg(glareProgram_.log()));
        if(!glareProgram_.link())
            throw QMessageBox::critical(nullptr, tr("Error linking shader program"),
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
        if(!luminanceToScreen_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc))
            QMessageBox::critical(nullptr, tr("Error compiling shader"),
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
        if(!luminanceToScreen_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc))
            QMessageBox::critical(nullptr, tr("Error compiling shader"),
                                  tr("Failed to compile %1:\n%2").arg("luminance-to-sRGB fragment shader").arg(luminanceToScreen_.log()));
        if(!luminanceToScreen_.link())
            throw QMessageBox::critical(nullptr, tr("Error linking shader program"),
                                        tr("Failed to link %1:\n%2").arg("luminance-to-sRGB shader program").arg(luminanceToScreen_.log()));
    }
}

void Canvas::setupRenderTarget()
{
    if(!luminanceTexture_)
        glGenTextures(1, &luminanceTexture_);
    glBindTexture(GL_TEXTURE_2D, luminanceTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width(), height(), 0, GL_RGBA, GL_FLOAT, nullptr);
    // We pass it without resizing
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if(!luminanceFBO_)
        glGenFramebuffers(1, &luminanceFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, luminanceFBO_);
    glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,luminanceTexture_,0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Canvas::setupWavelengths()
{
    constexpr double min=400; // nm
    constexpr double max=700; // nm
    constexpr auto range=max-min;
    constexpr auto count=256;
    for(int i=0;i<count;++i)
        wavelengths_.push_back(min+range*i/(count-1));
}

void Canvas::initializeGL()
{
    if(!initializeOpenGLFunctions())
    {
        QMessageBox::critical(nullptr, tr("Error initializing OpenGL"), tr("Failed to initialize OpenGL %1.%2 functions")
                                    .arg(OPENGL_MAJOR_VERSION)
                                    .arg(OPENGL_MINOR_VERSION));
        return;
    }

    setupBuffers();
    setupRenderTarget();
    setupShaders();
    setupWavelengths();

    glFinish();
}

Canvas::~Canvas()
{
    makeCurrent();
    if(luminanceFBO_)
        glDeleteFramebuffers(1, &luminanceFBO_);
    if(luminanceTexture_)
        glDeleteTextures(1, &luminanceTexture_);
}

QVector4D Canvas::radianceToLuminance(const unsigned texIndex) const
{
    using glm::mat4;
    const auto diag=[](GLfloat x, GLfloat y, GLfloat z, GLfloat w) { return mat4(x,0,0,0,
                                                                                 0,y,0,0,
                                                                                 0,0,z,0,
                                                                                 0,0,0,w); };
    const auto wlCount = wavelengths_.size();
    // Weights for the trapezoidal quadrature rule
    const auto weights = wlCount==4            ? diag(0.5,1,1,0.5) :
                         texIndex==0           ? diag(0.5,1,1,1  ) :
                         texIndex+1==wlCount/4 ? diag(  1,1,1,0.5) :
                                                 diag(  1,1,1,1);
    const auto dlambda = weights * abs(wavelengths_.back()-wavelengths_.front()) / (wlCount-1.f);
    // Ref: Rapport BIPM-2019/05. Principles Governing Photometry, 2nd edition. Sections 6.2, 6.3.
    const auto maxLuminousEfficacy=diag(683.002f,683.002f,683.002f,1700.13f); // lm/W
    const auto ret = maxLuminousEfficacy * wavelengthToXYZW(wavelengths_[texIndex]) * dlambda;
    return QVector4D(ret.x, ret.y, ret.z, ret.w);
}

void Canvas::paintGL()
{
    if(!isVisible()) return;

    if(width()!=lastWidth_ || height()!=lastHeight_)
    {
        needRedraw_=true;
        // NOTE: we don't use QOpenGLWindow::resizeGL(), because it's sometimes called _after_
        // QOpenGLWindow::paintGL() instead of before. This breaks rendering until subsequent
        // repaint, which doesn't happen if nothing triggers it.
        lastWidth_=width();
        lastHeight_=height();
        setupRenderTarget();
    }
    if(prevScale_!=tools_->scale())
    {
        needRedraw_=true;
        prevScale_=tools_->scale();
    }

    glViewport(0, 0, width(), height());
    glBindVertexArray(vao_);

    if(needRedraw_)
    {
        GLint targetFBO=-1;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &targetFBO);

        glBindFramebuffer(GL_FRAMEBUFFER,luminanceFBO_);
        for(unsigned wlIndex=0; wlIndex<wavelengths_.size(); ++wlIndex)
        {
            glareProgram_.bind();
            glareProgram_.setUniformValue("scale", float(std::pow(10., -tools_->scale())));
            glareProgram_.setUniformValue("wavelength", wavelengths_[wlIndex]);
            glareProgram_.setUniformValue("imageSize", QVector2D(width(), height()));
            glareProgram_.setUniformValue("radianceToLuminance", radianceToLuminance(wlIndex));

            glBlendFunc(GL_ONE, GL_ONE);
            if(wlIndex==0)
                glDisable(GL_BLEND);
            else
                glEnable(GL_BLEND);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDisable(GL_BLEND);
            // This sync is required to avoid sudden appearance of black frames starting with
            // some iteration on NVIDIA GTX 750 Ti with Linux binary driver 390.116. Seems like
            // command buffer overflowing, so here we force its execution.
            glFinish();
        }

        glBindFramebuffer(GL_FRAMEBUFFER,targetFBO);
        needRedraw_=false;
    }
    luminanceToScreen_.bind();
    luminanceToScreen_.setUniformValue("exposure", float(std::pow(10., tools_->exposure())));
    glBindTexture(GL_TEXTURE_2D, luminanceTexture_);
    luminanceToScreen_.setUniformValue("luminanceXYZW", 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
}

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
static QMatrix4x4 toQMatrix(glm::mat4 const& m) { return QMatrix4x4(&transpose(m)[0][0]); }

Canvas::Canvas(UpdateBehavior updateBehavior, QWindow* parent)
    : QOpenGLWindow(updateBehavior,parent)
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
uniform sampler2D radiance;
uniform vec2 stepDir;
uniform int stepNum, stepCount;
uniform vec4 wavelengths;
out vec4 XYZW;
const float PI=3.14159265;

vec4 sinc(const vec4 v)
{
    return vec4(v.x==0 ? 1 : sin(v.x)/v.x,
                v.y==0 ? 1 : sin(v.y)/v.y,
                v.z==0 ? 1 : sin(v.z)/v.z,
                v.w==0 ? 1 : sin(v.w)/v.w);
}

float sinc(const float x)
{
    return x==0 ? 1 : sin(x)/x;
}

vec4 weight(const float x)
{
    const float coef=100; // FIXME: give it a sensible name and a sensible value
    vec4 a=coef/wavelengths;
    return sinc(a*x)*sqrt(a/PI);
}

vec4 sample(const vec2 pos)
{
    vec4 tex = texture(radiance, pos/textureSize(radiance, 0));
    // Input image consists of intensities, intermediate results are field values
    return stepNum==0 ? sqrt(tex) : tex;
}

void main()
{
    vec2 size = textureSize(radiance, 0);
    XYZW=vec4(0);
    const float SAMPLES_X=4, SAMPLES_Y=4;
    for(float xSampleN=0.5; xSampleN<SAMPLES_X; ++xSampleN)
    {
        for(float ySampleN=0.5; ySampleN<SAMPLES_Y; ++ySampleN)
        {
            vec2 pos = gl_FragCoord.st+vec2(xSampleN/SAMPLES_X-0.5,ySampleN/SAMPLES_Y-0.5);
            if(stepDir.x*stepDir.y >= 0)
            {
                vec2 dir = stepDir.x<0 || stepDir.y<0 ? -stepDir : stepDir;
                float stepCountBottomLeft = 1+ceil(min(pos.x/dir.x, pos.y/dir.y));
                float stepCountTopRight = 1+ceil(min((size.x-pos.x-1)/dir.x, (size.y-pos.y-1)/dir.y));

                XYZW += weight(0) * sample(pos);
                for(float dist=1; dist<stepCountBottomLeft; ++dist)
                    XYZW += weight(dist) * sample(pos-dir*dist);
                for(float dist=1; dist<stepCountTopRight; ++dist)
                    XYZW += weight(dist) * sample(pos+dir*dist);
            }
            else
            {
                vec2 dir = stepDir.x<0 ? -stepDir : stepDir;
                float stepCountTopLeft = 1+ceil(min(pos.x/dir.x, (size.y-pos.y-1)/-dir.y));
                float stepCountBottomRight = 1+ceil(min((size.x-pos.x-1)/dir.x, pos.y/-dir.y));

                XYZW += weight(0) * sample(pos);
                for(float dist=1; dist<stepCountTopLeft; ++dist)
                    XYZW += weight(dist) * sample(pos-dir*dist);
                for(float dist=1; dist<stepCountBottomRight; ++dist)
                    XYZW += weight(dist) * sample(pos+dir*dist);
            }
        }
    }
    XYZW /= SAMPLES_X*SAMPLES_Y;

    if(stepNum==stepCount-1)
        XYZW *= XYZW;
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
        if(!radianceToLuminance_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc))
            QMessageBox::critical(nullptr, tr("Error compiling shader"),
                                  tr("Failed to compile %1:\n%2").arg("radiance-to-luminance vertex shader").arg(glareProgram_.log()));

        const char*const fragSrc = 1+R"(
#version 330

uniform mat4 radianceToLuminance;
uniform sampler2D radiance;
in vec2 texCoord;
out vec4 luminance;

void main()
{
    luminance=radianceToLuminance*texture(radiance, texCoord);
}
)";
        if(!radianceToLuminance_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc))
            QMessageBox::critical(nullptr, tr("Error compiling shader"),
                                  tr("Failed to compile %1:\n%2").arg("radiance-to-luminance fragment shader").arg(radianceToLuminance_.log()));
        if(!radianceToLuminance_.link())
            throw QMessageBox::critical(nullptr, tr("Error linking shader program"),
                                        tr("Failed to link %1:\n%2").arg("radiance-to-luminance shader program").arg(radianceToLuminance_.log()));
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

void Canvas::makeImageTexture()
{
    const int w=width(), h=height();
    glBindTexture(GL_TEXTURE_2D, glareTextures_[1]);
    std::vector<glm::vec4> image(w*h);
    const int x=w/2, y=h/2;
    image[w*y+x]=1e1f*glm::vec4(0.950455927051672, 1., 1.08905775075988, 0);
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
        // Our supersampling takes care of interpolation; doing it in the sampler would give bad results.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
}

void Canvas::resizeGL([[maybe_unused]] int w, [[maybe_unused]] int h)
{
    setupRenderTarget();
}

void Canvas::setupWavelengths()
{
    constexpr double min=400; // nm
    constexpr double max=700; // nm
    constexpr auto range=max-min;
    constexpr auto count=16;
    static_assert(count%4==0);
    for(int i=0;i<count;i+=4)
    {
        wavelengths_.push_back(QVector4D(min+range*double(i+0)/(count-1),
                                         min+range*double(i+1)/(count-1),
                                         min+range*double(i+2)/(count-1),
                                         min+range*double(i+3)/(count-1)));
    }
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
    if(glareFBOs_[0])
        glDeleteFramebuffers(std::size(glareFBOs_), glareFBOs_);
    if(glareTextures_[0])
        glDeleteTextures(std::size(glareTextures_), glareTextures_);
}

QMatrix4x4 Canvas::radianceToLuminance(const unsigned texIndex) const
{
    using glm::mat4;
    const auto diag=[](GLfloat x, GLfloat y, GLfloat z, GLfloat w) { return mat4(x,0,0,0,
                                                                                 0,y,0,0,
                                                                                 0,0,z,0,
                                                                                 0,0,0,w); };
    const auto wlCount = 4*wavelengths_.size();
    // Weights for the trapezoidal quadrature rule
    const auto weights = wlCount==4            ? diag(0.5,1,1,0.5) :
                         texIndex==0           ? diag(0.5,1,1,1  ) :
                         texIndex+1==wlCount/4 ? diag(  1,1,1,0.5) :
                                                 diag(  1,1,1,1);
    const auto dlambda = weights * abs(wavelengths_.back()[3]-wavelengths_.front()[0]) / (wlCount-1.f);
    // Ref: Rapport BIPM-2019/05. Principles Governing Photometry, 2nd edition. Sections 6.2, 6.3.
    const auto maxLuminousEfficacy=diag(683.002f,683.002f,683.002f,1700.13f); // lm/W
    const auto ret = maxLuminousEfficacy * mat4(wavelengthToXYZW(wavelengths_[texIndex][0]),
                                                wavelengthToXYZW(wavelengths_[texIndex][1]),
                                                wavelengthToXYZW(wavelengths_[texIndex][2]),
                                                wavelengthToXYZW(wavelengths_[texIndex][3])) * dlambda;
    return toQMatrix(ret);
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
    constexpr double angleStep=180*degree/numAngleSteps;

    for(unsigned wlSetIndex=0; wlSetIndex<wavelengths_.size(); ++wlSetIndex)
    {
        makeImageTexture();
        glareProgram_.bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glareTextures_[1]);
        glareProgram_.setUniformValue("radiance", 0);
        glareProgram_.setUniformValue("stepCount", numAngleSteps);
        glareProgram_.setUniformValue("wavelengths", wavelengths_[wlSetIndex]);
        for(int angleStepNum=0; angleStepNum<numAngleSteps; ++angleStepNum)
        {
            const auto angle = angleMin + angleStep*angleStepNum;
            glareProgram_.setUniformValue("stepDir", QVector2D(std::cos(angle),std::sin(angle)));
            glareProgram_.setUniformValue("stepNum", angleStepNum);
            glBindFramebuffer(GL_FRAMEBUFFER, glareFBOs_[angleStepNum%2]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            // Now use the result of this stage to feed the next stage
            glBindTexture(GL_TEXTURE_2D, glareTextures_[angleStepNum%2]);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, luminanceFBO_);
        radianceToLuminance_.bind();
        radianceToLuminance_.setUniformValue("radiance", 0);
        radianceToLuminance_.setUniformValue("radianceToLuminance", radianceToLuminance(wlSetIndex));
        glBlendFunc(GL_ONE, GL_ONE);
        if(wlSetIndex==0)
            glDisable(GL_BLEND);
        else
            glEnable(GL_BLEND);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_BLEND);
    }

    glBindFramebuffer(GL_FRAMEBUFFER,targetFBO);
    luminanceToScreen_.bind();
    const float exposure=1;
    luminanceToScreen_.setUniformValue("exposure", exposure);
    glBindTexture(GL_TEXTURE_2D, luminanceTexture_);
    luminanceToScreen_.setUniformValue("luminanceXYZW", 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
}

#include "ApertureOutline.hpp"
#include <algorithm>
#include <QPainter>
#include <glm/glm.hpp>
#include "ToolsWidget.hpp"

ApertureOutline::ApertureOutline(ToolsWidget* tools, QWidget* parent)
    : QWidget(parent)
    , tools_(tools)
{
}

void ApertureOutline::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setPen(Qt::white);
    p.setBrush(Qt::white);
    const auto size = (std::min(width(), height()) & -2) - 1;
    const auto topLeft = QPoint((width()-size)/2,(height()-size)/2);
    const auto canvasRect = QRect(topLeft, QSize(size,size));
    p.drawRect(canvasRect);

    p.setPen(Qt::black);
    p.setRenderHint(QPainter::Antialiasing);

    using namespace glm;
    using namespace std;

    const auto sqr = [](auto x){ return x*x; };
    const float PI = acos(-1.);

    const int sampleCount = tools_->sampleCount();
    const int pointCount = tools_->pointCount();
    const float globalRotationAngle = tools_->globalRotationAngle()*PI/180;
    const float curvatureRadius = tools_->curvatureRadius();
    const float arcPointCount = tools_->arcPointCount();

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
                    p.drawLine(canvasRect.center()+QPointF(1,1)+size/2*QPointF(arcP1.x, arcP1.y),
                               canvasRect.center()+QPointF(1,1)+size/2*QPointF(arcP2.x, arcP2.y));
                }
            }
        }
    }
}

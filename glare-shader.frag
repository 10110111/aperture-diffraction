R"(
#version 330
uniform int pointCount;
uniform int arcPointCount;
uniform float curvatureRadius;
uniform int sampleCount;
uniform float targetWidth; // mm
uniform float wavenumber; // mm^-1
uniform float globalRotationAngle;
uniform vec4 radianceToLuminance;
uniform vec2 imageSize;
uniform float apertureRadius; // mm
out vec4 XYZW;
const float PI=3.14159265;

float triangleArea(vec2 p1, vec2 p2, vec2 p3)
{
    return p1.y*p2.x + p2.y*p3.x + p1.x*p3.y - p1.x*p2.y - p1.y*p3.x - p2.x*p3.y;
}

float sinc(float x) { return abs(x)<1e-4 ? 1 : sin(x)/x; }
float sqr(float x) { return x*x; }

// Ref: Equations (4.1), (4.2), but altering the definition of sinc, in
//      R.M. Sillitto, W. Sillitto, "A Simple Fourier Approach to Fraunhofer Diffraction by Triangular Apertures"
//      http://dx.doi.org/10.1080/713819012
vec2 triangle(vec2 s1, vec2 s2, vec2 s3, vec2 k)
{
    if(k==vec2(0)) return vec2(1,0);
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

vec2 sampleShift(const float sampleNumX, const float sampleNumY)
{
    return vec2(sampleNumX+0.5, sampleNumY+0.5)/sampleCount;
}

void main()
{
    XYZW=vec4(0);
    const vec2 p0=vec2(0,0);
    for(int sampleNumY=0; sampleNumY<sampleCount; ++sampleNumY)
    {
        for(int sampleNumX=0; sampleNumX<sampleCount; ++sampleNumX)
        {
            vec2 shiftInPixel=sampleShift(sampleNumX,sampleNumY);
            // Distance from the center in mm at a distance of 10m from the aperture
            vec2 pointInTargetPlane = (gl_FragCoord.st - round(imageSize/2) + shiftInPixel) / (imageSize.x/2) * targetWidth;
            const float distToTargetPlane = 10e3; // mm
            // Distance from the center of the aperture to the point in the target plane
            float distToPoint = sqrt(dot(pointInTargetPlane, pointInTargetPlane) + sqr(distToTargetPlane));
            // Projection of the wave vector onto the plane of the aperture
            vec2 k = wavenumber * pointInTargetPlane / distToPoint;

            float XYZW_re=0, XYZW_im=0;
            for(int pointNum=1; pointNum<=pointCount; ++pointNum)
            {
                float phi1 = 2*PI*(pointNum-1)/pointCount + (pointCount%2==1 ? PI/2 : 0) + globalRotationAngle;
                float phi2 = 2*PI* pointNum   /pointCount + (pointCount%2==1 ? PI/2 : 0) + globalRotationAngle;
                vec2 p1=vec2(cos(phi1), sin(phi1));
                vec2 p2=vec2(cos(phi2), sin(phi2));
                vec2 midPoint = (p1+p2)/2;
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
                    arcP1 *= apertureRadius;
                    arcP2 *= apertureRadius;
                    float area = triangleArea(p0,arcP1,arcP2);
                    vec2 tri = triangle(p0,arcP1,arcP2,k);
                    XYZW_re += area*tri.x;
                    XYZW_im += area*tri.y;
                }
            }
            XYZW += radianceToLuminance*(XYZW_re*XYZW_re+XYZW_im*XYZW_im);
        }
    }
    XYZW /= sqr(sampleCount);
}
)"

#pragma once

#include <cmath>
#include <cassert>

inline float illuminantD65(float wavelength)
{
    // Ref: http://files.cie.co.at/204.xls
    // Relative spectral power distribution of CIE Standard Illuminant D65, 5 nm step
    static constexpr float wavelengthMin=300, wavelengthMax=830, step=5; // nm
    static constexpr float data[]={
                                 0.034100,
                                 1.664300,
                                 3.294500,
                                 11.765200,
                                 20.236000,
                                 28.644700,
                                 37.053500,
                                 38.501100,
                                 39.948800,
                                 42.430200,
                                 44.911700,
                                 45.775000,
                                 46.638300,
                                 49.363700,
                                 52.089100,
                                 51.032300,
                                 49.975500,
                                 52.311800,
                                 54.648200,
                                 68.701500,
                                 82.754900,
                                 87.120400,
                                 91.486000,
                                 92.458900,
                                 93.431800,
                                 90.057000,
                                 86.682300,
                                 95.773600,
                                 104.865000,
                                 110.936000,
                                 117.008000,
                                 117.410000,
                                 117.812000,
                                 116.336000,
                                 114.861000,
                                 115.392000,
                                 115.923000,
                                 112.367000,
                                 108.811000,
                                 109.082000,
                                 109.354000,
                                 108.578000,
                                 107.802000,
                                 106.296000,
                                 104.790000,
                                 106.239000,
                                 107.689000,
                                 106.047000,
                                 104.405000,
                                 104.225000,
                                 104.046000,
                                 102.023000,
                                 100.000000,
                                 98.167100,
                                 96.334200,
                                 96.061100,
                                 95.788000,
                                 92.236800,
                                 88.685600,
                                 89.345900,
                                 90.006200,
                                 89.802600,
                                 89.599100,
                                 88.648900,
                                 87.698700,
                                 85.493600,
                                 83.288600,
                                 83.493900,
                                 83.699200,
                                 81.863000,
                                 80.026800,
                                 80.120700,
                                 80.214600,
                                 81.246200,
                                 82.277800,
                                 80.281000,
                                 78.284200,
                                 74.002700,
                                 69.721300,
                                 70.665200,
                                 71.609100,
                                 72.979000,
                                 74.349000,
                                 67.976500,
                                 61.604000,
                                 65.744800,
                                 69.885600,
                                 72.486300,
                                 75.087000,
                                 69.339800,
                                 63.592700,
                                 55.005400,
                                 46.418200,
                                 56.611800,
                                 66.805400,
                                 65.094100,
                                 63.382800,
                                 63.843400,
                                 64.304000,
                                 61.877900,
                                 59.451900,
                                 55.705400,
                                 51.959000,
                                 54.699800,
                                 57.440600,
                                 58.876500,
                                 60.312500,
    };
    constexpr auto numPoints=sizeof data/sizeof*data;
    static_assert(numPoints==1+(wavelengthMax-wavelengthMin)/step,
                  "Mismatch between number of points in data and expected number of points");
    if(wavelength<wavelengthMin || wavelength>wavelengthMax)
        return 0;
    const auto fractionalIndex=(wavelength-wavelengthMin)/step;
    assert(fractionalIndex>=0);
    assert(fractionalIndex<numPoints);
    const unsigned smallerIdx=std::floor(fractionalIndex);
    const unsigned largerIdx =std::ceil (fractionalIndex);
    assert(smallerIdx<numPoints);
    assert(largerIdx<numPoints);
    assert(smallerIdx<=largerIdx);
    if(smallerIdx==largerIdx) return data[smallerIdx];
    const auto alpha=fractionalIndex-int(fractionalIndex);
    return data[smallerIdx]*(1-alpha)+data[largerIdx]*alpha;
}

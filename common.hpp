#pragma once

#include <QSurfaceFormat>

enum
{
    OPENGL_MAJOR_VERSION=3,
    OPENGL_MINOR_VERSION=3,
};

QSurfaceFormat makeGLSurfaceFormat();

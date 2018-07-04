/*
Copyright (c) 2018 Zhang Weijia

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "pixpix.h"
#include <cmath>
using namespace pixpix;

namespace pixpix {

MATRIX4
Math::translation(float dx, float dy, float dz) {
    MATRIX4 mat = MATRIX4();
    mat.setRow(0, 1.0, 0.0, 0.0, dx);
    mat.setRow(1, 0.0, 1.0, 0.0, dy);
    mat.setRow(2, 0.0, 0.0, 1.0, dz);
    mat.setRow(3, 0.0, 0.0, 0.0, 1.0);
    return mat;
}

MATRIX4
Math::rotationX(float angle) {
    MATRIX4 mat = MATRIX4();
    float c = cos(angle), s = sin(angle);
    mat.setRow(0, 1.0, 0.0, 0.0, 0.0);
    mat.setRow(1, 0.0, c,   -s,  0.0);
    mat.setRow(2, 0.0, s,   c,   0.0);
    mat.setRow(3, 0.0, 0.0, 0.0, 1.0);
    return mat;
}

MATRIX4
Math::rotationY(float angle) {
    MATRIX4 mat = MATRIX4();
    float c = cos(angle), s = sin(angle);
    mat.setRow(0, c,   0.0, s,   0.0);
    mat.setRow(1, 0.0, 1.0, 0.0, 0.0);
    mat.setRow(2, -s,  0.0, c,   0.0);
    mat.setRow(3, 0.0, 0.0, 0.0, 1.0);
    return mat;
}

MATRIX4
Math::rotationZ(float angle) {
    MATRIX4 mat = MATRIX4();
    float c = cos(angle), s = sin(angle);
    mat.setRow(0, c,   -s,  0.0, 0.0);
    mat.setRow(1, s,   c,   0.0, 0.0);
    mat.setRow(2, 0.0, 0.0, 1.0, 0.0);
    mat.setRow(3, 0.0, 0.0, 0.0, 1.0);
    return mat;
}

MATRIX4
Math::pitch_yaw_roll(float pitch, float yaw, float roll) {
    MATRIX4 rX, rY, rZ, mat;
    rX = rotationX(pitch);
    rY = rotationY(yaw);
    rZ = rotationZ(roll);
    mat = matrixMul(rX, matrixMul(rY, rZ));
    return mat;
}

MATRIX4
Math::projection(float fovY, float aspect_ratio, float nearZ, float farZ) {
    MATRIX4 mat;
    
    float t11 = 1.0f / (aspect_ratio * tanf(fovY / 2.0f));
    float t22 = 1.0f / tanf(fovY / 2.0f);
    /*
    float t33 = 1.0f / (farZ - nearZ);
    float t34 = - nearZ / (farZ - nearZ);

    mat.setRow(0, t11, 0.0, 0.0, 0.0);
    mat.setRow(1, 0.0, t22, 0.0, 0.0);
    mat.setRow(2, 0.0, 0.0, t33, t34);
    mat.setRow(3, 0.0, 0.0, 1.0, 0.0);
    */
    mat.setRow(0, t11, 0.0, 0.0, 0.0);
    mat.setRow(1, 0.0, t22, 0.0, 0.0);
    mat.setRow(2, 0.0, 0.0, -1.0, 0.0);
    mat.setRow(3, 0.0, 0.0, -1.0, 0.0);
    return mat;
}

VEC4
Math::matrixVecMul(MATRIX4 mat, VEC4 vec) {
    VEC4 res;
    res.x = mat.mat[0][0] * vec.x + mat.mat[0][1] * vec.y + mat.mat[0][2] * vec.z + mat.mat[0][3] * vec.w;
    res.y = mat.mat[1][0] * vec.x + mat.mat[1][1] * vec.y + mat.mat[1][2] * vec.z + mat.mat[1][3] * vec.w;
    res.z = mat.mat[2][0] * vec.x + mat.mat[2][1] * vec.y + mat.mat[2][2] * vec.z + mat.mat[2][3] * vec.w;
    res.w = mat.mat[3][0] * vec.x + mat.mat[3][1] * vec.y + mat.mat[3][2] * vec.z + mat.mat[3][3] * vec.w;
    // res.regularize();
    return res;
}

MATRIX4
Math::matrixMul(MATRIX4 mat1, MATRIX4 mat2) {
    MATRIX4 mat;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            mat.mat[i][j] = mat1.mat[i][0] * mat2.mat[0][j] + 
                            mat1.mat[i][1] * mat2.mat[1][j] + 
                            mat1.mat[i][2] * mat2.mat[2][j] + 
                            mat1.mat[i][3] * mat2.mat[3][j];
        }
    }
    return mat;
}


}
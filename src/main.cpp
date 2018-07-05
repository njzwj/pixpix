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

#include <cstdio>
#include <cstdlib>
#include <Windows.h>
#include "cli_graph.h"
#include "svpng.inc"
#include "pixpix.h"
using namespace pixpix;
using namespace std;

void drawPlane(RenderPipeline3D *pipeline, float w, float h, VEC3 position, VEC3 rotation) {
    VEC3 verts[4] = {
        (VEC3){-w/2.0f, h/2.0f, 0},
        (VEC3){w/2.0f, h/2.0f, 0},
        (VEC3){-w/2.0f, -h/2.0f, 0},
        (VEC3){w/2.0f, -h/2.0f, 0}
    };
    unsigned faceIndex[] = {3, 3};
    unsigned vertexIndex[] = {0, 2, 3, 0, 3, 1};
    VEC2 texCoord[] = {
        (VEC2){0.0f, 0.0f}, (VEC2){0.0f, 1.0f}, (VEC2){1.0f, 1.0f},
        (VEC2){0.0f, 0.0f}, (VEC2){1.0f, 1.0f}, (VEC2){1.0f, 0.0f}};
    
    MATRIX4 m_rot = Math::pitch_yaw_roll(rotation.y, rotation.x, rotation.z),
        m_trans = Math::translation(position.x, position.y, position.z),
        m_comp = Math::matrixMul(m_trans, m_rot);
    /* vertex transformation */
    for (size_t i = 0; i < 4; ++i) {
        VEC4 v_tmp = {verts[i].x, verts[i].y, verts[i].z, 1.0f};
        v_tmp = Math::matrixVecMul(m_comp, v_tmp);
        v_tmp.regularize();
        verts[i] = (VEC3){v_tmp.x, v_tmp.y, v_tmp.z};
    }
    
    MESH mesh;
    mesh.faceIndex = new vector<unsigned>(faceIndex, faceIndex+2);
    mesh.vertexIndex = new vector<unsigned>(vertexIndex, vertexIndex+6);
    mesh.normal = new vector<VEC3>(6, (VEC3){0, 0, 1.0});
    mesh.texCoord = new vector<VEC2>(texCoord, texCoord+6);
    mesh.verts = new vector<VEC3>(verts, verts+4);

    /* normal transformation */
    for (size_t i = 0; i < 6; ++i) {
        VEC4 v_tmp = (VEC4){(*mesh.normal)[i].x, (*mesh.normal)[i].y, (*mesh.normal)[i].z, 1.0f};
        v_tmp = Math::matrixVecMul(m_rot, v_tmp);
        v_tmp.regularize();
        (*mesh.normal)[i] = v_tmp;
    }

    pipeline->render(mesh);
}

int main() {
    /* testcode */
    const int W = 128, H = 64;
    CANVAS *cav = new CANVAS(W, H);
    CAMERA *cam = new CAMERA();
    RenderPipeline3D *pipeline = new RenderPipeline3D(cav, cam);
    TEXTURE *tex = new TEXTURE();
    LIGHT *lgt = new LIGHT();
    MATERIAL *mat = new MATERIAL();

    pipeline->init();
    cam->position = {0.0f, 0.0f, 4.0f};
    cam->lookAt(0, 0, 0);
    cam->fovY = Math::Pi / 180.0f * 60.0f;

    tex->ty = T_CHESS_BOARD;
    tex->sz = 4;
    tex->color1 = {0.2, 0.9, 0.1, 1.0};
    tex->color2 = {0.0, 0.1, 0.1, 1.0};
    
    lgt->mSpecularIntensity = 1.0f;
    lgt->mDiffuseIntensity = 0.5f;
    lgt->mIsEnabled = true;
    lgt->mAmbientColor = {0.1f, 0.1f, 0.1f};
    lgt->mDiffuseColor = {1.0f, 1.0f, 1.0f};
    lgt->mSpecularColor = {1.0f, 1.0f, 1.0f};
    lgt->mPosition = {0.0f, 5.0f, 0.0f};

    mat->specularSmoothLevel = 100;
    for (int i = -30; ; ++i) {
        if (i > 30) i = -30;
        cam->position = {i / 10.0f, i / 10.0f, 4.0f};
        cam->lookAt(0, 0, 0);
        pipeline->init();
        pipeline->setMaterial(mat);
        //lgt->mPosition = {3.0f * cos(Math::Pi/60.0f*i), 3.0f, 3.0f * sin(Math::Pi/60.0f*i)};
        lgt->mAmbientColor = {0.1f, 0.1f, 0.1f};
        lgt->mSpecularColor = {1.0f, 1.0f, 1.0f};
        lgt->mPosition = {1.0f, 5.0f, 0.0f};
        pipeline->addLight(*lgt);
        lgt->mAmbientColor = {0, 0, 0};
        lgt->mSpecularColor = {1.0f, 1.0f, 0.0f};
        lgt->mPosition = {-1.0f, -5.0f, 5.0f};
        pipeline->addLight(*lgt);

        tex->color1 = {1.0, 0.5, 0.5, 1.0};
        tex->color2 = {0.1, 0.1, 0.1, 1.0};
        pipeline->setTexture(tex);
        drawPlane(pipeline, 3.0f, 3.0f, {0.0f,0.0f,0.0f}, {0, 0, 0});

        cli_graph(W, H, cav->img);
        Sleep(50);
    }
    return 0;
}
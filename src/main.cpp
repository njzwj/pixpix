/*
Copyright 2018 njzwj. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software 
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

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

VERTEX3 getVertex(VEC3 pos, VEC2 tex_coord) {
    VERTEX3 vt;
    vt.pos = pos;
    vt.tex_coord = tex_coord;
    vt.color = {0.0, 0.0, 0.0, 1.0};
    vt.normal = {0.0, 0.0, 0.0};
    return vt;
}

void drawSphere(RenderPipeline3D *pipeline, float r, unsigned w, unsigned h, VEC3 position, VEC3 rotation) {
    MATRIX4 m_rot = Math::pitch_yaw_roll(rotation.y, rotation.x, rotation.z),
            m_trans = Math::translation(position.x, position.y, position.z),
            m_comp = Math::matrixMul(m_trans, m_rot);
    
    vector<VERTEX3> *vertex = new vector<VERTEX3>;

    float d_theta = Math::Pi * 2.0f / w;
    float d_alpha = Math::Pi / h;
    for (int i = 0; i < w; ++i) {
        for (int j = 0; j < h; ++j) {
            float theta[] = {d_theta * i, d_theta * (i + 1)};
            float alpha[] = {Math::Pi/2.0f-d_alpha * j, Math::Pi/2.0f-d_alpha * (j + 1)};
#define GPOS(t, a) ((VEC4){ cos(t)*cos(a), sin(t)*cos(a), sin(a), 1.0f })
            VEC3 normal, pos;
            VEC4 normalH, posH;
            VEC2 tex_coord;
            VERTEX3 ver[4];
            for (int u = 0; u < 2; ++u)
                for (int v = 0; v < 2; ++v) {
                    normalH = GPOS(theta[u], alpha[v]);
                    posH = normalH * r; posH.w = 1.0;
                    normalH = Math::matrixVecMul(m_rot, normalH);
                    normalH.regularize();
                    posH = Math::matrixVecMul(m_comp, posH);
                    posH.regularize();
                    normal = (VEC3){normalH.x, normalH.y, normalH.z};
                    pos = (VEC3){posH.x, posH.y, posH.z};
                    tex_coord = (VEC2){1.0f/w*(i+u), 1.0f/h*(j+v)};
                    // printf("%f %f\n", tex_coord.x, tex_coord.y);
                    VERTEX3 &cur_ver = ver[v*2+u];
                    cur_ver.normal = normal;
                    cur_ver.pos = pos;
                    cur_ver.tex_coord = tex_coord;
                }
            vertex->push_back(ver[0]);
            vertex->push_back(ver[2]);
            vertex->push_back(ver[3]);
            vertex->push_back(ver[0]);
            vertex->push_back(ver[3]);
            vertex->push_back(ver[1]);
#undef GPOS
        }
    }
    pipeline->render(vertex);
    delete vertex;
}

void drawPlane(RenderPipeline3D *pipeline, float w, float h, VEC3 position, VEC3 rotation) {
    vector<VERTEX3> *vertex = new vector<VERTEX3>;
    VERTEX3 va = getVertex( {-w/2.0f, h/2.0f, 0.0f}, {0.0f, 0.0f} ),
            vb = getVertex( {w/2.0f, h/2.0f, 0.0f}, {1.0f, 0.0f} ),
            vc = getVertex( {-w/2.0f, -h/2.0f, 0.0f}, {0.0f, 1.0f} ),
            vd = getVertex( {w/2.0f, -h/2.0f, 0.0f}, {1.0f, 1.0f} );
    vertex->push_back(va);
    vertex->push_back(vc);
    vertex->push_back(vd);
    vertex->push_back(va);
    vertex->push_back(vd);
    vertex->push_back(vb);

    MATRIX4 m_rot = Math::pitch_yaw_roll(rotation.y, rotation.x, rotation.z),
            m_trans = Math::translation(position.x, position.y, position.z),
            m_comp = Math::matrixMul(m_trans, m_rot);

    for (int i = 0; i < vertex->size(); ++i) {
        VERTEX3 &cur_vertex = (*vertex)[i];
        VEC3 pos = cur_vertex.pos,
             normal = {0.0f, 0.0f, 1.0f};
        VEC4 posH = {pos.x, pos.y, pos.z, 1.0f},
             normalH = {normal.x, normal.y, normal.z, 1.0f};
        posH = Math::matrixVecMul(m_comp, posH);
        posH.regularize();
        normalH = Math::matrixVecMul(m_rot, normalH);
        normalH.regularize();
        
        cur_vertex.pos = {posH.x, posH.y, posH.z};
        cur_vertex.normal = {normalH.x, normalH.y, normalH.z};
    }

    pipeline->render(vertex);
    delete vertex;
}

int main() {
    /* testcode */
    const int W = 128, H = 64;
    CANVAS *cav = new CANVAS(W, H);
    RenderPipeline3D *pipeline = new RenderPipeline3D();
    vector<VERTEX3> vecs;
    TEXTURE *tex = new TEXTURE();
    LIGHT *lgt = new LIGHT();
    MATERIAL *mat = new MATERIAL();

    pipeline->init(cav);
    pipeline->camera->position = {4.0, 4.0, 4.0};
    pipeline->camera->lookAt(0, 0, 0);
    pipeline->camera->fovY = Math::Pi / 180.0f * 60.0f;

    tex->ty = T_CHESS_BOARD;
    tex->sz = 8;
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
        pipeline->camera->position = {i / 10.0f, i / 10.0f, 4.0f};
        pipeline->camera->lookAt(0, 0, 0);
        pipeline->init(cav);
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

        tex->color1 = {0.2, 1.0, 0.5, 1.0};
        tex->color2 = {0.0, 0.5, 1.0, 1.0};
        pipeline->setTexture(tex);
        drawSphere(pipeline, 1.0f, 12, 6, {0,0,0}, {-Math::Pi/30.0f*i,Math::Pi/60.0f*i,Math::Pi/76.0f*i});
        tex->color1 = {1.0, 0.5, 0.5, 1.0};
        tex->color2 = {0.1, 0.1, 0.1, 1.0};
        pipeline->setTexture(tex);
        drawPlane(pipeline, 3.0f, 3.0f, {0.0f,0.0f,0.0f}, {0, 0, 0});

        cli_graph(W, H, cav->img);
        Sleep(50);
    }

    //pipeline->render(&vecs);
    return 0;
}
#include <cstdio>
#include <cstdlib>
#include <Windows.h>
#include "cli_graph.h"
#include "svpng.h"
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

    MATRIX4 m_rot = Math::yaw_pitch_roll(rotation.y, rotation.x, rotation.z),
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
    pipeline->init(cav);
    pipeline->camera->position = {0.0, 0.0, -8.0};
    pipeline->camera->fovY = Math::Pi / 180.0f * 20.0f;
    tex->ty = T_CHESS_BOARD;
    tex->sz = 6;
    tex->color1 = {0.1, 0.9, 0.3, 1.0};
    tex->color2 = {0.0, 0.1, 0.1, 1.0};
    for (int i = 0; ; ++i) {
        pipeline->init(cav);
        tex->color1 = {0.1, 0.9, 0.3, 1.0};
        tex->color2 = {0.0, 0.1, 0.1, 1.0};
        pipeline->setTexture(tex);
        drawPlane(pipeline, 2.0f, 2.0f, {0.0f,0.0f,0.0f}, {Math::Pi/135.0f*i, Math::Pi/60.0f*i, Math::Pi/224.0f*i});

        tex->color1 = {1.0, 0.0, 0.0, 1.0};
        tex->color2 = {0.0, 0.0, 1.0, 1.0};
        pipeline->setTexture(tex);
        drawPlane(pipeline, 2.0f, 2.0f, {0.0f,0.0f,0.0f}, {-Math::Pi/160.0f*i+1.5f, Math::Pi/95.0f*i, -Math::Pi/304.0f*i});


        cli_graph(W, H, cav->img);
        Sleep(50);
    }

    //pipeline->render(&vecs);
    // FILE *fp = fopen("./test.png", "wb");
    // svpng(fp, W, H, cav->img, 0);
    // fclose(fp);
    return 0;
}
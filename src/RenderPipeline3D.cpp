#include "pixpix.h"
#include <cmath>
#include <cstdio>
using namespace std;

namespace pixpix {

/*
    \brief calculate s, t so that s*b+t*c+(1-s-t)*a = p
*/
void
RenderPipeline3D::bilinearInterpolation(VEC2 a, VEC2 b, VEC2 c, VEC2 p, 
    float &s, float &t) {
    VEC2 v1 = b - a, v2 = c - a;
    p = p - a;
    float det = v1.x * v2.y - v1.y * v2.x;
    s = (v2.y * p.x - v2.x * p.y) / det;
    t = (- v1.y * p.x + v1.x * p.y) / det;
}

COLOR4
RenderPipeline3D::getChessBoard(VEC2 tex_coord, unsigned sz, COLOR4 color1, COLOR4 color2) {
    unsigned bit = (unsigned)(sz * tex_coord.x) + (unsigned)(sz * tex_coord.y);
    if (bit & 1) 
        return color1;
    return color2;
}

bool
RenderPipeline3D::zBufferTest(RASTERIZED_FRAGMENT frag, float depth) {
    for (int i = fragment->size()-1; i >= 0; --i) {
        if (frag.posX != (*fragment)[i].posX || frag.posY != (*fragment)[i].posY)
            continue;
        if (depth > (*zBuffer)[i])
            return false;
        return true;
    }
    return true;
}

void
RenderPipeline3D::shadeFragment(RASTERIZED_FRAGMENT &frag) {
    if (texture == nullptr) return;
    if (texture->ty == T_CHESS_BOARD) {
        frag.color = getChessBoard(frag.tex_coord, texture->sz, texture->color1, texture->color2);
    }
}

void
RenderPipeline3D::init(CANVAS *c) {
    
    if (fragment == nullptr)
        fragment = new vector<RASTERIZED_FRAGMENT>;
    else
        fragment->clear();
        
    if (zBuffer == nullptr)
        zBuffer = new vector<float>;
    else
        zBuffer->clear();
        
    texture = nullptr;
    
    cav = c;
    cav->clear();
}

void
RenderPipeline3D::setTexture(TEXTURE *tex) {
    texture = tex;
}

/*
    \brief for test use, to validate the 2D & 3D homo triangles drawing.
           it will generate interpolated rasterized fragments
    \param v_homo must has 3n elements, representing n triangles.
*/
void
RenderPipeline3D::render(vector<VERTEX3> *vecs) {
    if (vertex_homo == nullptr)
        vertex_homo = new vector<VERTEX_RENDER>();
    else
        vertex_homo->clear();
    
    /* cam transform */
    MATRIX4 m_cam_space_trans = Math::matrixMul(
        Math::yaw_pitch_roll(-camera->rotation.y, -camera->rotation.x, -camera->rotation.z),
        Math::translation(-camera->position.x, -camera->position.y, -camera->position.z)
    );
    MATRIX4 m_homo_space_trans = Math::projection(camera->fovY, camera->aspect_ratio, camera->nearZ, camera->farZ);
    /* transform vecs using cam info into homogenous space */
    for (int i = 0; i < vecs->size(); ++i) {
        VERTEX3 cur_v = (*vecs)[i];
        VERTEX_RENDER cur_v_render;
        VEC4 posH = (VEC4){ cur_v.pos.x, cur_v.pos.y, cur_v.pos.z, 1.0 };
        posH = Math::matrixVecMul(m_cam_space_trans, posH);
        if (posH.z < camera->nearZ) continue;
        posH = Math::matrixVecMul(m_homo_space_trans, posH);
        cur_v_render.posH = posH;
        cur_v_render.tex_coord = cur_v.tex_coord;
        cur_v_render.color = cur_v.color;
        vertex_homo->push_back(cur_v_render);
    }

    /* draw triangles */
    VERTEX_RENDER v1, v2, v3;
    
    /* travsal all the triangles */
    for (int i = 0; i < vertex_homo->size(); i += 3) {
        v1 = (*vertex_homo)[i];
        v2 = (*vertex_homo)[i+1];
        v3 = (*vertex_homo)[i+2];
        v1.posH.y = - v1.posH.y;
        v2.posH.y = - v2.posH.y;
        v3.posH.y = - v3.posH.y;
        /* 2d ref point */
        VEC2 va = {v1.posH.getx(), v1.posH.gety()},
             vb = {v2.posH.getx(), v2.posH.gety()},
             vc = {v3.posH.getx(), v3.posH.gety()};
        
        /* calculate bounding box */
        float minX, minY, maxX, maxY;
#define BOUND(a, b, c, mi, mx) do{mi=a>b?b:a;mi=mi>c?c:mi;mx=a>b?a:b;mx=mx>c?mx:c;}while(0)
        BOUND(va.x,vb.x,vc.x,minX,maxX);
        BOUND(va.y,vb.y,vc.y,minY,maxY);
        minX = minX>=-1?minX:-1; minY = minY>=-1?minY:-1;
        maxX = maxX<1?maxX:1; maxY = maxY<1?maxY:1;
#undef BOUND
        minX = (minX + 1.0) * cav->w / 2.0;
        maxX = (maxX + 1.0) * cav->w / 2.0;
        minY = (minY + 1.0) * cav->h / 2.0;
        maxY = (maxY + 1.0) * cav->h / 2.0;
        for (int y = minY; y < maxY; ++y) {
            for (int x = minX; x < maxX; ++x) {
                VEC2 pos = {(float)x / cav->w * 2.0f - 1.0f, (float)y / cav->h * 2.0f - 1.0f};
                RASTERIZED_FRAGMENT cur_frag;
                /* judge if pos is in triangle ABC */
#define BETWEEN(a, b, c, d) (((b-a)^(d-a))*((d-a)^(c-a))>=0)
                bool is_in_triangle = 
                    BETWEEN(va, vb, vc, pos) &&
                    BETWEEN(vb, va, vc, pos) &&
                    BETWEEN(vc, va, vb, pos);
#undef BETWEEN
                if (!is_in_triangle) continue;
                /* setup current fragment */
                cur_frag.posX = x;
                cur_frag.posY = y;
                /* screen space interpolation */
                /* use (->A)+S*(->AB)+T*(->AC) to represent the current point */
                /* POS=S*(->B)+T*(->C)+(1-S-T)*(->A) */
                float s, t;
                bilinearInterpolation(va, vb, vc, pos, s, t);
                float depth = 1.0 / ((1 - s - t) / v1.posH.z + s / v2.posH.z + t / v3.posH.z);
                if (!zBufferTest(cur_frag, depth)) continue;
                cur_frag.tex_coord = (v1.tex_coord / v1.posH.z * (1 - s - t) + 
                                 v2.tex_coord / v2.posH.z * s + 
                                 v3.tex_coord / v3.posH.z * t) * depth;
                
                cur_frag.color = (v1.color / v1.posH.z * (1 - s - t) + 
                                 v2.color / v2.posH.z * s + 
                                 v3.color / v3.posH.z * t) * depth;
                
                shadeFragment(cur_frag);
                
                fragment->push_back(cur_frag);
                zBuffer->push_back(depth);
                cav->setPixel(cur_frag.posX, cur_frag.posY, cur_frag.color);
            }
        }
    }
}

}
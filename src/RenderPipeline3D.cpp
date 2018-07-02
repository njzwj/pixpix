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
RenderPipeline3D::shadeFragment(RASTERIZED_FRAGMENT &frag, VEC3 pos_origin) {
    if (texture == nullptr) return;
    /* diffuse color */
    if (texture->ty == T_CHESS_BOARD) {
        frag.color = getChessBoard(frag.tex_coord, texture->sz, texture->color1, texture->color2);
    }
    COLOR4 diffuseColor = frag.color;
    if (frag.normal * (camera->position - pos_origin).normalize() <= 0) frag.normal = (VEC3){0, 0, 0} - frag.normal;
    /* 环境反射 漫反射 高光 */
    if (material != nullptr) {
        frag.color = {0, 0, 0, 1.0f};
        for (int i = 0; i < light->size(); ++i) {
            LIGHT cur_light = (*light)[i];
            if (!cur_light.mIsEnabled) continue;
            /* ambient color */
            frag.color = frag.color + (COLOR4){diffuseColor.x * cur_light.mAmbientColor.x, 
                         diffuseColor.y * cur_light.mAmbientColor.y,
                         diffuseColor.z * cur_light.mAmbientColor.z,
                         0.0};

            float diffuse = (cur_light.mPosition - pos_origin).normalize() * frag.normal * cur_light.mDiffuseIntensity;
            if (diffuse < 0) continue;
            /* reflection vector =
               (->)n + (->)n - (->)v
            */
            VEC3 v = (cur_light.mPosition - pos_origin).normalize();
            v = v / (v * frag.normal);
            float specular = pow((frag.normal * 2.0f - v).normalize()
            *(camera->position - pos_origin).normalize(),material->specularSmoothLevel) * cur_light.mSpecularIntensity;
            diffuse = diffuse > 0? diffuse : 0;
            specular = specular > 0? specular : 0;

            COLOR3 c1;
            /* diffuse color */
            frag.color = frag.color + (COLOR4){diffuseColor.x*cur_light.mDiffuseColor.x,
                        diffuseColor.y*cur_light.mDiffuseColor.y, 
                        diffuseColor.z*cur_light.mDiffuseColor.z, 
                        0} * diffuse;
            /* specular color */
            frag.color = frag.color + (COLOR4){cur_light.mSpecularColor.x, 
                        cur_light.mSpecularColor.y, 
                        cur_light.mSpecularColor.z, 0} * specular;
        }
    }
#define CUT(x) do { x=x>0?x:0; x=x<1?x:1; } while(0)
    CUT(frag.color.x);
    CUT(frag.color.y);
    CUT(frag.color.z);
    CUT(frag.color.w);
#undef CUT
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

    if (light == nullptr)
        light = new vector<LIGHT>;
    else
        light->clear();
        
    texture = nullptr;
    
    cav = c;
    cav->clear();
}

void
RenderPipeline3D::setTexture(TEXTURE *tex) {
    texture = tex;
}

void
RenderPipeline3D::setMaterial(MATERIAL *mat) {
    material = mat;
}

void
RenderPipeline3D::addLight(LIGHT l) {
    light->push_back(l);
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
    
    if (vertex_homo_clipped == nullptr)
        vertex_homo_clipped = new vector<VERTEX_RENDER>;
    else
        vertex_homo_clipped->clear();
    
    vector<unsigned> *v_homo_origin = new vector<unsigned>;

    /* cam transform */
    MATRIX4 m_cam_space_trans = Math::matrixMul(
        Math::pitch_yaw_roll(-camera->rotation.x, -camera->rotation.y, -camera->rotation.z),
        Math::translation(-camera->position.x, -camera->position.y, -camera->position.z)
    );
    MATRIX4 m_homo_space_trans = Math::projection(camera->fovY, camera->aspect_ratio, camera->nearZ, camera->farZ);
    /* transform vecs using cam info into homogenous space */
    for (int i = 0; i < vecs->size(); ++i) {
        VERTEX3 cur_v = (*vecs)[i];
        VERTEX_RENDER cur_v_render;
        VEC4 posH = (VEC4){ cur_v.pos.x, cur_v.pos.y, cur_v.pos.z, 1.0 };
        posH = Math::matrixVecMul(m_cam_space_trans, posH);
        posH = Math::matrixVecMul(m_homo_space_trans, posH);
        cur_v_render.posH = posH;
        cur_v_render.tex_coord = cur_v.tex_coord;
        cur_v_render.color = cur_v.color;
        cur_v_render.normal = cur_v.normal;
        vertex_homo->push_back(cur_v_render);
    }

    /* culling convex */
    VERTEX_RENDER v1, v2, v3;
    VERTEX3 vo1, vo2, vo3;
    VEC2 va, vb, vc;

    for (int i = 0; i < vertex_homo->size(); i += 3) {
        v1 = (*vertex_homo)[i];
        v2 = (*vertex_homo)[i+1];
        v3 = (*vertex_homo)[i+2];
        /* test if outof homogenuous convex */
#define ISOUT(v) (v.posH.getx()<-1||v.posH.getx()>1||v.posH.gety()<-1||v.posH.gety()>1||v.posH.z<camera->nearZ||v.posH.z>camera->farZ)
        if (ISOUT(v1) && ISOUT(v2) && ISOUT(v3)) continue;
#undef ISOUT
        /* test if back face */
        va = {v1.posH.getx(), v1.posH.gety()};
        vb = {v2.posH.getx(), v2.posH.gety()};
        vc = {v3.posH.getx(), v3.posH.gety()};
        /* cull if clock-wise */
        if (((vb-va)^(vc-va)) <= 0) continue;

        vertex_homo_clipped->push_back(v1);
        vertex_homo_clipped->push_back(v2);
        vertex_homo_clipped->push_back(v3);
        v_homo_origin->push_back(i);
        v_homo_origin->push_back(i+1);
        v_homo_origin->push_back(i+2);
    }
    
    /* travsal all the triangles */
    for (int i = 0; i < vertex_homo_clipped->size(); i += 3) {
        v1 = (*vertex_homo_clipped)[i];
        v2 = (*vertex_homo_clipped)[i+1];
        v3 = (*vertex_homo_clipped)[i+2];
        vo1 = (*vecs)[(*v_homo_origin)[i]];
        vo2 = (*vecs)[(*v_homo_origin)[i+1]];
        vo3 = (*vecs)[(*v_homo_origin)[i+2]];
        v1.posH.y = - v1.posH.y;      /* screen coord, left-top (0,0) */
        v2.posH.y = - v2.posH.y;      /* reverse y to convert into it */
        v3.posH.y = - v3.posH.y;
        /* 2d ref point */
        va = {v1.posH.getx(), v1.posH.gety()};
        vb = {v2.posH.getx(), v2.posH.gety()};
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
#define BETWEEN(a, b, c, d) (((b-a)^(d-a))*((d-a)^(c-a))>=-1e-6)
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
                
                cur_frag.normal = (v1.normal / v1.posH.z * (1 - s - t) + 
                                 v2.normal / v2.posH.z * s + 
                                 v3.normal / v3.posH.z * t) * depth;
                /* interplate the original position space for perpix lighting */
                VEC3 pos_origin = (vo1.pos / v1.posH.z * (1 - s - t) + 
                                 vo2.pos / v2.posH.z * s + 
                                 vo3.pos / v3.posH.z * t) * depth;
                
                shadeFragment(cur_frag, pos_origin);
                
                fragment->push_back(cur_frag);
                zBuffer->push_back(depth);
                cav->setPixel(cur_frag.posX, cur_frag.posY, cur_frag.color);
            }
        }
    }
    delete v_homo_origin;
}

}
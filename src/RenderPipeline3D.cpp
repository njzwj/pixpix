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
#include <cstdio>
#include <algorithm>
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

/*!
    \brief Draw triangle.
    \param verts: vertex dictionary
    \param verts_homo: vertex homospace dictionary
    \param normal: vertex normal dictionary
    \param tex_coord: vertex texture coordinate dictionary
    \param tri_verts: triangle vertex index
*/
void
RenderPipeline3D::renderTriangle(
    vector<VEC3> *verts, vector<VEC4> *verts_homo, 
    vector<VEC3> *normal, vector<VEC2> *tex_coord,
    vector<unsigned> *tri_verts
) {
    /* clip triangle 
       In this implementation, i just throw the out-of-range triangles out.
       Also, the face which winds clock-wisely should be thrown out.
    */
    bool is_clipped = true;
    VEC3 v_origin[3];
    VEC4 v_homo[3];
    VEC3 v_normal[3];
    VEC2 v_tex_coord[3];
    VEC2 v2d[3];
    for (size_t i = 0; i < 3; ++i) {
        unsigned v_idx = (*tri_verts)[i];
        VEC4 v_h = (*verts_homo)[v_idx];
        v_homo[i] = v_h;
        v_origin[i] = (*verts)[v_idx];
        v_tex_coord[i] = (*tex_coord)[i];
        v_normal[i] = (*normal)[i];
        
        float x = v_h.getx(), y = v_h.gety(), z = v_h.z;
        v2d[i] = (VEC2){x, y};
        if (x >= -1 && x <= 1 && y >= -1 && y <= 1 && z >= camera->nearZ && z <= camera->farZ) {
            is_clipped = false;
        }
    }
    if (is_clipped) return;
    /* winding */
    if (((v2d[1]-v2d[0])^(v2d[2]-v2d[0])) < 0) return;
    
    /* fill triangle */
    /* triangle bound */
#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)>(b)?(a):(b))
    float minX=1, minY=1, maxX=-1, maxY=-1;
    for (size_t i = 0; i < 3; ++i) {
        minX = MIN(minX, v2d[i].x);
        minY = MIN(minY, v2d[i].y);
        maxX = MAX(maxX, v2d[i].x);
        maxY = MAX(maxY, v2d[i].y);
    }
    /*
     ^ y         O---> x
     |       ->  |        SCREEN COORDINATE
     O---> x     v y
    */
    minX = (float)(MAX(-1,minX)+1)/2*canvas->w;
    minY = (float)(-MAX(-1,minY)+1)/2*canvas->h;
    maxX = (float)(MIN(1,maxX)+1)/2*canvas->w;
    maxY = (float)(-MIN(1,maxY)+1)/2*canvas->h;
    swap(minY, maxY);
#undef MIN
#undef MAX
    
    /* screen coordinate */
#define TO_SCREEN(v) (VEC2){(float)(v.x+1)/2*canvas->w, (float)(-v.y+1)/2*canvas->h}
    for (size_t i = 0; i < 3; ++i) {
        v2d[i] = TO_SCREEN(v2d[i]);
    }
#undef TO_SCREEN
    for (unsigned x = minX; x <= maxX; ++x) {
        for (unsigned y = minY; y <= maxY; ++y) {
            VEC2 pos = (VEC2){(float)x, (float)y};
            /* judge if (x y) is in triangle */
#define BETWEEN(a, b, c, d) (((b-a)^(d-a))*((d-a)^(c-a))>=-1e-6)
            bool is_in_triangle = 
                BETWEEN(v2d[0], v2d[1], v2d[2], pos) &&
                BETWEEN(v2d[1], v2d[0], v2d[2], pos) &&
                BETWEEN(v2d[2], v2d[0], v2d[1], pos);
#undef BETWEEN
            if (!is_in_triangle) continue;
            
            /* interpolation */
            RASTERIZED_FRAGMENT cur_frag;
            VEC3 cur_frag_origin;
            float depth, s, t;
            bilinearInterpolation(v2d[0], v2d[1], v2d[2], pos, s, t);
            depth = 1.0f / ((1-s-t)/v_homo[0].z + s/v_homo[1].z + t/v_homo[2].z);
            cur_frag.posX = x; cur_frag.posY = y;
            /* z test */
            if (!zBufferTest(cur_frag, depth)) continue;
            cur_frag.normal = (v_normal[0]*(1-s-t)/v_homo[0].z + v_normal[1]*s/v_homo[1].z + v_normal[2]*t/v_homo[2].z) * depth;
            cur_frag.tex_coord = (v_tex_coord[0]*(1-s-t)/v_homo[0].z + v_tex_coord[1]*s/v_homo[1].z + v_tex_coord[2]*t/v_homo[2].z) * depth;
            cur_frag_origin = (v_origin[0]*(1-s-t)/v_homo[0].z + v_origin[1]*s/v_homo[1].z + v_origin[2]*t/v_homo[2].z) * depth;
            shadeFragment(cur_frag, cur_frag_origin);
            fragment->push_back(cur_frag);
            zBuffer->push_back(depth);
            canvas->setPixel(cur_frag.posX, cur_frag.posY, cur_frag.color);
        }
    }
}

/*
    \brief World space -> Homogenous Clipping Space
    \returns vertexes in homogenous clipping space
    \param verts: vertexes
*/
vector<VEC4> *
RenderPipeline3D::getVertexClipSpace(vector<VEC3> *verts) {
    vector<VEC4> *verts_homo = new vector<VEC4>;
    /* cam transform */
    MATRIX4 m_cam_space_trans = Math::matrixMul(
        Math::pitch_yaw_roll(-camera->rotation.x, -camera->rotation.y, -camera->rotation.z),
        Math::translation(-camera->position.x, -camera->position.y, -camera->position.z)
    );
    MATRIX4 m_homo_space_trans = Math::projection(camera->fovY, camera->aspect_ratio, camera->nearZ, camera->farZ);

    for (size_t i = 0; i < verts->size(); ++i) {
        VEC3 cur_v = (*verts)[i];
        VEC4 vert4 = (VEC4){cur_v.x, cur_v.y, cur_v.z, 1.0f};
        vert4 = Math::matrixVecMul(m_cam_space_trans, vert4);
        vert4.regularize();
        vert4 = Math::matrixVecMul(m_homo_space_trans, vert4);
        verts_homo->push_back(vert4);
    }
    
    return verts_homo;
}

void
RenderPipeline3D::init() {
    
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
    material = nullptr;
    
    canvas->clear();
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
RenderPipeline3D::addLight(LIGHT lgt) {
    light->push_back(lgt);
}

/*
    \brief Render 3d object. ONLY triangles are supported.
    \param mesh: 3d mesh object
*/
void 
RenderPipeline3D::render(MESH mesh) {
    /* vertex_homo */
    vector<VEC3> *verts = mesh.verts;
    vector<VEC4> *verts_homo = getVertexClipSpace(verts);
    
    /* draw triangle faces */
    vector<unsigned> *faceIndex = mesh.faceIndex;
    size_t p = 0;
    for (size_t i = 0; i < faceIndex->size(); ++i) {
        vector<unsigned> tri_verts;
        vector<VEC3> tri_normal;
        vector<VEC2> tri_tex_coord;
        tri_verts.clear();
        tri_normal.clear();
        tri_tex_coord.clear();
        for (size_t j = 0; j < (*faceIndex)[i]; ++j, ++p) {
            tri_verts.push_back((*(mesh.vertexIndex))[p]);
            tri_normal.push_back((*mesh.normal)[p]);
            tri_tex_coord.push_back((*mesh.texCoord)[p]);
        }
        renderTriangle(verts, verts_homo, &tri_normal, &tri_tex_coord, &tri_verts);
    }
    delete verts_homo;
}

}
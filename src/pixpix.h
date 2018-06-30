#ifndef __PIXPIX_H__
#define __PIXPIX_H__

#include <cmath>
#include <vector>
using namespace std;

namespace pixpix {

/* This is a simple 3d renderer               */
/* ============== FLOW CHART ================ */
/*  STAGE 1
    3d working space, containing the follows:
        i.    3d meshes, meterials, textures
        ii.   lights and environment
        iii.  camera and viewpoint
    STAGE 2
        i.    input the 3d components from stage 1
        ii.   vertex texture coordinate calculation
        iii.  vertex light & color calculation
    STAGE 3
        i.    convert 3d space into camera space
        ii.   convert 3d meshes into homogeneous clip space
        iii.  generate rasterized units using interpolation and other techniques
            - interpolation
            - z buffer
   ==========================================
*/

/* ========================================== */
/*       Essentials, math & other ADTs        */
/* ========================================== */

/* vector 2d  */
struct VEC2 {
    float x, y;
    VEC2 operator + (VEC2 rhs) const { return (VEC2){x+rhs.x, y+rhs.y}; }
    VEC2 operator - (VEC2 rhs) const { return (VEC2){x-rhs.x, y-rhs.y}; }
    float operator * (VEC2 rhs) const { return x * rhs.x + y * rhs.y; }
    float operator ^ (VEC2 rhs) const { return x * rhs.y - y * rhs.x; }
    VEC2 operator * (float rhs) const { return (VEC2){x * rhs, y * rhs}; }
    VEC2 operator / (float rhs) const { return (VEC2){x / rhs, y / rhs}; }
};

/* vector 3d */
struct VEC3 {
    float x, y, z;
    VEC3 operator + (VEC3 rhs) const { return (VEC3){x+rhs.x,y+rhs.y,z+rhs.z}; }
    VEC3 operator - (VEC3 rhs) const { return (VEC3){x-rhs.x,y-rhs.y,z-rhs.z}; }
    float operator * (VEC3 rhs) const { return x*rhs.x+y*rhs.y+z*rhs.z; }
    VEC3 operator * (float rhs) const { return (VEC3){x*rhs,y*rhs,z*rhs}; }
    VEC3 operator / (float rhs) const { return (VEC3){x/rhs,y/rhs,z/rhs}; }
    VEC3 operator ^ (VEC3 rhs) const {
        return (VEC3){y*rhs.z-z*rhs.y,
                      z*rhs.x-x*rhs.z,
                      x*rhs.y-y*rhs.x};
    }
    VEC3 normalize() {
        float len = sqrt(x*x+y*y+z*z);
        return (VEC3){x/len, y/len, z/len};
    }
};

/* vector 4d */
struct VEC4 {
    float x, y, z, w;
    VEC4 operator + (VEC4 rhs) const { return (VEC4){x+rhs.x,y+rhs.y,z+rhs.z,w+rhs.w}; }
    VEC4 operator - (VEC4 rhs) const { return (VEC4){x-rhs.x,y-rhs.y,z-rhs.z,w-rhs.w}; }
    VEC4 operator * (float rhs) const { return (VEC4){x*rhs,y*rhs,z*rhs,w*rhs}; }
    VEC4 operator / (float rhs) const { return (VEC4){x/rhs,y/rhs,z/rhs,w/rhs}; }
    float getx() { return x / w; }
    float gety() { return y / w; }
    float getz() { return z / w; }
    void regularize() { x /= w, y /= w, z /= w; w = 1.0; }
};

/* matrix 4x4 */
struct MATRIX4 {
    float mat[4][4];
    void setRow(unsigned row, float a, float b, float c, float d) {
        mat[row][0] = a; mat[row][1] = b; mat[row][2] = c; mat[row][3] = d;
    };
    void setColumn(unsigned col, float a, float b, float c, float d) {
        mat[0][col] = a; mat[1][col] = b; mat[2][col] = c; mat[3][col] = d;
    };
};

/* math helper */
class Math {
public:
    static constexpr float Pi = 3.1415926f;
    static MATRIX4 translation(float dx, float dy, float dz);
    static MATRIX4 rotationX(float angle);
    static MATRIX4 rotationY(float angle);
    static MATRIX4 rotationZ(float angle);
    static MATRIX4 yaw_pitch_roll(float yaw, float pitch, float roll);
    static MATRIX4 projection(float fovY, float aspect_ratio, float nearZ, float farZ);
    static MATRIX4 matrixMul(MATRIX4, MATRIX4);
    static VEC4 matrixVecMul(MATRIX4, VEC4);
};

/* color */
using COLOR3 = VEC3;    /* rgb  */
using COLOR4 = VEC4;    /* rgba */

/* vertex 3d */
struct VERTEX3 {
    VEC3 pos;           /* position in 3d space */
    COLOR4 color;       /* rgba color           */
    VEC3 normal;        /* normal of surface    */
    VEC2 tex_coord;     /* texture coordinate   */
};

/* vertex for render */
struct VERTEX_RENDER {
    VEC4 posH;          /* homogeneous position */
    COLOR4 color;       /* color                */
    VEC2 tex_coord;     /* texture coordinate   */
    VEC3 normal;
};

/* Rasterized Fragment for screen space output */
struct RASTERIZED_FRAGMENT {
    unsigned posX, posY;/* uint position        */
    COLOR4 color;       /* color                */
    VEC2 tex_coord;     /* texture coordinate   */
    VEC3 normal;
};

/* Canvas contains width, height and buffer using 8bit depth color */
struct CANVAS {
    unsigned w, h;
    unsigned char *img;
    CANVAS(unsigned w, unsigned h):w(w), h(h) {
        img = new unsigned char[w * h * 3];
        clear();
    };
    ~CANVAS() { delete img; }
    void clear() {
        for (int i = 0; i < w * h * 3; ++i) img[i] = 0;
    }
    void setPixel(unsigned x, unsigned y, COLOR4 color) {
        unsigned p = (x + y * w) * 3;
        img[p] = color.x * 255;
        img[p+1] = color.y * 255;
        img[p+2] = color.z * 255;
    };
};

/* ADTs, render data structures */
class CAMERA {
public:
    VEC3 position;                  /* camera position */
    VEC3 rotation;                  /* camera rotation */
    float fovY;                     /* field of view - vertical */
    float nearZ, farZ;              /* near - far clip distance */
    float aspect_ratio;             /* aspect ratio */

    CAMERA():position({0.0, 0.0, 0.0}), rotation({0.0, 0.0, 0.0}), fovY(Math::Pi / 2.0f), 
             nearZ(1.0), farZ(100.0), aspect_ratio(1.0) {}

    void setPosition(float x, float y, float z) {
        position = {x, y, z};
    }

    void lookAt(float x, float y, float z) {
        
    }

};

/* texture */
enum TEXTURE_TYPE {
    T_CHESS_BOARD,
    T_BITMAP,
    T_COLOR 
};

struct TEXTURE {
    TEXTURE_TYPE ty;
    union {
        /* chess board */
        struct {
            unsigned sz;
            COLOR4 color1;
            COLOR4 color2;
        };
        /* color */
        COLOR4 color;
    };
};

/* material */
struct MATERIAL {
    COLOR3 ambient;
    COLOR3 diffuse;
    COLOR3 specular;
    unsigned specularSmoothLevel;
    MATERIAL (): ambient( {0,0,0} ), diffuse( {1.0f, 0, 0} ), specular( {1.0f, 1.0f, 1.0f }), specularSmoothLevel(10) {}
};

/* light */
struct LIGHT {
    COLOR3 mAmbientColor;
    COLOR3 mDiffuseColor;
    COLOR3 mSpecularColor;
    VEC3  mPosition;
    float mSpecularIntensity;
    float mDiffuseIntensity;
    bool mIsEnabled;
    Light() {
        mSpecularIntensity = 1.0f;
        mDiffuseIntensity = 0.5f;
        mIsEnabled = true;
    }
};


/* ============================================ */
/*        Renderer, render pipelines            */
/* ============================================ */

/*
    \brief class RenderPipeline3D handles vertex light & color calc, 
           texture & material render, per pixel lighting.
*/
class RenderPipeline3D {
private:
    vector<VERTEX_RENDER> *vertex_homo;     /* homogeneous vertexes */
    vector<RASTERIZED_FRAGMENT> *fragment;  /* rasterized fragment  */
    vector<float> *zBuffer;                 /* z - buffer           */
    vector<LIGHT> *light;                   /* lights               */
    CANVAS *cav;

    TEXTURE *texture;
    MATERIAL *material;
    
    void bilinearInterpolation(VEC2, VEC2, VEC2, VEC2, float &, float &);
    COLOR4 getChessBoard(VEC2, unsigned, COLOR4, COLOR4);
    bool zBufferTest(RASTERIZED_FRAGMENT, float);
    void shadeFragment(RASTERIZED_FRAGMENT &, VEC3);
public:
    CAMERA *camera;                         /* camera */
    RenderPipeline3D():camera(new CAMERA()), vertex_homo(nullptr), fragment(nullptr), zBuffer(nullptr), light(nullptr) {}
    
    void init(CANVAS *);
    void setTexture(TEXTURE *);
    void setMaterial(MATERIAL *);
    void addLight(LIGHT);
    void render(vector<VERTEX3> *);
};

}

#endif
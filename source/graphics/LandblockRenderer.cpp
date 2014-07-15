#include "graphics/LandblockRenderer.h"
#include "graphics/Image.h"
#include "math/Vec3.h"
#include "Landblock.h"
#include <algorithm>
#include <vector>

// TODO We could only create one indexBuffer per subdivision level
// TODO We could generate the x and y of the vertex data in a shader

static const uint32_t LANDSCAPE_TEXTURES[] =
{
    0x00000000, // 0x00
    0x00000000, // 0x01
    0x00000000, // 0x02
    0x06003794, // 0x03
    0x00000000, // 0x04
    0x00000000, // 0x05
    0x00000000, // 0x06
    0x00000000, // 0x07
    0x00000000, // 0x08
    0x00000000, // 0x09
    0x00000000, // 0x0A
    0x00000000, // 0x0B
    0x00000000, // 0x0C
    0x00000000, // 0x0D
    0x00000000, // 0x0E
    0x00000000, // 0x0F
    0x00000000, // 0x10
    0x00000000, // 0x11
    0x00000000, // 0x12
    0x00000000, // 0x13
    0x00000000, // 0x14 
    0x0600379a, // 0x15
    0x00000000, // 0x16
    0x00000000, // 0x17
    0x00000000, // 0x18
    0x00000000, // 0x19
    0x00000000, // 0x1A
    0x00000000, // 0x1B
    0x00000000, // 0x1C
    0x00000000, // 0x1D
    0x00000000, // 0x1E
    0x00000000, // 0x1F
    // road textures below this line
    0x06006d3f  // 0x20
};

// 6d3e
// 6d49
// 6d51
// 6d06
// 6d3d
// 6d3f
// 6d48
// 6d46
// 6d42
// 6d41
// 6d6f
// 6d55
// 6d40
// 6d53
// 6d44
// 6d3c
// 6d50
// 6d4c
// 6d45
// 6bb5
// 6f48
// 6d54
// 6d56
// 6d4b
// 6d43
// 6810
// 6844
// 74d8
// 6d4d
// 680e
// 6d4f
// 6d4e
// 6d6a
// 6d4a
// 6b9c
// 6d47
// ----
// 74d9
// 3824
// 72b3
// 3835
// 382c
// 3828
// 382a
// 72b2
// 3821
// ....


static const int TERRAIN_ARRAY_SIZE = 512;
static const int TERRAIN_ARRAY_DEPTH = sizeof(LANDSCAPE_TEXTURES) / sizeof(LANDSCAPE_TEXTURES[0]);

static const uint32_t BLEND_TEXTURES[] =
{
    0x06006d61, // vertical, black to white, left of center
    0x06006d6c, // top left corner, black, semi ragged
    0x06006d6d, // top left corner, black, ragged
    0x06006d60, // top left corner, black, rounded
    0x06006d30, // vertical, black to white, very left of center, wavy
    0x00000000, // special case, all black
    0xFFFFFFFF  // special case, all white
};

static const int BLEND_ARRAY_SIZE = 512;
static const int BLEND_ARRAY_DEPTH = sizeof(BLEND_TEXTURES) / sizeof(BLEND_TEXTURES[0]);

//
// each landblock visual consists of a standard vertex buffer containing a 8x8x2 triangle list:
// x, y, z -- vertex
// nx, ny, nz -- normal
// s, t -- texture coords
// tz1 -- "white" texture index
// tz2 -- "black" texture index
// btx, bty -- blend texture coords
// btz -- blend texture index
//
// it also contains a 512x512 16-bit "displacement map"
// that describes the difference in height between the original, untesselated heightmap and the pretty one
// as a side note, each z may be between 0 and 512 in steps of two (it's an 8-bit value in double units)
// so a reasonable range for these displacements is -8 to 8
// given the map of resolution of 4096 values per unit (foot?)
// this map is sampled by the tess eval shader to adjust the height of the tesselated geometry
//
// the displacement map is generated by resizing the height map from 9x9 to 512x512 with Lanczos resampling
// and storing only the difference
//

LandblockRenderer::LandblockRenderer(const Landblock& landblock)
{
    auto& data = landblock.getRawData();

    vector<uint8_t> vertexData;

    for(auto y = 0; y < Landblock::GRID_SIZE - 1; y++)
    {
        for(auto x = 0; x < Landblock::GRID_SIZE - 1; x++)
        {
            // 4-3
            // |X|
            // 1-2

#define T(dx, dy) (data.styles[x + (dx)][y + (dy)] >> 2) & 0x1F
            // terrain types
            auto t1 = T(0, 0);
            auto t2 = T(1, 0);
            auto t3 = T(1, 1);
            auto t4 = T(0, 1);
#undef T

#define R(dx, dy) data.styles[x + (dx)][y + (dy)] & 0x3
            // roads
            auto r1 = R(0, 0);
            auto r2 = R(1, 0);
            auto r3 = R(1, 1);
            auto r4 = R(0, 1);
#undef R

            auto road = data.styles[x][y] & 0x3;
            //auto type = (data.styles[x][y] >> 10) & 0x1F;
            //auto vege = data.styles[x][y] & 0xFF;

            //for(int i = 0; i < 16; i++)
            //{
            //    printf("%d", (data.styles[x][y] >> i) & 1);
            //}
            //printf("\n");

            //auto a1 = data.styles[x][y];
            //auto a2 = data.styles[x + 1][y];
            //auto a3 = data.styles[x + 1][y + 1];
            //auto a4 = data.styles[x][y + 1];
            //printf("%02x\n", a1);//, a2, a3, a4);
            //printf("%02x\n", road);//, type, vege);//, r2, r3, r4);

            // mirror blend texture horizontally?
            auto mhorz = false;
            // mirror blend texture vertically?
            auto mvert = false;
            // road texture number
            auto rp = 0x20;
            // blend texture number
            auto bp = 3;

            if(r1 && r2 && r3 && r4)
            {
                // it's all road, baby
                //bp = 6; // all white
            }
            else if(r1 && r2 && !r3 && r4)
            {
                // road above
                //bp = 0; // verti
            }

// See LandVertexShader.glsl too see what these are
#define V(dx, dy) \
    vertexData.push_back(x + (dx)); \
    vertexData.push_back(y + (dy)); \
    vertexData.push_back(data.heights[x + (dx)][y + (dy)]); \
    vertexData.push_back(dx); \
    vertexData.push_back(dy); \
    vertexData.push_back(mhorz ? 1 - (dx) : (dx)); \
    vertexData.push_back(mvert ? 1 - (dy) : (dy)); \
    vertexData.push_back(t1); \
    vertexData.push_back(t2); \
    vertexData.push_back(t3); \
    vertexData.push_back(t4); \
    vertexData.push_back(rp); \
    vertexData.push_back(bp);

            if(landblock.splitNESW(x, y))
            {
                // lower right triangle
                V(0, 0) V(1, 0) V(1, 1)

                // top left triangle
                V(1, 1) V(0, 1) V(0, 0)
            }
            else
            {
                // lower left triangle
                V(0, 0) V(1, 0) V(0, 1)

                // top right triangle
                V(1, 0) V(1, 1) V(0, 1)
            }
#undef V
        }

        printf("\n");
    }

    _vertexBuffer.create();
    _vertexBuffer.bind(GL_ARRAY_BUFFER);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(uint8_t), vertexData.data(), GL_STATIC_DRAW);

    // 13 components per vertex
    _vertexCount = vertexData.size() / 13;

    initTerrainTexture();
    initBlendTexture();
}

/*
LandblockRenderer::LandblockRenderer(const Landblock& landblock)
{
    auto data = landblock.getSubdividedData();
    auto size = (int)landblock.getSubdividedSize();

    vector<float> vertexData;

    for(auto y = 0; y < size; y++)
    {
        for(auto x = 0; x < size; x++)
        {
            auto hc = data[x + y * size];
            auto hw = data[max(x - 1, 0) + y * size];
            auto he = data[min(x + 1, size - 1) + y * size];
            auto hs = data[x + max(y - 1, 0) * size];
            auto hn = data[x + min(y + 1, size - 1) * size];

            double quadSize = Landblock::LANDBLOCK_SIZE / double(size - 1);

            Vec3 ve(quadSize, 0.0, he - hc);
            Vec3 vn(0.0, quadSize, hn - hc);
            Vec3 vw(-quadSize, 0.0, hw - hc);
            Vec3 vs(0.0, -quadSize, hs - hc);

            auto nen = ve.cross(vn);
            auto nnw = vn.cross(vw);
            auto nws = vw.cross(vs);
            auto nse = vs.cross(ve);

            auto n = (nen + nnw + nws + nse).normalize();

            auto lx = double(x) / double(size - 1) * Landblock::LANDBLOCK_SIZE;
            auto ly = double(y) / double(size - 1) * Landblock::LANDBLOCK_SIZE;

            //auto style = landblock.getStyle(Vec2(lx, ly));
            //auto texIndex = (style >> 2) & 0x1F;

            // x, y, z
            vertexData.push_back(lx);
            vertexData.push_back(ly);
            vertexData.push_back(data[x + y * size]);
            // nx, ny, nz
            vertexData.push_back(n.x);
            vertexData.push_back(n.y);
            vertexData.push_back(n.z);
            // s, t, tz
            vertexData.push_back(double(x) / double(size - 1) * (Landblock::GRID_SIZE - 1));
            vertexData.push_back(double(y) / double(size - 1) * (Landblock::GRID_SIZE - 1));
            vertexData.push_back(4.0);
        }
    }

    vector<uint16_t> indexData;

    // B-D-F
    // |\|\| ...
    // A-C-E
    // http://en.wikipedia.org/wiki/Triangle_strip
    for(auto y = 0; y < size - 1; y++)
    {
        if(y != 0)
        {
            // end old triangle strip
            indexData.push_back(0xffff);
        }

        auto x = 0;
        
        // begin new triangle strip
        indexData.push_back(x + y * size);
        indexData.push_back(x + (y + 1) * size);

        for(x = 0; x < size - 1; x++)
        {
            indexData.push_back((x + 1) + y * size);
            indexData.push_back((x + 1) + (y + 1) * size);
        }
    }

    assert(indexData.size() < 0xffff);

    _vertexBuffer.create();
    _vertexBuffer.bind(GL_ARRAY_BUFFER);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    _indexBuffer.create();
    _indexBuffer.bind(GL_ELEMENT_ARRAY_BUFFER);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size() * sizeof(uint16_t), indexData.data(), GL_STATIC_DRAW);

    _indexCount = indexData.size();

    initTerrainTexture();
    initBlendTexture();
}
*/

LandblockRenderer::~LandblockRenderer()
{
    cleanupTerrainTexture();
    cleanupBlendTexture();

    _vertexBuffer.destroy();
}

void LandblockRenderer::render()
{
    _vertexBuffer.bind(GL_ARRAY_BUFFER);

    glVertexAttribPointer(0, 3, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(uint8_t) * 13, nullptr);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(uint8_t) * 13, (GLvoid*)(sizeof(uint8_t) * 3));
    glVertexAttribPointer(2, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(uint8_t) * 13, (GLvoid*)(sizeof(uint8_t) * 5));
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(uint8_t) * 13, (GLvoid*)(sizeof(uint8_t) * 7));
    glVertexAttribPointer(4, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(uint8_t) * 13, (GLvoid*)(sizeof(uint8_t) * 11));
    glVertexAttribPointer(5, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(uint8_t) * 13, (GLvoid*)(sizeof(uint8_t) * 12));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _terrainTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _blendTexture);

    glDrawArrays(GL_TRIANGLES, 0, _vertexCount);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);
}

void LandblockRenderer::initTerrainTexture()
{
    // allocate terrain texture
    glGenTextures(1, &_terrainTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _terrainTexture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // default is GL_NEAREST_MIPMAP_LINEAR
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, TERRAIN_ARRAY_SIZE, TERRAIN_ARRAY_SIZE, TERRAIN_ARRAY_DEPTH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // populate terrain texture 
    for(auto i = 0; i < TERRAIN_ARRAY_DEPTH; i++)
    {
        Image image;

        if(LANDSCAPE_TEXTURES[i] == 0x00000000)
        {
            image.create(Image::RGB24, TERRAIN_ARRAY_SIZE, TERRAIN_ARRAY_SIZE);
        }
        else
        {
            image.load(LANDSCAPE_TEXTURES[i]);

            if(image.width() == image.height() && image.width() * 2 == TERRAIN_ARRAY_SIZE)
            {
                image.scale(2.0);
            }
        }

        if(image.width() != TERRAIN_ARRAY_SIZE || image.height() != TERRAIN_ARRAY_SIZE)
        {
            throw runtime_error("Bad terrain image size");
        }

        GLenum format;

        if(image.format() == Image::RGB24)
        {
            format = GL_RGB;
        }
        else if(image.format() == Image::BGR24)
        {
            format = GL_BGR;
        }
        else if(image.format() == Image::BGRA32)
        {
            format = GL_BGRA;
        }
        else
        {
            throw runtime_error("Bad terrain image format");
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, TERRAIN_ARRAY_SIZE, TERRAIN_ARRAY_SIZE, 1, format, GL_UNSIGNED_BYTE, image.data());
    }
}

void LandblockRenderer::cleanupTerrainTexture()
{
    glDeleteTextures(1, &_terrainTexture);
}

void LandblockRenderer::initBlendTexture()
{
    // allocate terrain texture
    glGenTextures(1, &_blendTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _blendTexture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // default is GL_NEAREST_MIPMAP_LINEAR
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, BLEND_ARRAY_SIZE, BLEND_ARRAY_SIZE, BLEND_ARRAY_DEPTH, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // populate terrain texture 
    for(auto i = 0; i < BLEND_ARRAY_DEPTH; i++)
    {
        Image image;

        printf("loading %08x\n", BLEND_TEXTURES[i]);

        if(BLEND_TEXTURES[i] == 0x00000000)
        {
            image.create(Image::A8, BLEND_ARRAY_SIZE, BLEND_ARRAY_SIZE);
        }
        else if(BLEND_TEXTURES[i] == 0xFFFFFFFF)
        {
            // TODO MAKE WHITE
            image.create(Image::A8, BLEND_ARRAY_SIZE, BLEND_ARRAY_SIZE);
        }
        else
        {
            image.load(BLEND_TEXTURES[i]);

            printf("got image width=%d height=%d format=%d", image.width(), image.height(), image.format());
        }

        //image.fill(0xFF);

        if(image.width() != BLEND_ARRAY_SIZE || image.height() != BLEND_ARRAY_SIZE)
        {
            throw runtime_error("Bad terrain image size");
        }

        if(image.format() != Image::A8)
        {
            throw runtime_error("Bad terrain image format");
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, BLEND_ARRAY_SIZE, BLEND_ARRAY_SIZE, 1, GL_RED, GL_UNSIGNED_BYTE, image.data());
    }
}

void LandblockRenderer::cleanupBlendTexture()
{
    glDeleteTextures(1, &_blendTexture);
}

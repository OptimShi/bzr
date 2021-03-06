/*
 * Bael'Zharon's Respite
 * Copyright (C) 2014 Daniel Skorupski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "graphics/SkyRenderer.h"
#include "graphics/SkyModel.h"
#include "graphics/util.h"
#include "Camera.h"
#include "Core.h"

#include "graphics/shaders/SkyVertexShader.h"
#include "graphics/shaders/SkyFragmentShader.h"

static const int kCubeSize = 256;

SkyRenderer::SkyRenderer()
{
    initProgram();
    initGeometry();
    initTexture();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

SkyRenderer::~SkyRenderer()
{
    program_.destroy();
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteTextures(1, &texture_);
}

void SkyRenderer::render()
{
    program_.use();

    glm::mat4 rotationMat = glm::mat4_cast(glm::conjugate(Core::get().camera().rotationQuat()));

    loadMat4ToUniform(rotationMat, program_.getUniform("rotationMat"));

    glBindVertexArray(vertexArray_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_);

    glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);

    glEnable(GL_DEPTH_TEST);
}

void SkyRenderer::initProgram()
{
    program_.create();
    program_.attach(GL_VERTEX_SHADER, SkyVertexShader);
    program_.attach(GL_FRAGMENT_SHADER, SkyFragmentShader);
    program_.link();
}

void SkyRenderer::initGeometry()
{
    static float VERTEX_DATA[]
    {
        -1.0, -1.0,
         1.0, -1.0,
        -1.0,  1.0,
         1.0,  1.0,
        -1.0,  1.0,
         1.0, -1.0
    };

    static const int kComponentsPerVertex = 2;

    vertexCount_ = sizeof(VERTEX_DATA) / sizeof(VERTEX_DATA[0]) / kComponentsPerVertex;

    glGenVertexArrays(1, &vertexArray_);
    glBindVertexArray(vertexArray_);

    glGenBuffers(1, &vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_DATA), VERTEX_DATA, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * kComponentsPerVertex, nullptr);

    glEnableVertexAttribArray(0);
}

void SkyRenderer::initTexture()
{
    static const GLenum kFaces[]
    {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    static const int kNumkFaces = sizeof(kFaces) / sizeof(kFaces[0]);

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    SkyModel model;

    SkyModel::Params params;
    params.dt = fp_t(180.0);
    params.tm = fp_t(0.7);
    params.lng = fp_t(0.0);
    params.lat = fp_t(0.0);
    params.tu = fp_t(5.0);
    model.prepare(params);

    vector<uint8_t> data(kCubeSize * kCubeSize * 3);

    for(int face = 0; face < kNumkFaces; face++)
    {
        for(int j = 0; j < kCubeSize; j++)
        {
            for(int i = 0; i < kCubeSize; i++)
            {
                // scale to cube face
                fp_t fi = fp_t(i) / fp_t(kCubeSize - 1) * fp_t(2.0) - fp_t(1.0);
                fp_t fj = fp_t(j) / fp_t(kCubeSize - 1) * fp_t(2.0) - fp_t(1.0);

                // find point on the cube we're mapping
                glm::vec3 cp;

                switch(face)
                {
                    case 0: cp = glm::vec3{ 1.0,  -fj,  -fi}; break; // +X
                    case 1: cp = glm::vec3{-1.0,  -fj,   fi}; break; // -X
                    case 2: cp = glm::vec3{  fi,  1.0,   fj}; break; // +Y
                    case 3: cp = glm::vec3{  fi, -1.0,  -fj}; break; // -Y
                    case 4: cp = glm::vec3{  fi,  -fj,  1.0}; break; // +Z
                    case 5: cp = glm::vec3{ -fi,  -fj, -1.0}; break; // -Z
                }

                // map cube to sphere
                // http://mathproofs.blogspot.com/2005/07/mapping-cube-to-sphere.html
                glm::vec3 sp;

                sp.x = cp.x * glm::sqrt(fp_t(1.0) - cp.y * cp.y / fp_t(2.0) - cp.z * cp.z / fp_t(2.0) + cp.y * cp.y * cp.z * cp.z / fp_t(3.0));
                sp.y = cp.y * glm::sqrt(fp_t(1.0) - cp.z * cp.z / fp_t(2.0) - cp.x * cp.x / fp_t(2.0) + cp.z * cp.z * cp.x * cp.x / fp_t(3.0));
                sp.z = cp.z * glm::sqrt(fp_t(1.0) - cp.x * cp.x / fp_t(2.0) - cp.y * cp.y / fp_t(2.0) + cp.x * cp.x * cp.y * cp.y / fp_t(3.0));

                // convert cartesian to spherical
                // http://en.wikipedia.org/wiki/Spherical_coordinate_system#Cartesian_coordinates
                // phi is ccw from -y, not ccw from +x
                fp_t theta = acos(sp.z / sqrt(sp.x * sp.x + sp.y * sp.y + sp.z * sp.z));
                fp_t phi = atan2(sp.x, -sp.y);

                // pull sky edge below land edge
                theta *= fp_t(0.9);

                // compute and store color
                glm::vec3 color = model.getColor(theta, phi);
                data[(i + j * kCubeSize) * 3] = static_cast<uint8_t>(color.x * 0xFF);
                data[(i + j * kCubeSize) * 3 + 1] = static_cast<uint8_t>(color.y * 0xFF);
                data[(i + j * kCubeSize) * 3 + 2] = static_cast<uint8_t>(color.z * 0xFF);
            }
        }

        glTexImage2D(kFaces[face], 0, GL_RGB8, kCubeSize, kCubeSize, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    }

    sunVector_.x = sin(model.thetaSun()) * sin(model.phiSun());
    sunVector_.y = -sin(model.thetaSun()) * cos(model.phiSun());
    sunVector_.z = cos(model.thetaSun());
}

const glm::vec3& SkyRenderer::sunVector() const
{
    return sunVector_;
}

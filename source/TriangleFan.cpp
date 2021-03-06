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
#include "TriangleFan.h"
#include "BinReader.h"

// enum StipplingType
enum StipplingType
{
    kNoStippling = 0,
    kPositiveStippling = 1,
    kNegativeStippling = 2,
    kBothStippling = 3,
    kNoPosUVs = 4,
    kNoNegUVs = 8,
};

// enum SidesType
enum SidesType
{
    kSingle = 0,
    kDouble = 1,
    kBoth = 2
};

void read(BinReader& reader, TriangleFan& trifan)
{
    uint8_t numIndices = reader.readByte();
    trifan.indices.resize(numIndices);

    trifan.stipplingType = reader.readByte();
    assert(trifan.stipplingType == kNoStippling || trifan.stipplingType == kPositiveStippling || trifan.stipplingType == kNoPosUVs);

    uint32_t sidesType = reader.readInt();
    assert(sidesType == kSingle || sidesType == kDouble || sidesType == kBoth);

    trifan.surfaceIndex = reader.readShort();

    /*negSurface*/ reader.readShort();

    for(TriangleFan::Index& index : trifan.indices)
    {
        index.vertexIndex = reader.readShort();
    }

    if(trifan.stipplingType != kNoPosUVs)
    {
        for(TriangleFan::Index& index : trifan.indices)
        {
            index.texCoordIndex = reader.readByte();
        }
    }

    if(sidesType == kBoth)
    {
        for(uint8_t i = 0; i < numIndices; i++)
        {
            reader.readByte();
        }
    }
}

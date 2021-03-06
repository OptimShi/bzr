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
#include "resource/AnimationFrame.h"
#include "resource/AnimationHook.h"
#include "BinReader.h"

AnimationFrame::AnimationFrame()
{}

AnimationFrame::AnimationFrame(AnimationFrame&& other)
{
    locations = move(other.locations);
}

AnimationFrame::~AnimationFrame()
{}

AnimationFrame& AnimationFrame::operator=(AnimationFrame&& other)
{
    locations = move(other.locations);
    return *this;
}

void read(BinReader& reader, AnimationFrame& frame, uint32_t numModels)
{
    frame.locations.resize(numModels);

    for(Location& loc : frame.locations)
    {
        read(reader, loc);
    }

    uint32_t numHooks = reader.readInt();
    frame.hooks.resize(numHooks);

    for(unique_ptr<AnimationHook>& hook : frame.hooks)
    {
        read(reader, hook);
    }
}

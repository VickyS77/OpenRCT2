/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../../entity/EntityRegistry.h"
#include "../../interface/Viewport.h"
#include "../../paint/Paint.h"
#include "../../paint/Supports.h"
#include "../Ride.h"
#include "../RideEntry.h"
#include "../Track.h"
#include "../TrackPaint.h"
#include "../Vehicle.h"

enum
{
    SPR_MOTION_SIMULATOR_STAIRS_R0 = 22154,
    SPR_MOTION_SIMULATOR_STAIRS_R1 = 22155,
    SPR_MOTION_SIMULATOR_STAIRS_R2 = 22156,
    SPR_MOTION_SIMULATOR_STAIRS_R3 = 22157,
    SPR_MOTION_SIMULATOR_STAIRS_RAIL_R0 = 22158,
    SPR_MOTION_SIMULATOR_STAIRS_RAIL_R1 = 22159,
    SPR_MOTION_SIMULATOR_STAIRS_RAIL_R2 = 22160,
    SPR_MOTION_SIMULATOR_STAIRS_RAIL_R3 = 22161,
};

static void PaintMotionSimulatorVehicle(
    paint_session* session, const Ride& ride, int8_t offsetX, int8_t offsetY, uint8_t direction, int32_t height,
    const TrackElement& trackElement)
{
    auto rideEntry = ride.GetRideEntry();
    if (rideEntry == nullptr)
        return;

    const TileElement* savedTileElement = static_cast<const TileElement*>(session->CurrentlyDrawnItem);
    CoordsXYZ offset(offsetX, offsetY, height + 2);

    Vehicle* vehicle = nullptr;
    if (ride.lifecycle_flags & RIDE_LIFECYCLE_ON_TRACK)
    {
        vehicle = GetEntity<Vehicle>(ride.vehicles[0]);
        if (vehicle != nullptr)
        {
            session->InteractionType = ViewportInteractionItem::Entity;
            session->CurrentlyDrawnItem = vehicle;
        }
    }

    auto imageIndex = rideEntry->vehicles[0].base_image_id + direction;
    if (vehicle != nullptr)
    {
        if (vehicle->restraints_position >= 64)
        {
            imageIndex += (vehicle->restraints_position >> 6) << 2;
        }
        else
        {
            imageIndex += vehicle->Pitch * 4;
        }
    }

    auto imageTemplate = ImageId(0, ride.vehicle_colours[0].Body, ride.vehicle_colours[0].Trim);
    auto imageFlags = session->TrackColours[SCHEME_MISC];
    if (imageFlags != IMAGE_TYPE_REMAP)
    {
        imageTemplate = ImageId::FromUInt32(imageFlags);
    }
    auto simulatorImageId = imageTemplate.WithIndex(imageIndex);
    auto stairsImageId = imageTemplate.WithIndex(SPR_MOTION_SIMULATOR_STAIRS_R0 + direction);
    auto stairsRailImageId = imageTemplate.WithIndex(SPR_MOTION_SIMULATOR_STAIRS_RAIL_R0 + direction);
    switch (direction)
    {
        case 0:
            PaintAddImageAsParent(session, simulatorImageId, offset, { 20, 20, 44 }, offset);
            PaintAddImageAsChild(session, stairsImageId, offset, { 20, 20, 44 }, offset);
            PaintAddImageAsParent(session, stairsRailImageId, offset, { 20, 2, 44 }, { offset.x, offset.y + 32, offset.z });
            break;
        case 1:
            PaintAddImageAsParent(session, simulatorImageId, offset, { 20, 20, 44 }, offset);
            PaintAddImageAsChild(session, stairsImageId, offset, { 20, 20, 44 }, offset);
            PaintAddImageAsParent(session, stairsRailImageId, offset, { 2, 20, 44 }, { offset.x + 34, offset.y, offset.z });
            break;
        case 2:
            PaintAddImageAsParent(session, stairsRailImageId, offset, { 20, 2, 44 }, { offset.x, offset.y - 10, offset.z });
            PaintAddImageAsParent(session, stairsImageId, offset, { 20, 20, 44 }, { offset.x, offset.y + 5, offset.z });
            PaintAddImageAsChild(session, simulatorImageId, offset, { 20, 20, 44 }, { offset.x, offset.y + 5, offset.z });
            break;
        case 3:
            PaintAddImageAsParent(session, stairsRailImageId, offset, { 2, 20, 44 }, { offset.x - 10, offset.y, offset.z });
            PaintAddImageAsParent(session, stairsImageId, offset, { 20, 20, 44 }, { offset.x + 5, offset.y, offset.z });
            PaintAddImageAsChild(session, simulatorImageId, offset, { 20, 20, 44 }, { offset.x + 5, offset.y, offset.z });
            break;
    }

    session->CurrentlyDrawnItem = savedTileElement;
    session->InteractionType = ViewportInteractionItem::Ride;
}

static void PaintMotionSimulator(
    paint_session* session, const Ride* ride, uint8_t trackSequence, uint8_t direction, int32_t height,
    const TrackElement& trackElement)
{
    trackSequence = track_map_2x2[direction][trackSequence];

    int32_t edges = edges_2x2[trackSequence];

    wooden_a_supports_paint_setup(session, (direction & 1), 0, height, session->TrackColours[SCHEME_MISC]);

    StationObject* stationObject = nullptr;
    if (ride != nullptr)
        stationObject = ride_get_station_object(ride);

    track_paint_util_paint_floor(session, edges, session->TrackColours[SCHEME_TRACK], height, floorSpritesCork, stationObject);

    if (ride != nullptr)
    {
        track_paint_util_paint_fences(
            session, edges, session->MapPosition, trackElement, ride, session->TrackColours[SCHEME_SUPPORTS], height,
            fenceSpritesRope, session->CurrentRotation);

        switch (trackSequence)
        {
            case 1:
                PaintMotionSimulatorVehicle(session, *ride, 16, -16, direction, height, trackElement);
                break;
            case 2:
                PaintMotionSimulatorVehicle(session, *ride, -16, 16, direction, height, trackElement);
                break;
            case 3:
                PaintMotionSimulatorVehicle(session, *ride, -16, -16, direction, height, trackElement);
                break;
        }
    }

    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height + 128, 0x20);
}

/**
 *
 *  rct2: 0x00763520
 */
TRACK_PAINT_FUNCTION get_track_paint_function_motionsimulator(int32_t trackType)
{
    switch (trackType)
    {
        case TrackElemType::FlatTrack2x2:
            return PaintMotionSimulator;
    }
    return nullptr;
}

/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../Paint.h"

#include "../../Context.h"
#include "../../Game.h"
#include "../../GameState.h"
#include "../../config/Config.h"
#include "../../drawing/LightFX.h"
#include "../../interface/Viewport.h"
#include "../../localisation/Localisation.h"
#include "../../object/EntranceObject.h"
#include "../../object/ObjectManager.h"
#include "../../object/StationObject.h"
#include "../../ride/RideData.h"
#include "../../ride/TrackDesign.h"
#include "../../world/Banner.h"
#include "../../world/Entrance.h"
#include "../../world/Footpath.h"
#include "../../world/Park.h"
#include "../../world/TileInspector.h"
#include "../Supports.h"
#include "Paint.TileElement.h"

using namespace OpenRCT2;

static void PaintRideEntranceExitScrollingText(
    paint_session* session, const EntranceElement& entranceEl, const StationObject& stationObj, Direction direction,
    int32_t height)
{
    if (stationObj.ScrollingMode == SCROLLING_MODE_NONE)
        return;

    if (entranceEl.GetEntranceType() == ENTRANCE_TYPE_RIDE_EXIT)
        return;

    auto ride = get_ride(entranceEl.GetRideIndex());
    if (ride == nullptr)
        return;

    auto ft = Formatter();
    ft.Add<rct_string_id>(STR_RIDE_ENTRANCE_NAME);
    if (ride->status == RideStatus::Open && !(ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN))
    {
        ride->FormatNameTo(ft);
    }
    else
    {
        ft.Add<rct_string_id>(STR_RIDE_ENTRANCE_CLOSED);
    }

    char text[256];
    if (gConfigGeneral.upper_case_banners)
    {
        format_string_to_upper(text, sizeof(text), STR_BANNER_TEXT_FORMAT, ft.Data());
    }
    else
    {
        format_string(text, sizeof(text), STR_BANNER_TEXT_FORMAT, ft.Data());
    }
    auto stringWidth = gfx_get_string_width(text, FontSpriteBase::TINY);
    auto scroll = stringWidth > 0 ? (gCurrentTicks / 2) % stringWidth : 0;

    PaintAddImageAsChild(
        session, scrolling_text_setup(session, STR_BANNER_TEXT_FORMAT, ft, scroll, stationObj.ScrollingMode, COLOUR_BLACK), 0,
        0, 28, 28, 51, height + stationObj.Height, 2, 2, height + stationObj.Height);
}

static void PaintRideEntranceExitLightEffects(paint_session* session, int32_t height, const EntranceElement& entranceEl)
{
#ifdef __ENABLE_LIGHTFX__
    if (lightfx_is_available())
    {
        if (entranceEl.GetEntranceType() == ENTRANCE_TYPE_RIDE_ENTRANCE)
        {
            lightfx_add_3d_light_magic_from_drawing_tile(session->MapPosition, 0, 0, height + 45, LightType::Lantern3);
        }

        switch (entranceEl.GetDirection())
        {
            case 0:
                lightfx_add_3d_light_magic_from_drawing_tile(session->MapPosition, 16, 0, height + 16, LightType::Lantern2);
                break;
            case 1:
                lightfx_add_3d_light_magic_from_drawing_tile(session->MapPosition, 0, -16, height + 16, LightType::Lantern2);
                break;
            case 2:
                lightfx_add_3d_light_magic_from_drawing_tile(session->MapPosition, -16, 0, height + 16, LightType::Lantern2);
                break;
            case 3:
                lightfx_add_3d_light_magic_from_drawing_tile(session->MapPosition, 0, 16, height + 16, LightType::Lantern2);
                break;
        }
    }
#endif
}

static void PaintRideEntranceExit(paint_session* session, uint8_t direction, int32_t height, const EntranceElement& entranceEl)
{
    auto rideIndex = entranceEl.GetRideIndex();
    if ((gTrackDesignSaveMode || (session->ViewFlags & VIEWPORT_FLAG_HIGHLIGHT_PATH_ISSUES))
        && (rideIndex != gTrackDesignSaveRideIndex))
    {
        return;
    }

    auto ride = get_ride(rideIndex);
    if (ride == nullptr)
    {
        return;
    }

    auto stationObj = ride_get_station_object(ride);
    if (stationObj == nullptr || stationObj->BaseImageId == ImageIndexUndefined)
    {
        return;
    }

    session->InteractionType = ViewportInteractionItem::Ride;

    PaintRideEntranceExitLightEffects(session, height, entranceEl);

    auto hasGlass = (stationObj->Flags & STATION_OBJECT_FLAGS::IS_TRANSPARENT) != 0;
    auto colourPrimary = ride->track_colour[0].main;
    auto colourSecondary = ride->track_colour[0].additional;
    auto imageTemplate = ImageId(0, colourPrimary, colourSecondary);
    ImageId glassImageTemplate;
    if (hasGlass)
    {
        glassImageTemplate = ImageId().WithTransparancy(colourPrimary);
    }

    if (entranceEl.IsGhost())
    {
        session->InteractionType = ViewportInteractionItem::None;
        imageTemplate = ImageId().WithRemap(FilterPaletteID::Palette44);
    }
    else if (OpenRCT2::TileInspector::IsElementSelected(reinterpret_cast<const TileElement*>(&entranceEl)))
    {
        imageTemplate = ImageId().WithRemap(FilterPaletteID::Palette44);
    }

    // Format modified to stop repeated code

    // Each entrance is split into 2 images for drawing
    // Certain entrance styles have another 2 images to draw for coloured windows

    auto isExit = entranceEl.GetEntranceType() == ENTRANCE_TYPE_RIDE_EXIT;
    auto lengthY = (direction & 1) ? 28 : 2;
    auto lengthX = (direction & 1) ? 2 : 28;
    auto lengthZ = isExit ? 35 : 51;

    // Back
    ImageIndex imageIndex = isExit ? stationObj->BaseImageId + direction + 8 : stationObj->BaseImageId + direction;
    ImageIndex glassImageIndex = isExit ? stationObj->BaseImageId + direction + 24 : stationObj->BaseImageId + direction + 16;
    PaintAddImageAsParent(
        session, imageTemplate.WithIndex(imageIndex), { 0, 0, height }, { lengthX, lengthY, lengthZ }, { 2, 2, height });
    if (hasGlass)
    {
        PaintAddImageAsChild(
            session, glassImageTemplate.WithIndex(glassImageIndex), { 0, 0, height }, { lengthX, lengthY, lengthZ },
            { 2, 2, height });
    }

    // Front
    imageIndex += 4;
    PaintAddImageAsParent(
        session, imageTemplate.WithIndex(imageIndex), { 0, 0, height }, { lengthX, lengthY, lengthZ },
        { (direction & 1) ? 28 : 2, (direction & 1) ? 2 : 28, height });
    if (hasGlass)
    {
        glassImageIndex += 4;
        PaintAddImageAsChild(
            session, glassImageTemplate.WithIndex(glassImageIndex), { 0, 0, height }, { lengthX, lengthY, lengthZ },
            { (direction & 1) ? 28 : 2, (direction & 1) ? 2 : 28, height });
    }

    paint_util_push_tunnel_rotated(session, direction, height, TUNNEL_SQUARE_FLAT);

    if (!entranceEl.IsGhost())
        PaintRideEntranceExitScrollingText(session, entranceEl, *stationObj, direction, height);

    auto supportsImageTemplate = imageTemplate;
    if (!entranceEl.IsGhost())
    {
        supportsImageTemplate = ImageId().WithPrimary(COLOUR_SATURATED_BROWN);
    }
    wooden_a_supports_paint_setup(session, direction & 1, 0, height, supportsImageTemplate);

    height += isExit ? 40 : 56;
    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height, 0x20);
}

static void PaintParkEntranceScrollingText(
    paint_session* session, const EntranceObject& entrance, Direction direction, int32_t height)
{
    if ((direction + 1) & (1 << 1))
        return;

    auto scrollingMode = entrance.GetScrollingMode();
    if (scrollingMode == SCROLLING_MODE_NONE)
        return;

    auto ft = Formatter();
    if (gParkFlags & PARK_FLAGS_PARK_OPEN)
    {
        const auto& park = OpenRCT2::GetContext()->GetGameState()->GetPark();
        auto name = park.Name.c_str();
        ft.Add<rct_string_id>(STR_STRING);
        ft.Add<const char*>(name);
    }
    else
    {
        ft.Add<rct_string_id>(STR_BANNER_TEXT_CLOSED);
        ft.Add<uint32_t>(0);
    }

    char text[256];
    if (gConfigGeneral.upper_case_banners)
    {
        format_string_to_upper(text, sizeof(text), STR_BANNER_TEXT_FORMAT, ft.Data());
    }
    else
    {
        format_string(text, sizeof(text), STR_BANNER_TEXT_FORMAT, ft.Data());
    }

    auto stringWidth = gfx_get_string_width(text, FontSpriteBase::TINY);
    auto scroll = stringWidth > 0 ? (gCurrentTicks / 2) % stringWidth : 0;
    auto imageIndex = scrolling_text_setup(
        session, STR_BANNER_TEXT_FORMAT, ft, scroll, scrollingMode + direction / 2, COLOUR_BLACK);
    auto textHeight = height + entrance.GetTextHeight();
    PaintAddImageAsChild(session, ImageId(imageIndex), { 0, 0, textHeight }, { 28, 28, 47 }, { 2, 2, textHeight });
}

static void PaintParkEntranceLightEffects(paint_session* session)
{
#ifdef __ENABLE_LIGHTFX__
    if (lightfx_is_available())
    {
        lightfx_add_3d_light_magic_from_drawing_tile(session->MapPosition, 0, 0, 155, LightType::Lantern3);
    }
#endif
}

static void PaintParkEntrance(paint_session* session, uint8_t direction, int32_t height, const EntranceElement& entranceEl)
{
    if (gTrackDesignSaveMode || (session->ViewFlags & VIEWPORT_FLAG_HIGHLIGHT_PATH_ISSUES))
        return;

    PaintParkEntranceLightEffects(session);

    session->InteractionType = ViewportInteractionItem::ParkEntrance;

    ImageId imageTemplate;
    if (entranceEl.IsGhost())
    {
        session->InteractionType = ViewportInteractionItem::None;
        imageTemplate = ImageId().WithRemap(FilterPaletteID::Palette44);
    }
    else if (OpenRCT2::TileInspector::IsElementSelected(reinterpret_cast<const TileElement*>(&entranceEl)))
    {
        imageTemplate = ImageId().WithRemap(FilterPaletteID::Palette44);
    }

    auto& objManager = GetContext()->GetObjectManager();
    auto entrance = reinterpret_cast<EntranceObject*>(objManager.GetLoadedObject(ObjectType::ParkEntrance, 0));
    auto sequence = entranceEl.GetSequenceIndex();
    switch (sequence)
    {
        case EntranceSequence::Centre:
        {
            // Footpath
            auto surfaceDescriptor = entranceEl.GetPathSurfaceDescriptor();
            if (surfaceDescriptor != nullptr)
            {
                auto imageIndex = (surfaceDescriptor->Image + 5 * (1 + (direction & 1)));
                PaintAddImageAsParent(
                    session, imageTemplate.WithIndex(imageIndex), { 0, 0, height }, { 32, 28, 0 }, { 0, 2, height });
            }

            // Entrance
            if (entrance != nullptr)
            {
                auto imageIndex = entrance->GetImage(sequence, direction);
                PaintAddImageAsParent(
                    session, imageTemplate.WithIndex(imageIndex), { 0, 0, height }, { 28, 28, 47 }, { 2, 2, height + 32 });

                if (!entranceEl.IsGhost())
                    PaintParkEntranceScrollingText(session, *entrance, direction, height);
            }
            break;
        }
        case EntranceSequence::Left:
        case EntranceSequence::Right:
            if (entrance != nullptr)
            {
                auto imageIndex = entrance->GetImage(sequence, direction);
                auto y = ((direction / 2 + sequence / 2) & 1) ? 26 : 32;
                PaintAddImageAsParent(
                    session, imageTemplate.WithIndex(imageIndex), { 0, 0, height }, { 26, y, 79 }, { 3, 3, height });
            }
            break;
    }

    auto supportsImageTemplate = imageTemplate;
    if (!entranceEl.IsGhost())
    {
        supportsImageTemplate = ImageId().WithPrimary(COLOUR_SATURATED_BROWN);
    }
    wooden_a_supports_paint_setup(session, direction & 1, 0, height, supportsImageTemplate);

    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height + 80, 0x20);
}

static void PaintHeightMarkers(paint_session* session, const EntranceElement& entranceEl, int32_t height)
{
    if (PaintShouldShowHeightMarkers(session, VIEWPORT_FLAG_PATH_HEIGHTS))
    {
        if (entranceEl.GetDirections() & 0xF)
        {
            auto heightMarkerBaseZ = entranceEl.GetBaseZ() + 3;
            ImageIndex baseImageIndex = SPR_HEIGHT_MARKER_BASE;
            baseImageIndex += heightMarkerBaseZ / 16;
            baseImageIndex += get_height_marker_offset();
            baseImageIndex -= gMapBaseZ;
            auto imageId = ImageId(baseImageIndex, COLOUR_GREY);
            PaintAddImageAsParent(session, imageId, { 16, 16, height }, { 1, 1, 0 }, { 31, 31, heightMarkerBaseZ + 64 });
        }
    }
}

void PaintEntrance(paint_session* session, uint8_t direction, int32_t height, const EntranceElement& entranceElement)
{
    session->InteractionType = ViewportInteractionItem::Label;

    PaintHeightMarkers(session, entranceElement, height);
    switch (entranceElement.GetEntranceType())
    {
        case ENTRANCE_TYPE_RIDE_ENTRANCE:
        case ENTRANCE_TYPE_RIDE_EXIT:
            PaintRideEntranceExit(session, direction, height, entranceElement);
            break;
        case ENTRANCE_TYPE_PARK_ENTRANCE:
            PaintParkEntrance(session, direction, height, entranceElement);
            break;
    }
}

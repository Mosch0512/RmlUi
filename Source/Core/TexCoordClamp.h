#pragma once

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

struct Mesh;

/*
    Linear filtering reaches half a texel around each sample. At a quad's outer pixels it therefore mixes in the
    texels just past the image's edge: the next sprite of a sheet, or, with a repeating sampler, the texture's
    opposite edge, and a sprite whose own edge is transparent shows a faint line along it. These helpers hold a
    quad's texture coordinates at least half a texel inside its sprite, adding the points needed so that the rest
    of the mapping, and so every pixel of the sprite, stays where it was.
*/

/// One point of a piecewise linear mapping, along one axis, from position to texture coordinate.
struct TexCoordStop {
	float position;
	float tex_coord;
};

/// Holds the mapping's texture coordinates within the sprite that spans [sprite_min, sprite_max] (texture
/// coordinates; `texel` is one texel's size in them), half a texel inside its edges: adds a stop where the mapping
/// crosses those limits and keeps the coordinate constant past them. A sprite no wider than a texel keeps its
/// centre. A mapping that reaches past the sprite tiles it, which is the sampler's to repeat, and is left alone, as
/// is one whose texel size is unknown (zero).
void ClampTexCoordStops(Vector<TexCoordStop>& stops, float sprite_min, float sprite_max, float texel);

/// Appends the grid of quads between the x and y stops to the mesh, neighbouring quads sharing their vertices.
/// Cells of zero width or height are left out.
void GenerateTexCoordGrid(Mesh& mesh, const Vector<TexCoordStop>& x_stops, const Vector<TexCoordStop>& y_stops, ColourbPremultiplied colour);

/// Like MeshUtilities::GenerateQuad, with the texture coordinates held inside the sprite spanning
/// [sprite_min, sprite_max] as ClampTexCoordStops() does, per axis.
void GenerateClampedQuad(Mesh& mesh, Vector2f origin, Vector2f dimensions, ColourbPremultiplied colour, Vector2f top_left_tex_coord,
	Vector2f bottom_right_tex_coord, Vector2f sprite_min, Vector2f sprite_max, Vector2f texel);

} // namespace Rml

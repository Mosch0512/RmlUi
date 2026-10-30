#include "TexCoordClamp.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/Mesh.h"
#include <utility>

namespace Rml {

void ClampTexCoordStops(Vector<TexCoordStop>& stops, float sprite_min, float sprite_max, float texel)
{
	if (stops.size() < 2 || !(texel > 0.f) || sprite_max < sprite_min)
		return;

	// Coordinates computed from a position over the texture size carry rounding error; a hundredth of a texel past the
	// sprite is still inside it.
	const float tolerance = 0.01f * texel;
	for (const TexCoordStop& stop : stops)
	{
		if (stop.tex_coord < sprite_min - tolerance || stop.tex_coord > sprite_max + tolerance)
			return;
	}

	float low = sprite_min + 0.5f * texel;
	float high = sprite_max - 0.5f * texel;
	if (low > high)
		low = high = 0.5f * (sprite_min + sprite_max);

	Vector<TexCoordStop> clamped;
	clamped.reserve(stops.size() + 2);
	auto add = [&clamped](float position, float tex_coord) {
		if (!clamped.empty() && clamped.back().position == position && clamped.back().tex_coord == tex_coord)
			return;
		clamped.push_back({position, tex_coord});
	};

	for (size_t i = 0; i + 1 < stops.size(); ++i)
	{
		const TexCoordStop a = stops[i];
		const TexCoordStop b = stops[i + 1];
		add(a.position, Math::Clamp(a.tex_coord, low, high));

		// Where the segment crosses a limit, in the order it meets them.
		const bool rising = a.tex_coord <= b.tex_coord;
		const float limits[2] = {rising ? low : high, rising ? high : low};
		for (const float limit : limits)
		{
			if ((a.tex_coord - limit) * (b.tex_coord - limit) < 0.f)
			{
				const float t = (limit - a.tex_coord) / (b.tex_coord - a.tex_coord);
				add(a.position + t * (b.position - a.position), limit);
			}
		}
	}
	add(stops.back().position, Math::Clamp(stops.back().tex_coord, low, high));

	stops = std::move(clamped);
}

void GenerateTexCoordGrid(Mesh& mesh, const Vector<TexCoordStop>& x_stops, const Vector<TexCoordStop>& y_stops, ColourbPremultiplied colour)
{
	if (x_stops.size() < 2 || y_stops.size() < 2)
		return;

	const int columns = static_cast<int>(x_stops.size());
	const int rows = static_cast<int>(y_stops.size());
	const int first = static_cast<int>(mesh.vertices.size());

	mesh.vertices.reserve(mesh.vertices.size() + size_t(columns) * size_t(rows));
	for (const TexCoordStop& y : y_stops)
	{
		for (const TexCoordStop& x : x_stops)
		{
			Vertex vertex;
			vertex.position = Vector2f(x.position, y.position);
			vertex.colour = colour;
			vertex.tex_coord = Vector2f(x.tex_coord, y.tex_coord);
			mesh.vertices.push_back(vertex);
		}
	}

	mesh.indices.reserve(mesh.indices.size() + size_t(columns - 1) * size_t(rows - 1) * 6);
	for (int y = 0; y + 1 < rows; ++y)
	{
		if (!(y_stops[y + 1].position > y_stops[y].position))
			continue;
		for (int x = 0; x + 1 < columns; ++x)
		{
			if (!(x_stops[x + 1].position > x_stops[x].position))
				continue;
			// The same winding as MeshUtilities::GenerateQuad().
			const int top_left = first + y * columns + x;
			const int top_right = top_left + 1;
			const int bottom_left = top_left + columns;
			const int bottom_right = bottom_left + 1;
			mesh.indices.insert(mesh.indices.end(), {top_left, bottom_left, top_right, top_right, bottom_left, bottom_right});
		}
	}
}

void GenerateClampedQuad(Mesh& mesh, Vector2f origin, Vector2f dimensions, ColourbPremultiplied colour, Vector2f top_left_tex_coord,
	Vector2f bottom_right_tex_coord, Vector2f sprite_min, Vector2f sprite_max, Vector2f texel)
{
	Vector<TexCoordStop> x_stops = {{origin.x, top_left_tex_coord.x}, {origin.x + dimensions.x, bottom_right_tex_coord.x}};
	Vector<TexCoordStop> y_stops = {{origin.y, top_left_tex_coord.y}, {origin.y + dimensions.y, bottom_right_tex_coord.y}};
	ClampTexCoordStops(x_stops, sprite_min.x, sprite_max.x, texel.x);
	ClampTexCoordStops(y_stops, sprite_min.y, sprite_max.y, texel.y);
	GenerateTexCoordGrid(mesh, x_stops, y_stops, colour);
}

} // namespace Rml

#include "../../../Source/Core/TexCoordClamp.h"
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <doctest.h>

using namespace Rml;

namespace {

// A stack of button states: the upper 76 x 23 sprite of a 76 x 46 sheet, drawn at 1.6x.
constexpr float sheet_height = 46.f;
constexpr float texel = 1.f / sheet_height;
constexpr float drawn = 23.f * 1.6f;

// The texture coordinate a mapping gives at `position`, interpolated between its stops.
float TexCoordAt(const Vector<TexCoordStop>& stops, float position)
{
	for (size_t i = 0; i + 1 < stops.size(); ++i)
	{
		if (position >= stops[i].position && position <= stops[i + 1].position && stops[i + 1].position > stops[i].position)
		{
			const float t = (position - stops[i].position) / (stops[i + 1].position - stops[i].position);
			return stops[i].tex_coord + t * (stops[i + 1].tex_coord - stops[i].tex_coord);
		}
	}
	return position <= stops.front().position ? stops.front().tex_coord : stops.back().tex_coord;
}

} // namespace

TEST_CASE("TexCoordClamp.sprite_edges")
{
	Vector<TexCoordStop> stops = {{0.f, 0.f}, {drawn, 0.5f}};
	ClampTexCoordStops(stops, 0.f, 0.5f, texel);

	// A band half a texel wide at both ends, holding the edge texel's centre.
	REQUIRE(stops.size() == 4);
	const float band = 0.5f * drawn / 23.f;
	CHECK(stops[0].tex_coord == doctest::Approx(0.5f * texel));
	CHECK(stops[1].position == doctest::Approx(band));
	CHECK(stops[1].tex_coord == doctest::Approx(0.5f * texel));
	CHECK(stops[2].position == doctest::Approx(drawn - band));
	CHECK(stops[2].tex_coord == doctest::Approx(22.5f * texel));
	CHECK(stops[3].tex_coord == doctest::Approx(22.5f * texel));

	// In between, every position keeps the coordinate the single quad gave it: no pixel of the sprite moves.
	for (float position = band; position <= drawn - band; position += 0.37f)
		CHECK(TexCoordAt(stops, position) == doctest::Approx(position / drawn * 0.5f));
}

TEST_CASE("TexCoordClamp.flipped")
{
	Vector<TexCoordStop> stops = {{0.f, 0.5f}, {drawn, 0.f}};
	ClampTexCoordStops(stops, 0.f, 0.5f, texel);
	REQUIRE(stops.size() == 4);
	CHECK(stops.front().tex_coord == doctest::Approx(22.5f * texel));
	CHECK(stops.back().tex_coord == doctest::Approx(0.5f * texel));
	for (const TexCoordStop& stop : stops)
	{
		CHECK(stop.tex_coord >= 0.5f * texel - 1e-6f);
		CHECK(stop.tex_coord <= 22.5f * texel + 1e-6f);
	}
}

TEST_CASE("TexCoordClamp.cut_ends")
{
	SUBCASE("cut more than half a texel inside the sprite: unchanged")
	{
		Vector<TexCoordStop> stops = {{0.f, 2.f * texel}, {10.f, 20.f * texel}};
		const Vector<TexCoordStop> before = stops;
		ClampTexCoordStops(stops, 0.f, 0.5f, texel);
		REQUIRE(stops.size() == 2);
		CHECK(stops[0].tex_coord == doctest::Approx(before[0].tex_coord));
		CHECK(stops[1].tex_coord == doctest::Approx(before[1].tex_coord));
	}

	SUBCASE("cut within half a texel of the sprite's edge: clamped there")
	{
		Vector<TexCoordStop> stops = {{0.f, 20.f * texel}, {10.f, 22.8f * texel}};
		ClampTexCoordStops(stops, 0.f, 0.5f, texel);
		REQUIRE(stops.size() == 3);
		CHECK(stops[1].tex_coord == doctest::Approx(22.5f * texel));
		CHECK(stops[2].tex_coord == doctest::Approx(22.5f * texel));
		CHECK(TexCoordAt(stops, 5.f) == doctest::Approx(21.4f * texel));
	}
}

TEST_CASE("TexCoordClamp.one_texel_sprite")
{
	// A one-pixel slice stretched to 40 px: it keeps its own texel's centre throughout.
	Vector<TexCoordStop> stops = {{0.f, 7.f / 15.f}, {40.f, 8.f / 15.f}};
	ClampTexCoordStops(stops, 7.f / 15.f, 8.f / 15.f, 1.f / 15.f);
	for (const TexCoordStop& stop : stops)
		CHECK(stop.tex_coord == doctest::Approx(7.5f / 15.f));
	CHECK(stops.front().position == 0.f);
	CHECK(stops.back().position == 40.f);
}

TEST_CASE("TexCoordClamp.left_alone")
{
	SUBCASE("tiling past the sprite")
	{
		Vector<TexCoordStop> stops = {{0.f, 0.f}, {100.f, 3.2f}};
		ClampTexCoordStops(stops, 0.f, 1.f, 1.f / 32.f);
		REQUIRE(stops.size() == 2);
		CHECK(stops[1].tex_coord == 3.2f);
	}

	SUBCASE("unknown texel size")
	{
		Vector<TexCoordStop> stops = {{0.f, 0.f}, {100.f, 0.5f}};
		ClampTexCoordStops(stops, 0.f, 0.5f, 0.f);
		REQUIRE(stops.size() == 2);
		CHECK(stops[0].tex_coord == 0.f);
		CHECK(stops[1].tex_coord == 0.5f);
	}
}

TEST_CASE("TexCoordClamp.several_segments")
{
	// A nine-patch row: corner, stretched middle, corner.
	Vector<TexCoordStop> stops = {{0.f, 0.f}, {16.f, 0.25f}, {84.f, 0.75f}, {100.f, 1.f}};
	ClampTexCoordStops(stops, 0.f, 1.f, 0.125f);
	REQUIRE(stops.size() == 6);
	CHECK(stops[1].position == doctest::Approx(4.f));
	CHECK(stops[2].position == 16.f);
	CHECK(stops[3].position == 84.f);
	CHECK(stops[4].position == doctest::Approx(96.f));
	CHECK(TexCoordAt(stops, 50.f) == doctest::Approx(0.5f));
}

TEST_CASE("TexCoordClamp.grid")
{
	const ColourbPremultiplied colour(255, 255, 255, 255);

	SUBCASE("one quad matches MeshUtilities::GenerateQuad")
	{
		Mesh grid, quad;
		GenerateTexCoordGrid(grid, {{0.f, 0.f}, {10.f, 1.f}}, {{0.f, 0.f}, {5.f, 1.f}}, colour);
		MeshUtilities::GenerateQuad(quad, Vector2f(0.f), Vector2f(10.f, 5.f), colour, Vector2f(0.f), Vector2f(1.f));
		REQUIRE(grid.indices.size() == quad.indices.size());
		// Same triangles, in the same winding.
		for (size_t i = 0; i < quad.indices.size(); ++i)
		{
			const Vertex& a = grid.vertices[grid.indices[i]];
			const Vertex& b = quad.vertices[quad.indices[i]];
			CHECK(a.position == b.position);
			CHECK(a.tex_coord == b.tex_coord);
		}
	}

	SUBCASE("shared vertices, zero-width cells left out")
	{
		Mesh mesh;
		GenerateTexCoordGrid(mesh, {{0.f, 0.f}, {4.f, 0.f}, {4.f, 0.5f}, {10.f, 1.f}}, {{0.f, 0.f}, {5.f, 1.f}}, colour);
		CHECK(mesh.vertices.size() == 8);
		CHECK(mesh.indices.size() == 2 * 6);
	}

	SUBCASE("clamped quad")
	{
		Mesh mesh;
		GenerateClampedQuad(mesh, Vector2f(0.f), Vector2f(76.f * 1.6f, drawn), colour, Vector2f(0.f), Vector2f(1.f, 0.5f), Vector2f(0.f),
			Vector2f(1.f, 0.5f), Vector2f(1.f / 76.f, texel));
		CHECK(mesh.vertices.size() == 16);
		CHECK(mesh.indices.size() == 9 * 6);
		for (const Vertex& vertex : mesh.vertices)
		{
			CHECK(vertex.tex_coord.y >= 0.5f * texel - 1e-6f);
			CHECK(vertex.tex_coord.y <= 22.5f * texel + 1e-6f);
		}
	}
}

#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String document_flex_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 300px;
			background: #333;
		}
		#flex {
			background: #666;
			display: flex;
			width: 50%;
			height: 30%;
		}
		input.checkbox {
			background: #fff;
			border: 1px #000;
		}
	</style>
</head>

<body>
	<div id="flex">
		<input type="checkbox" id="checkbox"/>
	</div>
</body>
</rml>
)";

TEST_CASE("FlexFormatting")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_flex_rml);
	REQUIRE(document);
	document->Show();

	Element* checkbox = document->GetElementById("checkbox");
	Element* flex = document->GetElementById("flex");

	struct TestCase {
		String flex_direction;
		String align_items;
		Vector2f expected_size;
	};
	TestCase test_cases[] = {
		{"column", "stretch", Vector2f(248, 16)},
		{"column", "center", Vector2f(16, 16)},
		{"row", "stretch", Vector2f(16, 88)},
		{"row", "center", Vector2f(16, 16)},
	};

	for (auto& test_case : test_cases)
	{
		flex->SetProperty("flex-direction", test_case.flex_direction);
		flex->SetProperty("align-items", test_case.align_items);

		TestsShell::RenderLoop();

		CAPTURE(test_case.align_items);
		CAPTURE(test_case.flex_direction);
		CHECK(checkbox->GetBox().GetSize().x == doctest::Approx(test_case.expected_size.x));
		CHECK(checkbox->GetBox().GetSize().y == doctest::Approx(test_case.expected_size.y));
	}

	document->Close();

	TestsShell::ShutdownShell();
}

static const String document_flex_dp_ratio_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 100%;
			height: 100%;
		}
		.window {
			width: 1920dp;
			height: 1080dp;
			margin: 0 auto;
			display: flex;
			flex-direction: column;
		}
		.inner {
			flex-grow: 1;
			display: flex;
			flex-direction: row;
		}
		.left {
			width: 30%;
			margin: 32dp;
			overflow: auto;
			background: #522;
		}
		.right {
			flex: 1;
			margin: 32dp;
			overflow: auto;
			background: #252;
		}
	</style>
</head>

<body>
	<div class="window">
		<div class="inner">
			<div class="left"/>
			<div class="right"/>
		</div>
	</div>
</body>
</rml>
)";

TEST_CASE("FlexFormatting.dp_ratio")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_flex_dp_ratio_rml);
	REQUIRE(document);
	document->Show();

	Element* window = document->GetChild(0);

	constexpr float native_width = 1920.f;
	constexpr float native_height = 1080.f;

	const float test_window_widths[] = {
		3440.f,
		2960.f,
		2880.f,
		2560.f,
		2400.f,
		2048.f,
		1921.f,
		1920.f,
		1919.f,
		1600.f,
		1366.f,
		1280.f,
		1024.f,
	};

	for (float window_width : test_window_widths)
	{
		const float dp_ratio = window_width / native_width;
		CAPTURE(window_width);
		CAPTURE(dp_ratio);

		context->SetDensityIndependentPixelRatio(dp_ratio);

		TestsShell::RenderLoop();

		CHECK(window->GetBox().GetSize().x == window_width);
		CHECK(window->GetBox().GetSize().y == doctest::Approx((window_width / native_width) * native_height));
	}

	document->Close();

	TestsShell::ShutdownShell();
}

static const String document_flex_absolute_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 500px;
		}
		.band {
			display: flex;
			position: relative;
			min-height: 40px;
			padding: 5px 0;
		}
		.column {
			flex-direction: column;
			min-height: 0;
		}
		.short {
			width: 20px;
			height: 10px;
		}
		.tall {
			width: 20px;
			height: 60px;
		}
		.art {
			position: absolute;
			top: 0;
			bottom: 0;
			left: 0;
			width: 10px;
		}
		.half {
			position: absolute;
			top: 0;
			left: 0;
			width: 10px;
			height: 50%;
		}
	</style>
</head>

<body>
	<div class="band" id="min_height">
		<div class="short"/>
		<div class="art" id="min_height_art"/>
	</div>
	<div class="band" id="grown">
		<div class="tall"/>
		<div class="art" id="grown_art"/>
		<div class="half" id="grown_half"/>
	</div>
	<div class="band column" id="column">
		<div class="tall"/>
		<div class="tall"/>
		<div class="art" id="column_art"/>
	</div>
</body>
</rml>
)";

TEST_CASE("FlexFormatting.absolute_child_of_auto_height_container")
{
	// Absolutely positioned children of an auto-height flex container must be sized against the container's final
	// height, not the indefinite height it had before its flex items were laid out.
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_flex_absolute_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	struct TestCase {
		String container_id;
		String child_id;
		float container_height;
		float child_height;
	};
	const TestCase test_cases[] = {
		// A 10px flex item leaves the band at its min-height: 40 + 2 * 5 padding.
		{"min_height", "min_height_art", 40.f, 50.f},
		// A 60px flex item grows the band past its min-height.
		{"grown", "grown_art", 60.f, 70.f},
		{"grown", "grown_half", 60.f, 35.f},
		// Column direction: two 60px items stacked.
		{"column", "column_art", 120.f, 130.f},
	};

	for (const TestCase& test_case : test_cases)
	{
		CAPTURE(test_case.child_id);

		Element* container = document->GetElementById(test_case.container_id);
		Element* child = document->GetElementById(test_case.child_id);
		REQUIRE(container);
		REQUIRE(child);

		CHECK(container->GetBox().GetSize().y == doctest::Approx(test_case.container_height));
		CHECK(child->GetBox().GetSize().y == doctest::Approx(test_case.child_height));
		CHECK(child->GetRelativeOffset().y == doctest::Approx(0.f));
	}

	document->Close();

	TestsShell::ShutdownShell();
}

static const String document_flex_empty_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 500px;
		}
		.band {
			display: flex;
			position: relative;
			min-height: 40px;
			padding: 5px 0;
		}
		.column {
			flex-direction: column;
		}
		.border_box {
			box-sizing: border-box;
		}
		.art {
			position: absolute;
			top: 0;
			bottom: 0;
			left: 0;
			width: 10px;
		}
	</style>
</head>

<body>
	<div class="band" id="row"/>
	<div class="band column" id="column"/>
	<div class="band" id="row_absolute">
		<div class="art" id="row_absolute_art"/>
	</div>
	<div class="band column" id="column_absolute">
		<div class="art" id="column_absolute_art"/>
	</div>
	<div class="band border_box" id="border_box"/>
	<div id="after"/>
</body>
</rml>
)";

TEST_CASE("FlexFormatting.empty_container_min_size")
{
	// A flex container without in-flow flex items still takes its min-height when its height is auto.
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_flex_empty_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	struct TestCase {
		String container_id;
		float content_height;
	};
	const TestCase test_cases[] = {
		{"row", 40.f},
		{"column", 40.f},
		{"row_absolute", 40.f},
		{"column_absolute", 40.f},
		// With border-box sizing, the min-height includes the 2 * 5px padding.
		{"border_box", 30.f},
	};

	for (const TestCase& test_case : test_cases)
	{
		CAPTURE(test_case.container_id);

		Element* container = document->GetElementById(test_case.container_id);
		REQUIRE(container);

		CHECK(container->GetBox().GetSize() == Vector2f(500.f, test_case.content_height));
	}

	// Absolutely positioned children span the padding box of the container.
	for (const String child_id : {"row_absolute_art", "column_absolute_art"})
	{
		CAPTURE(child_id);

		Element* child = document->GetElementById(child_id);
		REQUIRE(child);

		CHECK(child->GetBox().GetSize().y == doctest::Approx(50.f));
	}

	// The containers take up their space in the normal flow: four bands of 50px and one of 40px.
	Element* after = document->GetElementById("after");
	REQUIRE(after);
	CHECK(after->GetAbsoluteTop() == doctest::Approx(240.f));

	document->Close();

	TestsShell::ShutdownShell();
}

#pragma once

#include <nlohmann/json.hpp>

#include <assert.h>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#pragma warning(push, 0)
namespace tiled
{
	struct TiledColor
	{
		uint8_t a;
		uint8_t r;
		uint8_t g;
		uint8_t b;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledColor& value)
		{
			destination = "#00000000";
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledColor& value)
		{
			auto colorString = source.get<std::string>();
			assert(colorString.size() == 7 or colorString.size() == 9);

			if (colorString.size() == 9)
			{
				auto alpha = colorString.substr(1, 2);
				value.a = (uint8_t)std::stoi(alpha, nullptr, 16);
				colorString = colorString.substr(3, 6);
			}
			else
			{
				colorString = colorString.substr(1, 6);
			}
			auto red = colorString.substr(0, 2);
			auto green = colorString.substr(2, 2);
			auto blue = colorString.substr(4, 2);
			value.r = (uint8_t)std::stoi(red, nullptr, 16);
			value.g = (uint8_t)std::stoi(green, nullptr, 16);
			value.b = (uint8_t)std::stoi(blue, nullptr, 16);
		}
	};

	using TiledLayerData = std::vector<unsigned int>;

	struct TiledText
	{
		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledText& value)
		{
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledText& value)
		{
		}
	};

	struct TiledPoint
	{
		double x;
		double y;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledPoint& value)
		{
			destination["x"] = std::get<std::string>(value.x);
			destination["y"] = std::get<std::string>(value.y);
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledPoint& value)
		{
			const auto defaultValue = TiledPoint{};
			value.x = source.value("x", defaultValue.x);
			value.y = source.value("y", defaultValue.y);
		}
	};

	enum class TiledPropertyType
	{
		string_,
		int_,
		float_,
		bool_,
		color_,
		file_,
		object_,
		class_
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledPropertyType,
								 { { TiledPropertyType::string_, "string" },
								   { TiledPropertyType::int_, "int" },
								   { TiledPropertyType::float_, "float" },
								   { TiledPropertyType::bool_, "bool" },
								   { TiledPropertyType::color_, "color" },
								   { TiledPropertyType::file_, "file" },
								   { TiledPropertyType::object_, "object" },
								   { TiledPropertyType::class_, "class" } })

	using TiledValue = std::variant<std::string, int, float, bool, TiledColor>;

	struct TiledProperty
	{
		std::string name;
		TiledPropertyType type{ TiledPropertyType::string_ };
		std::string propertytype;
		TiledValue value;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledProperty& value)
		{
			destination["name"] = value.name;
			destination["type"] = value.type;
			destination["propertytype"] = value.propertytype;

			switch (value.type)
			{
			case TiledPropertyType::string_:
				destination["value"] = std::get<std::string>(value.value);
				break;
			case TiledPropertyType::int_:
				destination["value"] = std::get<int>(value.value);
				break;
			case TiledPropertyType::float_:
				destination["value"] = std::get<float>(value.value);
				break;
			case TiledPropertyType::bool_:
				destination["value"] = std::get<bool>(value.value);
				break;
			case TiledPropertyType::color_:
				destination["value"] = std::get<TiledColor>(value.value);
				break;
			case TiledPropertyType::file_:
				destination["value"] = std::get<std::string>(value.value);
				break;
			case TiledPropertyType::object_:
				destination["value"] = std::get<std::string>(value.value);
				break;
			case TiledPropertyType::class_:
				destination["value"] = std::get<std::string>(value.value);
				break;
			}
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledProperty& value)
		{
			const auto defaultValue = TiledProperty{};
			value.name = source.value("name", defaultValue.name);
			value.type = source.value("type", defaultValue.type);
			value.propertytype = source.value("propertytype", defaultValue.propertytype);

			switch (value.type)
			{
			case TiledPropertyType::string_:
				value.value = source["value"].get<std::string>();
				break;
			case TiledPropertyType::int_:
				value.value = source["value"].get<int>();
				break;
			case TiledPropertyType::float_:
				value.value = source["value"].get<float>();
				break;
			case TiledPropertyType::bool_:
				value.value = source["value"].get<bool>();
				break;
			case TiledPropertyType::color_:
				value.value = source["value"].get<TiledColor>();
				break;
			case TiledPropertyType::file_:
				value.value = source["value"].get<std::string>();
				break;
			case TiledPropertyType::object_:
				value.value = source["value"].get<std::string>();
				break;
			case TiledPropertyType::class_:
				value.value = source["value"].get<std::string>();
				break;
			}
		}
	};

	struct TiledObject
	{
		bool ellipse;
		int gid;
		double height;
		int id;
		std::string name;
		bool point;
		std::vector<TiledPoint> polygon;
		std::vector<TiledPoint> polyline;
		std::vector<TiledProperty> properties;
		double rotation;
		std::string template_;
		TiledText text;
		std::string type{ "" };
		bool visible;
		double width;
		double x;
		double y;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledObject& value)
		{
			destination["ellipse"] = value.ellipse;
			destination["gid"] = value.gid;
			destination["height"] = value.height;
			destination["id"] = value.id;
			destination["name"] = value.name;
			destination["point"] = value.point;
			destination["polygon"] = value.polygon;
			destination["polyline"] = value.polyline;
			destination["properties"] = value.properties;
			destination["rotation"] = value.rotation;
			destination["template"] = value.template_;
			destination["text"] = value.text;
			destination["type"] = value.type;
			destination["visible"] = value.visible;
			destination["width"] = value.width;
			destination["x"] = value.x;
			destination["y"] = value.y;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledObject& value)
		{
			const auto defaultValue = TiledObject{};
			value.ellipse = source.value("ellipse", defaultValue.ellipse);
			value.gid = source.value("gid", defaultValue.gid);
			value.height = source.value("height", defaultValue.height);
			value.id = source.value("id", defaultValue.id);
			value.name = source.value("name", defaultValue.name);
			value.point = source.value("point", defaultValue.point);
			value.polygon = source.value("polygon", defaultValue.polygon);
			value.polyline = source.value("polyline", defaultValue.polyline);
			value.properties = source.value("properties", defaultValue.properties);
			value.rotation = source.value("rotation", defaultValue.rotation);
			value.template_ = source.value("template", defaultValue.template_);
			value.text = source.value("text", defaultValue.text);
			value.type = source.value("type", defaultValue.type);
			value.visible = source.value("visible", defaultValue.visible);
			value.width = source.value("width", defaultValue.width);
			value.x = source.value("x", defaultValue.x);
			value.y = source.value("y", defaultValue.y);
		}
	};

	struct TiledChunk
	{
		std::variant<std::string, TiledLayerData> data;
		int height;
		int width;
		int x;
		int y;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledChunk& value)
		{
			destination["data"] = std::get<std::string>(value.data);
			destination["height"] = std::get<std::string>(value.height);
			destination["width"] = std::get<std::string>(value.width);
			destination["x"] = std::get<std::string>(value.x);
			destination["y"] = std::get<std::string>(value.y);
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledChunk& value)
		{
			const auto defaultValue = TiledChunk{};
			value.data = source["data"].get<TiledLayerData>();
			value.height = source.value("height", defaultValue.height);
			value.width = source.value("width", defaultValue.width);
			value.x = source.value("x", defaultValue.x);
			value.y = source.value("y", defaultValue.y);
		}
	};

	enum class TiledCompression
	{
		default_compression,
		zlib,
		gzip,
		zstd
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledCompression,
								 { { TiledCompression::default_compression, "" },
								   { TiledCompression::zlib, "zlib" },
								   { TiledCompression::gzip, "gzip" },
								   { TiledCompression::zstd, "zstd" } })

	enum class TiledDrawOrder
	{
		topdown,
		index
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledDrawOrder,
								 { { TiledDrawOrder::topdown, "topdown" }, { TiledDrawOrder::index, "index" } })

	enum class TiledEncoding
	{
		csv,
		base64
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledEncoding, { { TiledEncoding::csv, "csv" }, { TiledEncoding::base64, "base64" } })

	enum class TiledLayerType
	{
		tilelayer,
		objectgroup,
		imagelayer,
		group
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledLayerType,
								 { { TiledLayerType::tilelayer, "tilelayer" },
								   { TiledLayerType::objectgroup, "objectgroup" },
								   { TiledLayerType::imagelayer, "imagelayer" },
								   { TiledLayerType::group, "group" } })


	struct TiledLayer
	{
		std::vector<TiledChunk> chunks;
		std::string class_;
		TiledCompression compression;
		std::variant<std::string, TiledLayerData> data;
		TiledDrawOrder draworder{ TiledDrawOrder::topdown };
		TiledEncoding encoding{ TiledEncoding::csv };
		int height;
		int id;
		std::string image;
		int imageheight;
		int imagewidth;
		std::vector<TiledLayer> layers;
		bool locked{ false };
		std::string name;
		std::vector<TiledObject> objects;
		double offsetx{ 0.0 };
		double offsety{ 0.0 };
		double opacity{};
		double parallaxx{ 1.0 };
		double parallaxy{ 1.0 };
		std::vector<TiledProperty> properties;
		bool repeatx;
		bool repeaty;
		int startx;
		int starty;
		TiledColor tintcolor;
		TiledLayerType type;
		bool visible;
		int width;
		int x{ 0 };
		int y{ 0 };

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledLayer& value)
		{
			destination["chunks"] = value.chunks;
			destination["class"] = value.class_;
			destination["compression"] = value.compression;
			if (value.encoding == TiledEncoding::base64)
			{
				destination["data"] = std::get<std::string>(value.data);
			}
			else
			{
				destination["data"] = std::get<TiledLayerData>(value.data);
			}

			destination["draworder"] = value.draworder;
			destination["encoding"] = value.encoding;
			destination["height"] = value.height;
			destination["id"] = value.id;
			destination["image"] = value.image;
			destination["imageheight"] = value.imageheight;
			destination["imagewidth"] = value.imagewidth;
			destination["layers"] = value.layers;
			destination["locked"] = value.locked;
			destination["name"] = value.name;
			destination["objects"] = value.objects;
			destination["offsetx"] = value.offsetx;
			destination["offsety"] = value.offsety;
			destination["opacity"] = value.opacity;
			destination["parallaxx"] = value.parallaxx;
			destination["parallaxy"] = value.parallaxy;
			destination["properties"] = value.properties;
			destination["repeatx"] = value.repeatx;
			destination["repeaty"] = value.repeaty;
			destination["startx"] = value.startx;
			destination["starty"] = value.starty;
			destination["tintcolor"] = value.tintcolor;
			destination["type"] = value.type;
			destination["visible"] = value.visible;
			destination["width"] = value.width;
			destination["x"] = value.x;
			destination["y"] = value.y;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledLayer& value)
		{
			const auto defaultValue = TiledLayer{};
			value.chunks = source.value("chunks", defaultValue.chunks);
			value.class_ = source.value("class", defaultValue.class_);
			value.compression = source.value("compression", defaultValue.compression);
			value.draworder = source.value("draworder", defaultValue.draworder);
			value.encoding = source.value("chunks", defaultValue.encoding);
			if (value.encoding == TiledEncoding::csv)
			{
				value.data = source.value("data", "");
			}
			else
			{
				value.data = source["data"].get<TiledLayerData>();
			}
			value.height = source.value("height", defaultValue.height);
			value.id = source.value("id", defaultValue.id);
			value.image = source.value("image", defaultValue.image);
			value.imageheight = source.value("imageheight", defaultValue.imageheight);
			value.imagewidth = source.value("imagewidth", defaultValue.imagewidth);
			value.layers = source.value("layers", defaultValue.layers);
			value.locked = source.value("locked", defaultValue.locked);
			value.name = source.value("name", defaultValue.name);
			value.objects = source.value("objects", defaultValue.objects);
			value.offsetx = source.value("offsetx", defaultValue.offsetx);
			value.offsety = source.value("offsety", defaultValue.offsety);
			value.opacity = source.value("opacity", defaultValue.opacity);
			value.parallaxx = source.value("parallaxx", defaultValue.parallaxx);
			value.parallaxy = source.value("parallaxy", defaultValue.parallaxy);
			value.properties = source.value("properties", defaultValue.properties);
			value.repeatx = source.value("repeatx", defaultValue.repeatx);
			value.repeaty = source.value("repeaty", defaultValue.repeaty);
			value.startx = source.value("startx", defaultValue.startx);
			value.starty = source.value("starty", defaultValue.starty);
			value.tintcolor = source.value("tintcolor", defaultValue.tintcolor);
			value.type = source.value("type", defaultValue.type);
			value.visible = source.value("visible", defaultValue.visible);
			value.width = source.value("width", defaultValue.width);
			value.x = source.value("x", defaultValue.x);
			value.y = source.value("y", defaultValue.y);
		}
	};


	enum class TiledTilesetFillMode
	{
		stretch,
		preserve_aspect_fit
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledTilesetFillMode,
								 { { TiledTilesetFillMode::stretch, "stretch" },
								   { TiledTilesetFillMode::preserve_aspect_fit, "preserve-aspect-fit" } })

	enum class TiledGridOrientation
	{
		orthogonal,
		isometric
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledGridOrientation,
								 { { TiledGridOrientation::orthogonal, "orthogonal" },
								   { TiledGridOrientation::isometric, "isometric" } })

	struct TiledGrid
	{
		int width;
		int height;
		TiledGridOrientation orientation{ TiledGridOrientation::orthogonal };

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledGrid& value)
		{
			destination["width"] = value.width;
			destination["orientation"] = value.orientation;
			destination["height"] = value.height;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledGrid& value)
		{
			const auto defaultValue = TiledGrid{};
			value.height = source.value("height", defaultValue.height);
			value.orientation = source.value("orientation", defaultValue.orientation);
			value.width = source.value("width", defaultValue.width);
		}
	};

	enum class TiledObjectAlignment
	{
		unspecified,
		topleft,
		top,
		topright,
		left,
		center,
		right,
		bottomleft,
		bottom,
		bottomright
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledObjectAlignment,
								 { { TiledObjectAlignment::unspecified, "unspecified" },
								   { TiledObjectAlignment::topleft, "topleft" },
								   { TiledObjectAlignment::top, "top" },
								   { TiledObjectAlignment::topright, "topright" },
								   { TiledObjectAlignment::left, "left" },
								   { TiledObjectAlignment::center, "center" },
								   { TiledObjectAlignment::right, "right" },
								   { TiledObjectAlignment::bottomleft, "bottomleft" },
								   { TiledObjectAlignment::bottom, "bottom" },
								   { TiledObjectAlignment::bottomright, "bottomright" } })

	struct TiledTerrain
	{
		std::string name;
		std::vector<TiledProperty> properties;
		int tile;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledTerrain& value)
		{
			destination["name"] = value.name;
			destination["properties"] = value.properties;
			destination["tile"] = value.tile;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledTerrain& value)
		{
			const auto defaultValue = TiledTerrain{};
			value.name = source.value("name", defaultValue.name);
			value.properties = source.value("properties", defaultValue.properties);
			value.tile = source.value("tile", defaultValue.tile);
		}
	};

	struct TiledTileOffset
	{
		int x;
		int y;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledTileOffset& value)
		{
			destination["x"] = value.x;
			destination["y"] = value.y;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledTileOffset& value)
		{

			const auto defaultValue = TiledTileOffset{};
			value.x = source.value("x", defaultValue.x);
			value.y = source.value("y", defaultValue.y);
		}
	};

	enum class TiledTileRenderSize
	{
		tile,
		grid
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledTileRenderSize,
								 { { TiledTileRenderSize::tile, "tile" }, { TiledTileRenderSize::grid, "grid" } })

	struct TiledTileFrame
	{
		int duration;
		int tileid;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledTileFrame& value)
		{
			destination["duration"] = value.duration;
			destination["tileid"] = value.tileid;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledTileFrame& value)
		{
			const auto defaultValue = TiledTileFrame{};
			value.duration = source.value("duration", defaultValue.duration);
			value.tileid = source.value("tileid", defaultValue.tileid);
		}
	};

	struct TiledWangSet
	{
		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledWangSet& value)
		{
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledWangSet& value)
		{
		}
	};

	struct TiledTile
	{
		std::vector<TiledTileFrame> animation;
		int id;
		std::string image;
		int imageheight;
		int imagewidth;
		int x;
		int y;
		int width;
		int height;
		TiledLayer objectgroup;
		double probability;
		std::vector<TiledProperty> properties;
		std::vector<int> terrain;
		std::string type;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledTile& value)
		{
			destination["animation"] = value.animation;
			destination["id"] = value.id;
			destination["image"] = value.image;
			destination["imageheight"] = value.imageheight;
			destination["imagewidth"] = value.imagewidth;
			destination["x"] = value.x;
			destination["y"] = value.y;
			destination["width"] = value.width;
			destination["height"] = value.height;
			destination["objectgroup"] = value.objectgroup;
			destination["probability"] = value.probability;
			destination["properties"] = value.properties;
			destination["terrain"] = value.terrain;
			destination["type"] = value.type;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledTile& value)
		{
			const auto defaultValue = TiledTile{};
			value.animation = source.value("animation", defaultValue.animation);
			value.id = source.value("id", defaultValue.id);
			value.image = source.value("image", defaultValue.image);
			value.imageheight = source.value("imageheight", defaultValue.imageheight);
			value.imagewidth = source.value("imagewidth", defaultValue.imagewidth);
			value.x = source.value("x", defaultValue.x);
			value.y = source.value("y", defaultValue.y);
			value.width = source.value("width", defaultValue.width);
			value.height = source.value("height", defaultValue.height);
			value.objectgroup = source.value("objectgroup", defaultValue.objectgroup);
			value.probability = source.value("probability", defaultValue.probability);
			value.terrain = source.value("terrain", defaultValue.terrain);
			value.type = source.value("type", defaultValue.type);
		}
	};

	struct TiledTileTransformations
	{
		bool hflip;
		bool vflip;
		bool rotate;
		bool preferuntransformed;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledTileTransformations& value)
		{
			destination["hflip"] = value.hflip;
			destination["vflip"] = value.vflip;
			destination["rotate"] = value.rotate;
			destination["preferuntransformed"] = value.preferuntransformed;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledTileTransformations& value)
		{
			const auto defaultValue = TiledTileTransformations{};
			value.hflip = source.value("hflip", defaultValue.hflip);
			value.vflip = source.value("vflip", defaultValue.vflip);
			value.rotate = source.value("rotate", defaultValue.rotate);
			value.preferuntransformed = source.value("preferuntransformed", defaultValue.preferuntransformed);
		}
	};

	enum class TiledTilesetType
	{
		tileset
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledTilesetType, { { TiledTilesetType::tileset, "tileset" } })


	struct TiledTileset
	{
		TiledColor backgroundcolor;
		std::string class_;
		int columns;
		TiledTilesetFillMode fillmode{ TiledTilesetFillMode::stretch };
		int firstgid;
		TiledGrid grid;
		std::string image;
		int imageheight;
		int imagewidth;
		int margin;
		std::string name;
		TiledObjectAlignment objectalignment{ TiledObjectAlignment::unspecified };
		std::vector<TiledProperty> properties;
		std::string source;
		int spacing;
		std::vector<TiledTerrain> terrains;
		int tilecount;
		std::string tiledversion;
		int tileheight;
		TiledTileOffset tileoffset;
		TiledTileRenderSize tilerendersize{ TiledTileRenderSize::tile };
		std::vector<TiledTile> tiles;
		int tilewidth;
		TiledTileTransformations transformations;
		TiledColor transparentcolor;
		TiledTilesetType type{ TiledTilesetType::tileset };
		std::string version;
		std::vector<TiledWangSet> wangsets;

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledTileset& value)
		{
			destination["backgroundcolor"] = value.backgroundcolor;
			destination["class"] = value.class_;
			destination["columns"] = value.columns;
			destination["fillmode"] = value.fillmode;
			destination["firstgid"] = value.firstgid;
			destination["grid"] = value.grid;
			destination["image"] = value.image;
			destination["imageheight"] = value.imageheight;
			destination["imagewidth"] = value.imagewidth;
			destination["margin"] = value.margin;
			destination["name"] = value.name;
			destination["objectalignment"] = value.objectalignment;
			destination["properties"] = value.properties;
			destination["source"] = value.source;
			destination["spacing"] = value.spacing;
			destination["terrains"] = value.terrains;
			destination["tilecount"] = value.tilecount;
			destination["tiledversion"] = value.tiledversion;
			destination["tileheight"] = value.tileheight;
			destination["tileoffset"] = value.tileoffset;
			destination["tilerendersize"] = value.tilerendersize;
			destination["tiles"] = value.tiles;
			destination["tilewidth"] = value.tilewidth;
			destination["transformations"] = value.transformations;
			destination["transparentcolor"] = value.transparentcolor;
			destination["type"] = value.type;
			destination["version"] = value.version;
			destination["wangsets"] = value.wangsets;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledTileset& value)
		{
			const auto defaultValue = TiledTileset{};
			value.backgroundcolor = source.value("backgroundcolor", defaultValue.backgroundcolor);
			value.class_ = source.value("class", defaultValue.class_);
			value.columns = source.value("columns", defaultValue.columns);
			value.fillmode = source.value("fillmode", defaultValue.fillmode);
			value.firstgid = source.value("firstgid", defaultValue.firstgid);
			value.grid = source.value("grid", defaultValue.grid);
			value.image = source.value("image", defaultValue.image);
			value.imageheight = source.value("imageheight", defaultValue.imageheight);
			value.imagewidth = source.value("imagewidth", defaultValue.imagewidth);
			value.margin = source.value("margin", defaultValue.margin);
			value.name = source.value("name", defaultValue.name);
			value.objectalignment = source.value("objectalignment", defaultValue.objectalignment);
			value.properties = source.value("properties", defaultValue.properties);
			value.source = source.value("source", defaultValue.source);
			value.spacing = source.value("spacing", defaultValue.spacing);
			value.terrains = source.value("terrains", defaultValue.terrains);
			value.tilecount = source.value("tilecount", defaultValue.tilecount);
			value.tiledversion = source.value("tiledversion", defaultValue.tiledversion);
			value.tileheight = source.value("tileheight", defaultValue.tileheight);
			value.tileoffset = source.value("tileoffset", defaultValue.tileoffset);
			value.tilerendersize = source.value("tilerendersize", defaultValue.tilerendersize);
			value.tiles = source.value("tiles", defaultValue.tiles);
			value.tilewidth = source.value("tilewidth", defaultValue.tilewidth);
			value.transformations = source.value("transformations", defaultValue.transformations);
			value.type = source.value("type", defaultValue.type);
			value.version = source.value("version", defaultValue.version);
			value.wangsets = source.value("wangsets", defaultValue.wangsets);
		}
	};


	enum class TiledOrientation
	{
		orthogonal,
		isometric,
		staggered,
		hexagonal
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledOrientation,
								 { { TiledOrientation::orthogonal, "orthogonal" },
								   { TiledOrientation::isometric, "isometric" },
								   { TiledOrientation::staggered, "staggered" },
								   { TiledOrientation::hexagonal, "hexagonal" } })
	enum class TiledRenderOrder
	{
		right_down,
		right_up,
		left_down,
		left_up
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledRenderOrder,
								 { { TiledRenderOrder::right_down, "right-down" },
								   { TiledRenderOrder::right_up, "right-up" },
								   { TiledRenderOrder::left_down, "left-down" },
								   { TiledRenderOrder::left_up, "left-up" } })

	enum class TiledStaggerAxis
	{
		x,
		y
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledStaggerAxis, { { TiledStaggerAxis::x, "x" }, { TiledStaggerAxis::y, "y" } })

	enum class TiledStaggerIndex
	{
		odd,
		even
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledStaggerIndex,
								 { { TiledStaggerIndex::odd, "odd" }, { TiledStaggerIndex::even, "even" } })

	enum class TiledType
	{
		map
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(TiledType, { { TiledType::map, "map" } })


	struct TiledMap
	{
		TiledColor backgroundcolor{};
		std::string class_{ "" };
		int compressionlevel{ -1 };
		int height{};
		int hexsidelength{};
		bool infinite{ false };
		std::vector<TiledLayer> layers{};
		int nextlayerid{ 0 };
		int nextobjectid{ 0 };
		TiledOrientation orientation{ TiledOrientation::orthogonal };
		double parallaxoriginx{ 0.0 };
		double parallaxoriginy{ 0.0 };
		std::vector<TiledProperty> properties;
		TiledRenderOrder renderorder{ TiledRenderOrder::right_down };
		TiledStaggerAxis staggeraxis{ TiledStaggerAxis::x };
		TiledStaggerIndex staggerindex{ TiledStaggerIndex::odd };
		std::string tiledversion{};
		int tileheight{};
		std::vector<TiledTileset> tilesets;
		int tilewidth{};
		TiledType type{ TiledType::map };
		std::string version{ "1.6" };
		int width{};

		template <typename BasicJsonType>
		friend void to_json(BasicJsonType& destination, const TiledMap& value)
		{
			destination["backgroundcolor"] = value.backgroundcolor;
			destination["class"] = value.class_;
			destination["height"] = value.height;
			destination["hexsidelength"] = value.hexsidelength;
			destination["infinite"] = value.infinite;
			destination["layers"] = value.layers;
			destination["nextlayerid"] = value.nextlayerid;
			destination["nextobjectid"] = value.nextobjectid;
			destination["orientation"] = value.orientation;
			destination["parallaxoriginx"] = value.parallaxoriginx;
			destination["parallaxoriginy"] = value.parallaxoriginy;
			destination["properties"] = value.properties;
			destination["staggeraxis"] = value.staggeraxis;
			destination["staggerindex"] = value.staggerindex;
			destination["tiledversion"] = value.tiledversion;
			destination["tilesets"] = value.tilesets;
			destination["tilewidth"] = value.tilewidth;
			destination["type"] = value.type;
			destination["version"] = value.version;
			destination["width"] = value.width;
		}

		template <typename BasicJsonType>
		friend void from_json(const BasicJsonType& source, TiledMap& value)
		{
			const auto defaultValue = TiledMap{};
			value.backgroundcolor = source.value("backgroundcolor", defaultValue.backgroundcolor);
			value.class_ = source.value("class", defaultValue.class_);
			value.height = source.value("height", defaultValue.height);
			value.hexsidelength = source.value("hexsidelength", defaultValue.hexsidelength);
			value.infinite = source.value("infinite", defaultValue.infinite);
			value.layers = source.value("layers", defaultValue.layers);
			value.nextlayerid = source.value("nextlayerid", defaultValue.nextlayerid);
			value.nextobjectid = source.value("nextobjectid", defaultValue.nextobjectid);
			value.orientation = source.value("orientation", defaultValue.orientation);
			value.parallaxoriginx = source.value("parallaxoriginx", defaultValue.parallaxoriginx);
			value.parallaxoriginy = source.value("parallaxoriginy", defaultValue.parallaxoriginy);
			value.properties = source.value("properties", defaultValue.properties);
			value.staggeraxis = source.value("staggeraxis", defaultValue.staggeraxis);
			value.staggerindex = source.value("staggerindex", defaultValue.staggerindex);
			value.tiledversion = source.value("tiledversion", defaultValue.tiledversion);
			value.tilesets = source.value("tilesets", defaultValue.tilesets);
			value.tilewidth = source.value("tilewidth", defaultValue.tilewidth);
			value.type = source.value("type", defaultValue.type);
			value.version = source.value("version", defaultValue.version);
			value.width = source.value("width", defaultValue.width);
		}
	};
} // namespace tiled
#pragma warning(pop)

tiled::TiledMap ImportMap(const std::filesystem::path file);
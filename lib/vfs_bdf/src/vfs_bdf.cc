/*
 * \brief  BDF font file system
 * \author Norman Feske
 * \date   2024-01-24
 */

/*
 * Copyright (C) 2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <vfs/dir_file_system.h>
#include <vfs/readonly_value_file_system.h>
#include <os/vfs.h>

/* gems includes */
#include <bdf_font.h>
#include <gems/cached_font.h>

/* local includes */
#include <glyphs_file_system.h>

namespace Vfs_bdf {

	using namespace Vfs;
	using namespace Genode;

	class Font_from_file;
	class Local_factory;
	class File_system;

	typedef Text_painter::Font Font;
}


struct Vfs_bdf::Font_from_file
{
	typedef Directory::Path Path;

	Directory    const _dir;
	File_content const _content;

	Constructible<Bdf_font const> _font { };

	Font_from_file(Vfs::Env &vfs_env, Path const &file_path, Bdf_font::Attr const &attr)
	:
		_dir(vfs_env),
		_content(vfs_env.alloc(), _dir, file_path, File_content::Limit{10*1024*1024})
	{
		_content.bytes([&] (char const *ptr, size_t size) {
			_font.construct(vfs_env.alloc(), Const_byte_range_ptr { ptr, size }, attr); });
	}

	Font const &font() const { return *_font; }
};


struct Vfs_bdf::Local_factory : File_system_factory, Watch_response_handler
{
	Vfs::Env &_env;

	struct Font_config
	{
		Directory::Path    path;
		Bdf_font::Attr     attr;
		Cached_font::Limit cache_limit;

		Font_config(Xml_node const &config)
		:
			path(config.attribute_value("path", Directory::Path())),
			attr(Bdf_font::Attr::from_xml(config)),
			cache_limit({config.attribute_value("cache", Number_of_bytes())})
		{ }
	} _font_config;

	struct Font
	{
		Font_from_file     font;
		Cached_font::Limit cache_limit;
		Cached_font        cached_font;

		Font(Vfs::Env &env, Font_config &config)
		:
			font(env, config.path, config.attr),
			cache_limit(config.cache_limit),
			cached_font(env.alloc(), font.font(), cache_limit)
		{ }
	};

	Reconstructible<Font> _font;

	Glyphs_file_system _glyphs_fs { _font->cached_font };

	Readonly_value_file_system<unsigned> _baseline_fs   { "baseline",   0 };
	Readonly_value_file_system<unsigned> _height_fs     { "height",     0 };
	Readonly_value_file_system<unsigned> _max_width_fs  { "max_width",  0 };
	Readonly_value_file_system<unsigned> _max_height_fs { "max_height", 0 };

	Watcher _watcher;

	void _update_attributes()
	{
		_baseline_fs  .value(_font->font.font().baseline());
		_height_fs    .value(_font->font.font().height());
		_max_width_fs .value(_font->font.font().bounding_box().w);
		_max_height_fs.value(_font->font.font().bounding_box().h);
	}

	Local_factory(Vfs::Env &env, Xml_node const &config)
	:
		_env(env),
		_font_config(config),
		_font(env, _font_config),
		_watcher(env, _font_config.path, *this)
	{
		_update_attributes();
	}

	Vfs::File_system *create(Vfs::Env&, Xml_node const &node) override
	{
		if (node.has_type(Glyphs_file_system::type_name()))
			return &_glyphs_fs;

		if (node.has_type(Readonly_value_file_system<unsigned>::type_name()))
			return _baseline_fs.matches(node)   ? &_baseline_fs
			     : _height_fs.matches(node)     ? &_height_fs
			     : _max_width_fs.matches(node)  ? &_max_width_fs
			     : _max_height_fs.matches(node) ? &_max_height_fs
			     : nullptr;

		return nullptr;
	}

	void apply_config(Xml_node const &config)
	{
		_font_config = Font_config(config);
		_font.construct(_env, _font_config);
		_update_attributes();
		_glyphs_fs.trigger_watch_response();
	}

	void watch_response() override
	{
		_font.construct(_env, _font_config);
		_update_attributes();
		_glyphs_fs.trigger_watch_response();
	}
};


class Vfs_bdf::File_system : private Local_factory,
                             public  Vfs::Dir_file_system
{
	private:

		typedef String<200> Config;
		static Config _config(Xml_node const &node)
		{
			char buf[Config::capacity()] { };

			Xml_generator xml(buf, sizeof(buf), "dir", [&] () {
				typedef String<64> Name;
				xml.attribute("name", node.attribute_value("name", Name()));
				xml.node("glyphs", [&] () { });
				xml.node("readonly_value", [&] () { xml.attribute("name", "baseline");   });
				xml.node("readonly_value", [&] () { xml.attribute("name", "height");     });
				xml.node("readonly_value", [&] () { xml.attribute("name", "max_width");  });
				xml.node("readonly_value", [&] () { xml.attribute("name", "max_height"); });
			});
			return Config(Cstring(buf));
		}

	public:

		File_system(Vfs::Env &vfs_env, Genode::Xml_node const &node)
		:
			Local_factory(vfs_env, node),
			Vfs::Dir_file_system(vfs_env,
			                     Xml_node(_config(node).string()),
			                     *this)
		{ }

		char const *type() override { return "bdf"; }

		void apply_config(Xml_node const &node) override
		{
			Local_factory::apply_config(node);
		}
};


/**************************
 ** VFS plugin interface **
 **************************/

extern "C" Vfs::File_system_factory *vfs_file_system_factory(void)
{
	struct Factory : Vfs::File_system_factory
	{
		Vfs::File_system *create(Vfs::Env &vfs_env,
		                         Genode::Xml_node const &node) override
		{
			try { return new (vfs_env.alloc())
				Vfs_bdf::File_system(vfs_env, node); }
			catch (...) { }
			return nullptr;
		}
	};

	static Factory factory;
	return &factory;
}

#include <SDL2/SDL.h>
#include <iostream>


struct Area { unsigned w, h; };


struct Time { unsigned value; };


template <typename T>
static T &_deref(auto const &error_msg, T *ptr)
{
	if (ptr)
		return *ptr;

	std::cerr << error_msg << ", SDL_Error: " << SDL_GetError() << "\n";
	SDL_Quit();
	std::abort();
}


struct Pixel
{
	uint32_t value;

	static Pixel rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		return { uint32_t((a << 24) | (r << 16) | (g << 8) | b) };
	}

	static Pixel rgb(uint8_t r, uint8_t g, uint8_t b) { return rgba(r, g, b, 255); }

	void mix(Pixel p)
	{
		uint32_t const MASK = 0xfefefefe;
		value = ((value & MASK) >> 1) + ((p.value & MASK) >> 1);
	}

	Pixel dim25() const
	{
		return { ((value & 0xfefefefe) >> 1)
		       + ((value & 0xfcfcfcfc) >> 2) };
	}
};


struct Render_buffer
{
	SDL_Renderer *_renderer_ptr;

	Area _size;

	SDL_Texture *_texture_ptr = &_deref("SDL_CreateTexture",
	                                    SDL_CreateTexture(_renderer_ptr, SDL_PIXELFORMAT_ARGB8888,
	                                                      SDL_TEXTUREACCESS_STREAMING, _size.w, _size.h));

	size_t  _pixels_num_bytes = sizeof(Pixel)*_size.w*_size.h;
	Pixel * _pixels_ptr       = (Pixel *)malloc(_pixels_num_bytes);

	Render_buffer(SDL_Renderer &renderer, Area const size) : _renderer_ptr(&renderer), _size(size) { }

	void resize(Area size)
	{
		SDL_DestroyTexture(_texture_ptr);
		free(_pixels_ptr);
		*this = Render_buffer { *_renderer_ptr, size };
	}

	void with_pixels(auto const &fn)
	{
		fn(_pixels_ptr, _size);

		SDL_UpdateTexture(_texture_ptr, nullptr, _pixels_ptr, sizeof(Pixel)*_size.w);
		SDL_RenderClear(_renderer_ptr);
		SDL_RenderCopy(_renderer_ptr, _texture_ptr, nullptr, nullptr);
		SDL_RenderPresent(_renderer_ptr);
	}
};


struct Window
{
	Area const _initial_size;

	SDL_Window  &_window = _deref("SDL_CreateWindow",
	                              SDL_CreateWindow("SDL", SDL_WINDOWPOS_UNDEFINED,
	                                                      SDL_WINDOWPOS_UNDEFINED,
	                                                      _initial_size.w, _initial_size.h,
	                                                      SDL_WINDOW_SHOWN));
	SDL_Renderer &_renderer = _deref("SDL_CreateRenderer",
	                                 SDL_CreateRenderer(&_window, -1, SDL_RENDERER_SOFTWARE
	                                                                | SDL_RENDERER_TARGETTEXTURE));
	Render_buffer buffer { _renderer, _initial_size };

	SDL_Surface &screen = *SDL_GetWindowSurface(&_window);

	Window(Area initial_size) : _initial_size(initial_size)
	{
		SDL_SetRenderDrawBlendMode(&_renderer, SDL_BLENDMODE_NONE);
		SDL_FillRect(&screen, NULL, SDL_MapRGB( screen.format, 0xFF, 0xFF, 0xFF ) );
	}

	~Window() { SDL_DestroyWindow(&_window); }

	void resize(Area size)
	{
		buffer.resize(size);
		SDL_UpdateWindowSurface(&_window);
	}

	void with_pixels(auto const &fn)
	{
		buffer.with_pixels(fn);
		SDL_UpdateWindowSurface(&_window);
	}
};


struct Main
{
	bool running = true;

	Window window { { 1024, 1024 } };

	void _render(Time const time, Pixel *dst, unsigned w, unsigned h)
	{
		unsigned const N = 200;
		float    const R = M_PI*2/235;
		float    const SCALE = 220;

		float u = 0, v = 0;
		float x = 0;
		float t = time.value*0.0015;

		for (unsigned i = 0; i < N; i++) {
			for (unsigned j = 0; j < N; j++) {
				u = sin(i + v) + sin(R*i + x);
				v = cos(i + v) + cos(R*i + x);
				x = u + t;
				unsigned const px = unsigned(w/2 + u*SCALE);
				unsigned const py = unsigned(h/2 + v*SCALE);
				if (px < w && py < h)
					dst[py*w + px].mix(Pixel::rgb(i, j, 99));
			}
		}
	}

	void _dim(Pixel *dst, unsigned const count)
	{
		for (unsigned i = 0; i < count; i++, dst++)
			*dst = dst->dim25();
	}

	void render(Time time)
	{
		window.with_pixels([&] (Pixel *dst, Area size) {
			_dim(dst, size.w*size.h);
			_render(time, dst, size.w, size.h); });
	}

	void handle(SDL_Event const &event)
	{
		if (event.type == SDL_QUIT)
			running = false;
	}
};


int main()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		std::cerr << "SDL_Init failed, SDL_Error: " << SDL_GetError() << "\n";
		SDL_Quit();
	}

	static Main main { };

	Time time { };

	while (main.running) {
		main.render(time);

		SDL_Event event { };
		while (SDL_PollEvent(&event))
			main.handle(event);

		SDL_Delay(10);

		time.value++;
	}
	SDL_Quit();
	return 0;
}

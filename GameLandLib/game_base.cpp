/**
 * Copyright 2012 Willem Duminy
 * See LICENCE.txt
 */
#include "game.h"
#include "log.h"
#include <windows.h>

namespace game {
using systemex::string_from_file;
SDL_Window * initSDL(const bool fullscreen, const int w, const int h,
		bool use_opengl) {
	check(SDL_Init(SDL_INIT_VIDEO));
	atexit(SDL_Quit);
	check(TTF_Init());
	atexit(TTF_Quit);

	SDL_version version;
	SDL_version compiled;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&version);
	ENSURE(compiled.major == version.major 
		&& compiled.minor == version.minor
		&& compiled.patch == version.patch
		, "Compiled and linked version mismatch");
	LOG<< "Linking against SDL version " << (int) version.major << "." << (int) version.minor << "." << (int) version.patch;

	int flags =0;
	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;
	if (use_opengl) {
		flags |= SDL_WINDOW_OPENGL;
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	}
	auto window = SDL_CreateWindow("GameEx Game",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,w,h,flags);
	check(window);
	return window;
}

DrawContext::DrawContext(const bool fullscreen, const int w,	const int h, const bool opengl) :
		_window(0), _glex(opengl ? new Glex() : 0), _width(w), _height(h), _renderer() {
	auto window = initSDL(fullscreen, w, h, opengl);	
	_renderer.reset(new Renderer(window));
	_renderer->set_logical_size(w,h);
	if (opengl) 
		SDL_GL_CreateContext(window);
}

Glex& DrawContext::gl() const {
	if (!_glex)
		throw std::runtime_error("draw context does not have opengl");
	return *_glex;
}

void DrawContext::swap() {
	if (has_opengl())
	 	SDL_GL_SwapWindow(_window);
	else 
		_renderer->present();
}

DrawContext::~DrawContext() {
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

GameContext::GameContext(const ResourceContext &pr,const UpdateContext &pu, const DrawContext &pd)
	: r(pr), d(pd), u(pu) {}

void GameObjectWithParts::collect(std::deque<GameObject *> &c) {
	c.push_back(this);
	for_each(it,_parts)
		(*it)->collect(c);
}

void GameObjectWithParts::update(const GameContext &context) {
	for_each(it,_parts)
	{
		if ((*it)->is_active())
			(*it)->update(context);
	}
}

bool InputEvent::is_quit() const {
	return _event.type == SDL_QUIT;
}

SDL_Keycode InputEvent::key_down() const {
	if (_event.type != SDL_KEYDOWN)
		return SDLK_UNKNOWN;
	else
		return _event.key.keysym.sym;
}

SDL_Keycode InputEvent::key_up() const {
	if (_event.type != SDL_KEYUP)
		return SDLK_UNKNOWN;
	else
		return _event.key.keysym.sym;
}

bool InputEvent::is_ctl_down() const {
	if (_event.type != SDL_KEYDOWN)
		return false;
	return _event.key.keysym.mod & KMOD_CTRL;
}

void InputEvent::poll() {
	SDL_PollEvent(&_event);
}

/**
 *
 * @param updatesPerSec
 * @param framesPerSec
 */
UpdateContext::UpdateContext(unsigned int updatesPerSec,
		unsigned int framesPerSec) :
		_event(), _update_interval(1000 / updatesPerSec), _draw_interval(
				1000 / framesPerSec), _first_tick(SDL_GetTicks()), next_update(
				_first_tick + _update_interval), next_draw(
				_first_tick + _draw_interval), update(false), draw(false), _tick_time(
				_first_tick), _draws(0), _updates(0) {
}

void UpdateContext::tick() {
	_tick_time = SDL_GetTicks();
	update = (_tick_time >= next_update);
	draw = (_tick_time >= next_draw);
	if (update) {
		//this could be a problem ... events on SDL is queued and will be
		//processed one at a time of the update loop. so, if the input seems
		//jittery, it could be that the SDL events must be processed in a different
		//way -- maybe an 'input context' would be a good idea
		_event.poll();
		next_update = _tick_time + _update_interval;
		_updates++;
	}

	if (draw) {
		next_draw = _tick_time + _draw_interval;
		_draws++;
	} 
}

void UpdateContext::log_statistics() const {
	auto elapsed = _tick_time - _first_tick;
	LOG<< "Run time (seconds): " << elapsed / 1000.0
	<< "\n\t FPS: " << _draws * 1000.0 / elapsed
	<< "\n\t UPS: " << _updates * 1000.0 / elapsed;
}

Game::Game(MainObject::u_ptr primaryPart, UpdateContext::u_ptr update,
		DrawContext::u_ptr draw, ResourceContext::u_ptr resource) :
		_primary(std::move(primaryPart)), _update(std::move(update)), _draw(
				std::move(draw)), _resource(std::move(resource)),
		context_(*_resource, *_update, *_draw) {
}

Game::Game(MainObject * primaryPart,
		const std::string& resource_path,
		int ups, int fps,
		bool fullscreen, int width, int height,
		bool opengl) : Game(
				MainObject::u_ptr(primaryPart),
				UpdateContext::u_ptr(new UpdateContext(ups,fps)),
				DrawContext::u_ptr(new DrawContext(fullscreen,width,height,opengl)),
				ResourceContext::u_ptr(new ResourceContext(resource_path))
			) {}

int Game::run() {
	try {
		LOG << "running";
		_primary->initialise(*_resource, *_draw);
		LOG << "initialise done";
		std::deque<GameObject *> objects;
		_primary->collect(objects);
		// to fix draw order, sort the object
		std::sort(objects.begin(), objects.end(),
				[](const GameObject * a, const GameObject * b) {
					return a->draw_order() < b->draw_order();});
		LOG << "starting game loop";
		_update->tick();
		while (_primary->is_active()) {
			if (_update->is_update_step()) {
				_primary->update(context_);
			}
			if (_update->is_draw_step()) {
				for (auto it = objects.begin(); it != objects.end(); it++) {
					if ((*it)->is_visible())
						(*it)->draw(*_draw);
				}
				_draw->swap();
			}
			_update->tick();
		};
		_update->log_statistics();
		return EXIT_SUCCESS;
	} catch (std::exception &e) {
		LOG << "exception: " << (e.what()) << std::endl;
	    return EXIT_FAILURE;
		} catch (...) {
		LOG << "unexpected error occurred";
	    return EXIT_FAILURE;
	}
}

void MainObject::initialise(const ResourceContext & rc, const DrawContext& dc) {
	if (dc.has_opengl()) {
		/* Our shading model--Gouraud (smooth). */
		glShadeModel(GL_SMOOTH);
		/* Culling. */
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0, 0.0, 0.0, 0);
		glViewport(0, 0, dc.width(), dc.height());
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		float ratio = (float) dc.width() / (float) dc.height();
		glLoadIdentity(); // load identity because we want to 'reset' the perspective
		gluPerspective(45.0, ratio, _nearest, _farest);
		/* blending */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	GameObjectWithParts::initialise(rc, dc);
}

void MainObject::update(const GameContext& ctx) {
	GameObjectWithParts::update(ctx);
	auto i = ctx.u.input();
	if (i.is_quit() || (i.key_down() == SDLK_c && i.is_ctl_down()))
		deactivate();
	if (ctx.u.input().key_down() == SDL_SCANCODE_PRINTSCREEN)
		_print_screen = true;
}

void MainObject::draw(const DrawContext& dc) {
	if (dc.has_opengl()) {
		/* Clear the colour and depth buffers. */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* We don't want to modify the projection matrix. */
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	} else
		dc.render().clear();
	if (_print_screen) {
		dc.screen_to_bmp("screen.bmp");
	}
}

const std::string RESOURCE_PATH("media/");

ResourceContext::ResourceContext(const string& subDirectory) :
		_root_directory(RESOURCE_PATH + subDirectory) {
	if (_root_directory.back() != '/')
		throw systemex::runtime_error_ex("subDirectory (%s) must end with '/'",
				subDirectory.c_str());
}

/**
 * @brief Loads the text from the resource folder
 * @param filename
 * @return
 */
string ResourceContext::load_text(const char * filename) const {
	auto fullname = _root_directory + filename;
	return string_from_file(fullname.c_str());
}

ShaderProgram * ResourceContext::load_program(Glex * gl,
		const char * filePrefix) const {
	ShaderProgram * result(new ShaderProgram());
	auto full_prefix = _root_directory + filePrefix;
	result->initialise(gl,string_from_file((full_prefix + ".vert").c_str()),
			string_from_file((full_prefix + ".frag").c_str()));
	return result;
}

Texture * ResourceContext::load_texture_bmp(Glex*  gl, const char* filename,
		const int textureIndex) const {
	Texture * result(new Texture(gl));
	Surface::u_ptr sface(new Surface(_root_directory + filename));
	result->bind(textureIndex);
	result->copy_from(*sface);
	return result;
}

void GameObjectWithParts::set_hidden(const bool value) {
	GameObject::set_hidden(value);
	for_each(e,_parts)
	{
		(*e)->set_hidden(value);
	}
}

void GameObjectWithParts::set_active(const bool value) {
	GameObject::set_active(value);
	for_each(e,_parts)
	{
		(*e)->set_active(value);
	}
}

SDL_Surface* flip_vertical(SDL_Surface* sfc) {
     SDL_Surface* result = SDL_CreateRGBSurface(sfc->flags, sfc->w, sfc->h,
         sfc->format->BytesPerPixel * 8, sfc->format->Rmask, sfc->format->Gmask,
         sfc->format->Bmask, sfc->format->Amask);
     const auto pitch = sfc->pitch;
     const auto pxlength = pitch*sfc->h;
     auto pixels = static_cast<unsigned char*>(sfc->pixels) + pxlength;
     auto rpixels = static_cast<unsigned char*>(result->pixels) ;
     for(auto line = 0; line < sfc->h; ++line) {
         memcpy(rpixels,pixels,pitch);
         pixels -= pitch;
         rpixels += pitch;
     }
     return result;
}

void DrawContext::screen_to_bmp(const std::string& filename) const {
	if (!has_opengl())
		throw std::runtime_error("screen shot works only in open gl mode");
	SDL_Surface * image = SDL_CreateRGBSurface(SDL_SWSURFACE, _width, _height, 24, 0x000000FF, 0x0000FF00, 0x00FF0000, 0);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
	SDL_Surface * flipped = flip_vertical(image);
	SDL_SaveBMP(flipped, filename.c_str());
	SDL_FreeSurface(image);
	SDL_FreeSurface(flipped);
}

}


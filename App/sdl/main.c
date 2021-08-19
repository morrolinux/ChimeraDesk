// Build with: gcc -o main main.c `pkg-config --libs --cflags mpv sdl2 SDL2_ttf` -std=c99

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>

#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

static Uint32 wakeup_on_mpv_render_update, wakeup_on_mpv_events;

mpv_handle *mpv;
SDL_Window *window;
SDL_Renderer *splash_renderer;
const char *btn_string[] = {NULL, "Button.left", "Button.middle", "Button.right"};

// TCP Server
static const int MSGLEN = 64;
static const int PORT = 12346;
struct sockaddr_in server, client;
int server_fd, client_fd, err;

// Fullscreen mode (WIP)
bool isFullScreen = false;
void toggle_fullscreen(SDL_Window* window, bool currentState)
{
    isFullScreen = !currentState;

    SDL_SetWindowFullscreen(window, !currentState);
    SDL_ShowCursor(currentState);
}

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void *get_proc_address_mpv(void *fn_ctx, const char *name)
{
    return SDL_GL_GetProcAddress(name);
}

static void on_mpv_events(void *ctx)
{
    SDL_Event event = {.type = wakeup_on_mpv_events};
    SDL_PushEvent(&event);
}

static void on_mpv_render_update(void *ctx)
{
    SDL_Event event = {.type = wakeup_on_mpv_render_update};
    SDL_PushEvent(&event);
}

static int send_message(const char *msg)
{
    char buffer[MSGLEN];
    const char *padding="#####################################################";
    int padLen = MSGLEN - strlen(msg);
    if(padLen < 0) padLen = 0;
    snprintf(buffer, MSGLEN, "%s %*.*s", msg, padLen, padLen, padding);
    printf("%s\n", buffer);
    err = send(client_fd, buffer, MSGLEN, 0);
    if (err < 0) on_error("Client write failed\n");
}

// translate to remote screen coordinates taking into account window size and borders
int translate_mouse_coords(int *x, int *y)
{
  int osd_border_top, osd_border_left, osd_w, osd_h, win_w, win_h, video_w, video_h, nx, ny;
  float video_ar, win_ar;

  mpv_get_property(mpv, "video-params/w", MPV_FORMAT_INT64, &video_w); 
  mpv_get_property(mpv, "video-params/h", MPV_FORMAT_INT64, &video_h); 
  SDL_GetWindowSize(window, &win_w, &win_h);

  // safeguard for mouse events before video feed
  if (video_w <= 0 || video_h <= 0 || win_w <= 0 || win_h <= 0){
    printf("translate_mouse_coords: video/win size values cannot be 0\n");
    return -1;
  }

  video_ar = video_w/(float)video_h;
  win_ar = win_w/(float)win_h;

  if (win_ar > video_ar){
      osd_h = win_h;
      osd_w = win_h * video_ar;
  } else {
      osd_w = win_w;
      osd_h = win_w / video_ar;
  }
  
  if (osd_w <= 0 || osd_h <= 0){
      printf("osd w/h wrong value: %d %d\n", osd_w, osd_h);
      return -1;
  }

  osd_border_left = (win_w - osd_w)/2;
  osd_border_top = (win_h - osd_h)/2;

  nx = (*x - osd_border_left) * video_w / osd_w;
  ny = (*y - osd_border_top) * video_h / osd_h;
  
  nx = (nx < 0) ? 0: nx;
  ny = (ny < 0) ? 0: ny;

  // printf("\ntranslate_mouse_coords(%d, %d): \
  //   \nosd_border_left: %d, osd_border_top: %d \
  //   \nwin_w: %d, win_h: %d \
  //   \nosd_w: %d, osd_h: %d \
  //   \nvideo_w: %d, video_h: %d \
  //   \nnx: %d, ny: %d\n", \
  //   *x, *y, \
  //   osd_border_left, osd_border_top, \
  //   win_w, win_h, \
  //   osd_w, osd_h, \
  //   video_w, video_h, \
  //   nx, ny);
  
  *x = nx;
  *y = ny;
  return 0;
}

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

int main(int argc, char *argv[])
{
    // Init TCP server
    server_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "could not create TCP listening socket");
    int server_flags = guard(fcntl(server_fd, F_GETFL), "could not get flags on TCP listening socket");
    guard(fcntl(server_fd, F_SETFL, server_flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    guard(bind(server_fd, (struct sockaddr *) &server, sizeof(server)), "could not bind");
    guard(listen(server_fd, 128), "could not listen");
    printf("TCP server is listening on port %d. Waiting for client to connect...\n", PORT);
    socklen_t client_len = sizeof(client);

    mpv = mpv_create();
    if (!mpv)
        die("context init failed");

    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0)
        die("mpv init failed");

    mpv_request_log_messages(mpv, "debug");

    mpv_set_property_string(mpv, "profile", "low-latency");
    int yes = 1;
    mpv_set_option(mpv, "untimed", MPV_FORMAT_FLAG, &yes);
    mpv_set_property(mpv, "untimed", MPV_FORMAT_FLAG, &yes);

    // Jesus Christ SDL, you suck!
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "no");

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        die("SDL init failed");

    window =
        SDL_CreateWindow("ChimeraDesk", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                                    SDL_WINDOW_RESIZABLE);
    if (!window)
        die("failed to create SDL window");

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    if (!glcontext)
        die("failed to create SDL GL context");

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &(mpv_opengl_init_params){
            .get_proc_address = get_proc_address_mpv,
        }},
        // Tell libmpv that you will call mpv_render_context_update() on render
        // context update callbacks, and that you will _not_ block on the core
        // ever (see <libmpv/render.h> "Threading" section for what libmpv
        // functions you can call at all when this is active).
        // In particular, this means you must call e.g. mpv_command_async()
        // instead of mpv_command().
        // If you want to use synchronous calls, either make them on a separate
        // thread, or remove the option below (this will disable features like
        // DR and is not recommended anyway).
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){1}},
        {0}
    };

    // This makes mpv use the currently set GL context. It will use the callback
    // (passed via params) to resolve GL builtin functions, as well as extensions.
    mpv_render_context *mpv_gl;
    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
        die("failed to initialize mpv GL context");

    // We use events for thread-safe notification of the SDL main loop.
    // Generally, the wakeup callbacks (set further below) should do as least
    // work as possible, and merely wake up another thread to do actual work.
    // On SDL, waking up the mainloop is the ideal course of action. SDL's
    // SDL_PushEvent() is thread-safe, so we use that.
    wakeup_on_mpv_render_update = SDL_RegisterEvents(1);
    wakeup_on_mpv_events = SDL_RegisterEvents(1);
    if (wakeup_on_mpv_render_update == (Uint32)-1 ||
        wakeup_on_mpv_events == (Uint32)-1)
        die("could not register events");

    // When normal mpv events are available.
    mpv_set_wakeup_callback(mpv, on_mpv_events, NULL);

    // When there is a need to call mpv_render_context_update(), which can
    // request a new frame to be rendered.
    // (Separate from the normal event handling mechanism for the sake of
    //  users which run OpenGL on a different thread.)
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_render_update, NULL);

    // prepare a splashscreen to show to the user while waiting for a client to connect 
    TTF_Init();
    splash_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* sans = TTF_OpenFont("fonts/FreeSans.ttf", 48);
    SDL_Color white = {255, 255, 255};
    SDL_Surface* surface_message = TTF_RenderText_Solid(sans, "Waiting for a client to connect...", white); 
    SDL_Surface* surface_logo = SDL_LoadBMP("logo.bmp");
    SDL_Texture* splash_message = SDL_CreateTextureFromSurface(splash_renderer, surface_message);
    SDL_Texture* splash_logo = SDL_CreateTextureFromSurface(splash_renderer, surface_logo);
    SDL_FreeSurface(surface_message);
    SDL_FreeSurface(surface_logo);

    SDL_Rect message_rect;
    message_rect.x = 0;   
    message_rect.y = 0; 
    message_rect.w = 480;
    message_rect.h = 40; 

    SDL_Rect logo_rect;
    logo_rect.x = 460; 
    logo_rect.y = 180; 
    logo_rect.w = 360;
    logo_rect.h = 360; 

    // Wait for TCP connection.
    // We do busy wait instead of a blocking wait BECAUSE this is a  
    // single-threaded application and we don't want to block the UI thread
    // or the OS will think the program freezed and prompt the user for termination.
    while (1) {
        SDL_RenderClear(splash_renderer);
        SDL_RenderCopy(splash_renderer, splash_logo, NULL, &logo_rect);
        SDL_RenderCopy(splash_renderer, splash_message, NULL, &message_rect);
        SDL_RenderPresent(splash_renderer);

        SDL_Event e;
        if (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                exit(0);
            }
        }

        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno == EWOULDBLOCK) {
                sleep(0.005);  // TODO: maybe find something better
            } else {
                perror("error when accepting connection");
                exit(1);
            }
        } else {
            printf("Client connected!\n");
            // flush all local events prior to client connection as they are not to be forwarded
            SDL_PumpEvents();
            SDL_FlushEvents(SDL_KEYDOWN, SDL_DROPCOMPLETE);
            break;
        }
    }

    // Play network video stream (async wait)
    const char *cmd[] = {"loadfile", "tcp://0.0.0.0:12345?listen", NULL};
    mpv_command_async(mpv, 0, cmd);
	
    const char *action;
    int x, y;
    int mouse_status = SDL_MOUSEBUTTONUP;
    // toggle_fullscreen(window, isFullScreen);

    while (1) {
        char buffer[MSGLEN];
        SDL_Event event;
        if (SDL_WaitEvent(&event) != 1)
            die("event loop error");
        int redraw = 0;
        switch (event.type) {
        case SDL_QUIT:
            goto done;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
                redraw = 1;
            break;
        case SDL_MOUSEWHEEL:
            SDL_GetMouseState(&x, &y);
            if(translate_mouse_coords(&x, &y) < 0) break;
            snprintf(buffer, MSGLEN, "mouse %d %d %s %d %d", x, y, "scroll", -event.wheel.x, event.wheel.y);  
            send_message(buffer);
            break;
        case SDL_MOUSEMOTION: 
            // if (mouse_status != SDL_MOUSEBUTTONDOWN) break;  // perf. optimization if slow network
            SDL_GetMouseState(&x, &y);
            if(translate_mouse_coords(&x, &y) < 0) break;
            snprintf(buffer, MSGLEN, "mouse %d %d %s", x, y, "move");  
            send_message(buffer);
            break;
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            // get event type (click/release) and button
            mouse_status = event.type;
            action = (event.type == SDL_MOUSEBUTTONDOWN) ? "click" : "release"; 
            const SDL_MouseButtonEvent mouse_event = event.button;

            // get mouse coordinates
            SDL_GetMouseState(&x, &y);
            if(translate_mouse_coords(&x, &y) < 0) break;
            snprintf(buffer, MSGLEN, "mouse %d %d %s %s", x, y, action, btn_string[mouse_event.button]);  
            send_message(buffer);
            break;      
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            action = (event.type == SDL_KEYDOWN) ? "press" : "release"; 
            snprintf(buffer, MSGLEN, "keyboard %s %d", action, event.key.keysym.sym);         
            send_message(buffer);
            break;
        default:
            // Happens when there is new work for the render thread (such as
            // rendering a new video frame or redrawing it).
            if (event.type == wakeup_on_mpv_render_update) {
                uint64_t flags = mpv_render_context_update(mpv_gl);
                if (flags & MPV_RENDER_UPDATE_FRAME)
                    redraw = 1;
            }
            // Happens when at least 1 new event is in the mpv event queue.
            if (event.type == wakeup_on_mpv_events) {
                // Handle all remaining mpv events.
                while (1) {
                    mpv_event *mp_event = mpv_wait_event(mpv, 0);
                    if (mp_event->event_id == MPV_EVENT_NONE)
                        break;
                    if (mp_event->event_id == MPV_EVENT_LOG_MESSAGE) {
                        mpv_event_log_message *msg = mp_event->data;
                        // Print log messages about DR allocations, just to
                        // test whether it works. If there is more than 1 of
                        // these, it works. (The log message can actually change
                        // any time, so it's possible this logging stops working
                        // in the future.)
                        if (strstr(msg->text, "DR image"))
                            printf("log: %s", msg->text);
                        continue;
                    }
                    printf("event: %s\n", mpv_event_name(mp_event->event_id));
                }
            }
        }
        if (redraw) {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            mpv_render_param params[] = {
                // Specify the default framebuffer (0) as target. This will
                // render onto the entire screen. If you want to show the video
                // in a smaller rectangle or apply fancy transformations, you'll
                // need to render into a separate FBO and draw it manually.
                {MPV_RENDER_PARAM_OPENGL_FBO, &(mpv_opengl_fbo){
                    .fbo = 0,
                    .w = w,
                    .h = h,
                }},
                // Flip rendering (needed due to flipped GL coordinate system).
                {MPV_RENDER_PARAM_FLIP_Y, &(int){1}},
                {0}
            };
            // See render_gl.h on what OpenGL environment mpv expects, and
            // other API details.
            mpv_render_context_render(mpv_gl, params);
            SDL_GL_SwapWindow(window);
        }
    }
done:
    // toggle_fullscreen(window, isFullScreen);

    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.
    mpv_render_context_free(mpv_gl);
    mpv_detach_destroy(mpv);

    // Destroy "splash screen" resources
    SDL_DestroyRenderer(splash_renderer);
    SDL_DestroyTexture(splash_message);

    printf("properly terminated\n");
    return 0;
}

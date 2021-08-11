// Build with: gcc -o main main.c `pkg-config --libs --cflags mpv sdl2` -std=c99

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <SDL.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>

#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

static Uint32 wakeup_on_mpv_render_update, wakeup_on_mpv_events;

mpv_handle *mpv;
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
void translate_mouse_coords(int *x, int *y)
{
  int osd_border_top, osd_border_left, w, h, video_w, video_h, nx, ny;
  mpv_get_property(mpv, "osd-dimensions/mt", MPV_FORMAT_INT64, &osd_border_top); 
  mpv_get_property(mpv, "osd-dimensions/ml", MPV_FORMAT_INT64, &osd_border_left); 
  mpv_get_property(mpv, "osd-width", MPV_FORMAT_INT64, &w); 
  mpv_get_property(mpv, "osd-height", MPV_FORMAT_INT64, &h); 
  mpv_get_property(mpv, "video-params/w", MPV_FORMAT_INT64, &video_w); 
  mpv_get_property(mpv, "video-params/h", MPV_FORMAT_INT64, &video_h); 
  w -= osd_border_left * 2;
  h -= osd_border_top * 2;
  
  nx = (*x - osd_border_left) * video_w / w;
  ny = (*y - osd_border_top) * video_h / h;

  nx = (nx < 0) ? 0: nx;
  ny = (ny < 0) ? 0: ny;
  
  *x = nx;
  *y = ny;
}

int main(int argc, char *argv[])
{
    // Init TCP server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) on_error("Could not create socket\n");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);
    err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
    if (err < 0) on_error("Could not bind socket\n");
    err = listen(server_fd, 128);
    if (err < 0) on_error("Could not listen on socket\n");
    printf("TCP server is listening on port %d\n", PORT);
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

    SDL_Window *window =
        SDL_CreateWindow("hi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         1000, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
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

    // Wait for TCP connection
    client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);
    if (client_fd < 0) on_error("Could not establish new connection\n");

    // Read TCP client message
    // int read = recv(client_fd, buf, BUFFER_SIZE, 0);
    // if (!read) break; // done reading
    // if (read < 0) on_error("Client read failed\n");
	
    // Play network video stream
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
            translate_mouse_coords(&x, &y);
            snprintf(buffer, MSGLEN, "mouse %d %d %s %d %d", x, y, "scroll", -event.wheel.x, event.wheel.y);  
            send_message(buffer);
            break;
        case SDL_MOUSEMOTION: 
            // if (mouse_status != SDL_MOUSEBUTTONDOWN) break;  // perf. optimization if slow network
            SDL_GetMouseState(&x, &y);
            translate_mouse_coords(&x, &y);
            snprintf(buffer, MSGLEN, "mouse %d %d %s", x, y, "move", NULL);  
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
            translate_mouse_coords(&x, &y);
            snprintf(buffer, MSGLEN, "mouse %d %d %s %s", x, y, action, btn_string[mouse_event.button], NULL);  
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

    printf("properly terminated\n");
    return 0;
}

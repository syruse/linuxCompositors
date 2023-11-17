#define WLR_USE_UNSTABLE
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/util/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct wayland_server
{
   struct wl_display *wl_display;
   struct wlr_renderer *renderer;
   struct wlr_allocator *allocator;
   struct wlr_backend *backend;
   struct wlr_output_layout *output_layout;
   struct wl_listener new_output; // when new outputs added
   struct wl_list outputs;
};

 struct target_output {
        struct wlr_output *wlr_output;
        struct wayland_server *server;
        struct timespec last_frame;
        struct wl_listener destroy;
        struct wl_listener frame;
        struct wl_list link;
 };

static void output_frame_notify(struct wl_listener *listener, void *data) {
   	/* This function is called every time an output is ready to display a frame,
	   * generally at the output's refresh rate (e.g. 60Hz). */
      struct target_output *output = wl_container_of(listener, output, frame);
      struct wlr_output *wlr_output = data;
      struct wlr_renderer *renderer = output->server->renderer;

	   /* wlr_output_attach_render makes the OpenGL context current. */
	   if (!wlr_output_attach_render(output->wlr_output, NULL)) {
	   	return;
	   }
	   /* The "effective" resolution can change if you rotate your outputs. */
	   int width, height;
	   wlr_output_effective_resolution(output->wlr_output, &width, &height);
	   /* Begin the renderer (calls glViewport and some other GL sanity checks) */
      wlr_renderer_begin(renderer, width, height);

      float color[4] = {1.0, 0, 0, 1.0};
      wlr_renderer_clear(renderer, color);

      // swap buffers and show final frame
      wlr_renderer_end(renderer);
      wlr_output_commit(output->wlr_output);
}

// when output is unplagged
static void output_destroy_notify(struct wl_listener *listener, void *data) {
        struct target_output *output = wl_container_of(listener, output, destroy);
        wl_list_remove(&output->link);
        wl_list_remove(&output->destroy.link);
        wl_list_remove(&output->frame.link);
        free(output);
}

static void new_output_notify(struct wl_listener *listener, void *data) {
      struct wayland_server *server = wl_container_of(
                      listener, server, new_output);
      struct wlr_output *wlr_output = data;

      /* Configures the output created by the backend to use our allocator
	   * and our renderer. Must be done once, before commiting the output */
	   wlr_output_init_render(wlr_output, server->allocator, server->renderer);

      /* Some backends don't have modes. DRM+KMS does, and we need to set a mode
       * before we can use the output. The mode is a tuple of (width, height,
       * refresh rate), and each monitor supports only a specific set of modes. We
       * just pick the monitor's preferred mode, a more sophisticated compositor
       * would let the user configure it. */
      if (!wl_list_empty(&wlr_output->modes)) {
      	struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
      	wlr_output_set_mode(wlr_output, mode);
      	wlr_output_enable(wlr_output, true);
      	if (!wlr_output_commit(wlr_output)) {
      		return;
      	}
      }

      struct target_output *output = calloc(1, sizeof(struct target_output));
      clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
      output->server = server;
      output->wlr_output = wlr_output;
      wl_list_insert(&server->outputs, &output->link);

      output->destroy.notify = output_destroy_notify;
      wl_signal_add(&wlr_output->events.destroy, &output->destroy);

      // Now, whenever an output is ready for a new frame, output_frame_notify will be called
      // wlr keeps track of receiving frames at certain rate and signalling us for drawing
      output->frame.notify = output_frame_notify;
      wl_signal_add(&wlr_output->events.frame, &output->frame);

       /* Adds this to the output layout. The add_auto function arranges outputs
       * from left-to-right in the order they appear. A more sophisticated
       * compositor would let the user configure the arrangement of outputs in the
       * layout.
       *
       * The output layout utility automatically adds a wl_output global to the
       * display, which Wayland clients can see to find out information about the
       * output (such as DPI, scale factor, manufacturer, etc).
       */
      //struct wlr_output_layout_output *l_output = wlr_output_layout_add_auto(server->output_layout,
      //                                                                       wlr_output);
      //struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
      //wlr_scene_output_layout_add_output(server->scene_layout, l_output, scene_output);
}

int main(int argc, char **argv)
{
   wlr_log_init(WLR_DEBUG, NULL);
   struct wayland_server server;

/* The Wayland display is managed by libwayland. It handles accepting
	 * clients from the Unix socket, manging Wayland globals, and so on. */
	server.wl_display = wl_display_create();
   assert(server.wl_display);
	/* The backend is a wlroots feature which abstracts the underlying input and
	 * output hardware. The autocreate option will choose the most suitable
	 * backend based on the current environment, such as opening an X11 window
	 * if an X11 server is running. */
	server.backend = wlr_backend_autocreate(server.wl_display);
   assert(server.backend);

	/* If we don't provide a renderer, autocreate makes a GLES2 renderer for us.
	 * The renderer is responsible for defining the various pixel formats it
	 * supports for shared memory, this configures that for clients. */
   server.renderer = wlr_renderer_autocreate(server.backend);
   assert(server.renderer);

	wlr_renderer_init_wl_display(server.renderer, server.wl_display);

   /* Autocreates an allocator for us.
	 * The allocator is the bridge between the renderer and the backend. It
	 * handles the buffer creation, allowing wlroots to render onto the
	 * screen */
	server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
   assert(server.allocator);

   /* This creates some hands-off wlroots interfaces. The compositor is
	 * necessary for clients to allocate surfaces and the data device manager
	 * handles the clipboard. Each of these wlroots interfaces has room for you
	 * to dig your fingers in and play with their behavior if you want. Note that
	 * the clients cannot set the selection directly without compositor approval,
	 * see the handling of the request_set_selection event below.*/
	wlr_compositor_create(server.wl_display, server.renderer);
	//wlr_data_device_manager_create(server.wl_display);

	/* Creates an output layout, which a wlroots utility for working with an
	 * arrangement of screens in a physical layout. */
	server.output_layout = wlr_output_layout_create();
   
   wl_list_init(&server.outputs);
   server.new_output.notify = new_output_notify;
   wl_signal_add(&server.backend->events.new_output, &server.new_output);

   /* Add a Unix socket to the Wayland display. */
	const char *socket = wl_display_add_socket_auto(server.wl_display);
	if (!socket) {
      wlr_log(WLR_ERROR, "adding Unix socket failed");
		wlr_backend_destroy(server.backend);
		return 1;
	}

	/* Start the backend. This will enumerate outputs and inputs, become the DRM
	 * master, etc */
	if (!wlr_backend_start(server.backend)) {
      wlr_log(WLR_ERROR, "Running server failed");
		wlr_backend_destroy(server.backend);
		wl_display_destroy(server.wl_display);
		return 1;
	}

	/* Set the WAYLAND_DISPLAY environment variable to our socket and run the
	 * startup command if requested. */
	setenv("WAYLAND_DISPLAY", socket, true);

	/* Run the Wayland event loop. This does not return until you exit the
	 * compositor. Starting the backend rigged up all of the necessary event
	 * loop configuration to listen to libinput events, DRM events, generate
	 * frame events at the refresh rate, and so on. */
	wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s",
			socket);
	wl_display_run(server.wl_display);

   wl_display_destroy(server.wl_display);
   return 0;
}

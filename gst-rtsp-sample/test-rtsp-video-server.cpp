#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <string>

GstElement* selector;
int switch_interval = 0;

static gboolean
timeout (GstRTSPServer * server)
{
  GstRTSPSessionPool *pool;

  pool = gst_rtsp_server_get_session_pool (server);
  gst_rtsp_session_pool_cleanup (pool);
  g_object_unref (pool);

  return TRUE;
}

// Function to switch between sources
static gboolean switch_sources(gpointer data) {
    GstPad *active_pad, *sinkpad;

    // Get the current active pad of the selector
    g_object_get(selector, "active-pad", &active_pad, NULL);

    // Get the other pad (source) to switch to
    sinkpad = (active_pad == gst_element_get_static_pad(selector, "sink_0")) ?
        gst_element_get_static_pad(selector, "sink_1") :
        gst_element_get_static_pad(selector, "sink_0");

    // Set the active pad to the other pad
    g_object_set(selector, "active-pad", sinkpad, NULL);

    // Unreference the pads
    gst_object_unref(active_pad);
    gst_object_unref(sinkpad);

    return G_SOURCE_CONTINUE;
}


// Signal handler for incoming media requests
static GstRTSPMediaFactory *on_media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media) {
    g_print("Media requested:\n");
    GstElement *element = gst_rtsp_media_get_element(media);

    selector = gst_bin_get_by_name(GST_BIN(element), "selector");

    // g_signal_connect(element, "pad-added", G_CALLBACK(on_new_state), NULL);

    selector = gst_bin_get_by_name(GST_BIN(element), "selector");

    const int INTERVAL = switch_interval;  // Change this value to set the switching interval (in seconds)
    g_timeout_add_seconds(INTERVAL, switch_sources, NULL);

    return factory;
}

GstElement* binPipeline;

GstElement *create_bin_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url)
{
  return binPipeline;
}


GstRTSPMediaFactory *custom_pipeline_factory(GstElement *bin)
{
  GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
  GstRTSPMediaFactoryClass *memberFunctions = GST_RTSP_MEDIA_FACTORY_GET_CLASS(factory);

  binPipeline = bin;
  memberFunctions->create_element = create_bin_element;
  return factory;
}

int
main (int argc, char *argv[])
{

  if(argc != 4){
    g_print("wrong number of arguments %d\n", argc);
  }
  std::string address = argv[1];
  std::string path_to_file = argv[2];
  switch_interval = std::stoi(argv[3]); 

  g_print("video file path %s\n", path_to_file.c_str());
  g_print("video switch interval %d\n", switch_interval);
  
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory_snow, *factory_ball, *factory_media_file;
  GstElement *pipeline;

  gst_init (&argc, &argv);

  std::string pipe = " rtspsrc location=rtsp://admin:Admin12345@reg.fuzzun.ru:50232/ISAPI/Streaming/Channels/101 latency=200 ! decodebin ! x264enc ! queue !"
    " input-selector name=selector !" 
    " rtph264pay name=pay0 pt=96 "
    " multifilesrc location=$ loop=true ! decodebin ! x264enc ! queue !"
    " selector.";
    
    auto pos = pipe.find("$");
    pipe.erase(pos, 1);

    pipe.insert(pos, path_to_file);

    g_print("pipeline: %s\n", pipe.c_str());
    pipeline = gst_parse_launch( pipe.c_str(), NULL);

    // pipeline = gst_parse_launch(
    //  " rtspsrc location=rtsp://admin:Admin12345@reg.fuzzun.ru:50232/ISAPI/Streaming/Channels/101 latency=200 ! decodebin ! x264enc bitrate=8000 ! queue !"
    //  " input-selector name=selector !" 
    //  " rtph264pay name=pay0 pt=96 "
    //  " multifilesrc location=/home/vladislav/Videos/bunny_short_original.avi loop=true ! decodebin ! x264enc bitrate=8000 ! queue !"
    //  " selector.", NULL);

    if (!pipeline) {
        g_print("Failed to create the pipeline.\n");
        return -1;
    }


  loop = g_main_loop_new (NULL, FALSE);

  /* create a server instance */
  server = gst_rtsp_server_new ();

  gst_rtsp_server_set_service(server, "8554");
  gst_rtsp_server_set_address(server, address.c_str());

  mounts = gst_rtsp_server_get_mount_points (server);

  factory_snow = custom_pipeline_factory(pipeline);


  gst_rtsp_mount_points_add_factory (mounts, "/test", factory_snow);

  g_object_unref (mounts);

  /* attach the server to the default maincontext */
  if (gst_rtsp_server_attach (server, NULL) == 0)
    goto failed;

  /* add a timeout for the session cleanup */
  g_timeout_add_seconds (2, (GSourceFunc) timeout, server);
    
  g_signal_connect(factory_snow, "media-configure", G_CALLBACK(on_media_configure), NULL);
  
  g_print ("stream ready at rtsp://%s:8554/test\n", address.c_str());
  g_main_loop_run (loop);

  return 0;

  /* ERRORS */
failed:
  {
    g_print ("failed to attach the server\n");
    return -1;
  }
}

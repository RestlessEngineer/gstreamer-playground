# LOCALISATION FILTER

## Introduction

The Localization Filter is a simple example of how to utilize localization in GStreamer filters.

## Installation

To install the Localization Filter, follow these steps:

```
mkdir build; cd build
cmake ..
make
```

## Quick Start

You can move or copy the library to your GStreamer library directory. For example (on my system):

```
sudo cp libgstlocfilter.so  /usr/lib/x86_64-linux-gnu/gstreamer-1.0/ 
```

Alternatively, you can set the GST_PLUGIN_PATH temporarily for your current terminal session. Here's how:

```
cd build
export GST_PLUGIN_PATH=$(pwd) 
```

Now, you can use the Localization Filter in your current terminal session.

## Example Usage

To demonstrate how to use the Localization Filter, follow this example:

Launch GStreamer from the build directory

```
gst-launch-1.0 filesrc location=../Videos/birds.mp4 ! decodebin ! videoconvert ! locfilter show-boxes=true  ! videoconvert ! autovideosink
```

This pipeline loads a video file (birds.mp4), decodes it, applies the locfilter with the show-boxes parameter set to true, and displays the result on the screen.


## Options

### show-boxes

**Description:** Determines whether to display boxes or not.

You can use the `show-boxes` option to control the display of boxes in the output of the Localization Filter. Set it to `true` to display boxes, and `false` to hide them.
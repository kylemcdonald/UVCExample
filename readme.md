# UVCExample

Combines multiple libraries to access UVC cameras on Mac that are not supported by the native Mac UVC library.

- https://github.com/libusb/libusb
- https://github.com/ktossell/libuvc/
- https://github.com/jdthomas/bayer2rgb/
- https://github.com/openframeworks/openFrameworks/

There is a big problem with this example right now, which is that libusb or libuvc is not being closed correctly (or there is a problem with the library itself). This means every time that you want to start the app, you need to unplug and replug the camera.

The copies of libusb and libuvc binaries were taken from Homebrew. They correspond to libusb 1.0.22 and libuvc 0.0.6.

Tested with openFrameworks commit c274c7fb51b4ae0552cd4cdb00475458aeeb610a from May 10, 2018.

Tested with the Imaging Source 37BUX290 https://www.theimagingsource.com/products/industrial-cameras/usb-3.1-color/dfk37bux290/
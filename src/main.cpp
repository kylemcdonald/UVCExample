#include "ofMain.h"

#include "libuvc.h"
#include "bayer.h"

std::vector<uint8_t> bayer, rgb;
std::vector<uint16_t> bayer16, rgb16;
bool isFrameNew = false;

uvc_context_t *ctx;
uvc_device_t *dev;
uvc_device_handle_t *devh;
uvc_stream_ctrl_t ctrl;
uvc_error_t res;

// this isn't really safe. there should be a thread channel here
// for buffering the data between the callback and the main draw loop.
// for higher framerates and faster motion the tearing will be obvious.
void cb(uvc_frame_t *frame, void *ptr) {
    memcpy(&bayer[0], frame->data, frame->data_bytes);
    isFrameNew = true;
}

class ofApp : public ofBaseApp {
public:
    ofImage img;
    vector<uint16_t> lut;
    
    void setup() {
        bayer.resize(1920*1080);
        rgb.resize(1920*1080*3);
        bayer16.resize(1920*1080);
        rgb16.resize(1920*1080*3);
        
        int n = 256;
        int min_brightness = 0;
        float gamma = 2.0;
        lut.resize(n);
        for(int i = 0; i < n; i++) {
            float x = pow((i - min_brightness) / (255. - min_brightness), 1/gamma);
            if (x > 1) x = 1;
            else if (x < 0) x = 0;
            lut[i] = x * ((1 << 16) - 1);
        }
        
        res = uvc_init(&ctx, NULL);
        
        if (res < 0) {
            uvc_perror(res, "uvc_init");
            cout << "uvc_init error" << endl;
        }
        
        cout << "UVC initialized" << endl;
        
        /* Locates the first attached UVC device, stores in dev */
        res = uvc_find_device(ctx, &dev, 0, 0, NULL); /* filter devices: vendor_id, product_id, "serial_num" */
        
        if (res < 0) {
            uvc_perror(res, "uvc_find_device"); /* no devices found */
        } else {
            cout << "Device found" << endl;
            
            /* Try to open the device: requires exclusive access */
            res = uvc_open(dev, &devh);
            
            if (res < 0) {
                uvc_perror(res, "uvc_open"); /* unable to open device */
            } else {
                cout << "Device opened" << endl;
                
                /* Print out a message containing all the information that libuvc
                 * knows about the device */
                uvc_print_diag(devh, stderr);
                
                /* Try to negotiate a 640x480 30 fps YUYV stream profile */
                res = uvc_get_stream_ctrl_format_size(devh, &ctrl, /* result stored in ctrl */
                                                      UVC_FRAME_FORMAT_SRGGB8, /* YUV 422, aka YUV 4:2:2. try _COMPRESSED */
                                                      1920, 1080, 30 /* width, height, fps */
                                                      );
                
                /* Print out the result */
                uvc_print_stream_ctrl(&ctrl, stderr);
                
                if (res < 0) {
                    uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
                } else {
                    /* Start the video stream. The library will call user function cb:
                     *   cb(frame, (void*) 12345)
                     */
                    res = uvc_start_streaming(devh, &ctrl, cb, (void*)12345, 0);
                    
                    if (res < 0) {
                        uvc_perror(res, "start_streaming"); /* unable to start stream */
                    } else {
                        // libuvc will claim the video is streaming
                        // but in fact it will never make the callback
                        cout << "Streaming..." << endl;
                        
                        uvc_set_ae_mode(devh, 0); /* e.g., turn off auto exposure */
                    }
                }
                
            }
            
        }
        
    }
    void exit() {
        // for some reason all of this is useless in genuinely closing the connection
        // you will always need to disconnect and reconnect the camera
        // every time you run the app.
        
        /* End the stream. Blocks until last callback is serviced */
        uvc_stop_streaming(devh);
        cout << "Done streaming." << endl;
        
        /* Release our handle on the device */
        uvc_close(devh);
        cout << "Device closed" << endl;
        
        /* Release the device descriptor */
        uvc_unref_device(dev);
        
        /* Close the UVC context. This closes and cleans up any existing device handles,
         * and it closes the libusb context if one was not provided. */
        uvc_exit(ctx);
        cout << "UVC exited" << endl;
    }
    void update() {
        if(isFrameNew) {
            bool useLut = true;
            
            if(useLut) {
                // i think it's better to use the lut to map to a 16 bit space
                // if it's done before demosaicing then the colors are less posterized
                for(int i = 0; i < bayer.size(); i++) {
                    bayer16[i] = lut[bayer[i]];
                }
                dc1394_bayer_decoding_16bit(&bayer16[0],
                                            &rgb16[0],
                                            1920, 1080,
                                            DC1394_COLOR_FILTER_RGGB,
                                            DC1394_BAYER_METHOD_BILINEAR,
                                            16);
                for(int i = 0; i < rgb.size(); i++) {
                    rgb[i] = rgb16[i] >> 8; // could add 0-256 diffusion here
                }
            } else {
                dc1394_bayer_decoding_8bit(&bayer[0],
                                           &rgb[0],
                                           1920, 1080,
                                           DC1394_COLOR_FILTER_RGGB,
                                           DC1394_BAYER_METHOD_BILINEAR);
            }
            
            img.setFromPixels(&rgb[0], 1920, 1080, OF_IMAGE_COLOR);
            img.update();
            isFrameNew = false;
        }
    }
    void draw() {
        if(img.getTexture().isAllocated()) {
            img.draw(0, 0);
        }
    }
    void keyPressed(int key) {
        if(key == ' ') {
            img.save(ofGetTimestampString() + ".png");
        }
    }
};

int main() {
    ofSetupOpenGL(1920, 1080, OF_WINDOW);
    ofRunApp(new ofApp());
}

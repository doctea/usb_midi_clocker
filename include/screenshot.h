#include "Config.h"

#include <SdFat.h>
#include "ST7789_t3.h"

//#include <SdFatUtil.h>

/*
 WRITE BMP TO SD CARD
 Jeff Thompson
 Summer 2012

 from https://forum.arduino.cc/t/writing-binary-file-to-sd-creating-bmp-files/110331/6
 
 Writes pixel data to an SD card, saved as a BMP file.  Lots of code
 via the following...
 
 BMP header and pixel format:
   http://stackoverflow.com/a/2654860
 
 SD save:
   http://arduino.cc/forum/index.php?topic=112733 (lots of thanks!)
 ... and the SdFat example files too
 
 www.jeffreythompson.org
 */

bool save_screenshot(ST7789_t3 *screen) {

    Serial.println(F("save_screenshot!"));

    char name[] = "umc_0000.bmp";       // filename convention (will auto-increment)
    const int w = screen->width();                   // image width in pixels
    const int h = screen->height();                    // " height
    //const boolean debugPrint = true;    // print details of process over serial?
        
    const int imgSize = w*h;
    //int px[w*h];                        // actual pixel data (grayscale - added programatically below)
    const uint16_t *px = screen->getFrameBuffer();

    SdFile file;

    Serial.println(F("Looking for a free filename..")); Serial_flush();
    // if name exists, create new filename
    for (unsigned int i=0; i<10000; i++) {
        name[4] = (i/1000)%10 + '0';    // thousands place
        name[5] = (i/100)%10 + '0';     // hundreds
        name[6] = (i/10)%10 + '0';      // tens
        name[7] = i%10 + '0';           // ones
        if (file.open(name, O_CREAT | O_EXCL | O_WRITE)) {
            break;
        }
    }
    Serial.printf(F("Found one in %s!\n"), name); Serial_flush();

    // set fileSize (used in bmp header)
    const int rowSize = 4 * ((3*w + 3)/4);      // how many bytes in the row (used to create padding)
    const int fileSize = 54 + h*rowSize;        // headers (54 bytes) + pixel data

    // create image data; heavily modified version via:
    // http://stackoverflow.com/a/2654860
    /*unsigned char *img = NULL;            // image data
    if (img) {                            // if there's already data in the array, clear it
        free(img);
    }*/
    //img = (unsigned char *)malloc(3*imgSize);

    // create padding (based on the number of pixels in a row
    unsigned char bmpPad[rowSize - 3*w];
    for (unsigned int i=0; i<sizeof(bmpPad); i++) {         // fill with 0s
        bmpPad[i] = 0;
    }

    // create file headers (also taken from StackOverflow example)
    unsigned char bmpFileHeader[14] = {            // file header (always starts with BM!)
        'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0   };
    unsigned char bmpInfoHeader[40] = {            // info about the file (size, etc)
        40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0   };

    bmpFileHeader[ 2] = (unsigned char)(fileSize      );
    bmpFileHeader[ 3] = (unsigned char)(fileSize >>  8);
    bmpFileHeader[ 4] = (unsigned char)(fileSize >> 16);
    bmpFileHeader[ 5] = (unsigned char)(fileSize >> 24);

    bmpInfoHeader[ 4] = (unsigned char)(       w      );
    bmpInfoHeader[ 5] = (unsigned char)(       w >>  8);
    bmpInfoHeader[ 6] = (unsigned char)(       w >> 16);
    bmpInfoHeader[ 7] = (unsigned char)(       w >> 24);
    bmpInfoHeader[ 8] = (unsigned char)(       h      );
    bmpInfoHeader[ 9] = (unsigned char)(       h >>  8);
    bmpInfoHeader[10] = (unsigned char)(       h >> 16);
    bmpInfoHeader[11] = (unsigned char)(       h >> 24);

    // write the file (thanks forum!)
    file.write(bmpFileHeader, sizeof(bmpFileHeader));    // write file header
    file.write(bmpInfoHeader, sizeof(bmpInfoHeader));    // " info header

    for (int y=0; y<h; y++) {                            // iterate image array
        //file.write(img+(w*(h-i-1)*3), 3*w);                // write px data
        for (int x = 0 ; x < w ; x++) {
            const int addr = imgSize - ((y * w) + (w-x));
            const uint16_t v = px[addr];
            const unsigned char red = (v & 0xf800) >> 11;
            const unsigned char green = (v & 0x07e0) >> 5;
            const unsigned char blue = v & 0x001f;
            //file.write(&px[i], 2*w);
            file.write(blue << 3);
            file.write(green << 2);
            file.write(red << 3);
        }
        file.write(bmpPad, (4-(w*3)%4)%4);                 // and padding as needed
    }
    file.close();                                        // close file when done writing

    //if (debugPrint) {
        Serial.print("\n\n---\n");
    //}
    
    return true;
}


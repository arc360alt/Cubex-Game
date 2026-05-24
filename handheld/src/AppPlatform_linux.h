#ifndef APPPLATFORM_LINUX_H__
#define APPPLATFORM_LINUX_H__

#include "AppPlatform.h"
#include "platform/log.h"
#include "client/renderer/gles.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <png.h>

static void png_linux_readFile(png_structp pngPtr, png_bytep data, png_size_t length) {
    ((std::istream*)png_get_io_ptr(pngPtr))->read((char*)data, length);
}

class AppPlatform_linux : public AppPlatform
{
public:
    AppPlatform_linux() {}

    void saveScreenshot(const std::string& filename, int glWidth, int glHeight) {
        if (glWidth <= 0 || glHeight <= 0)
            return;

        std::vector<unsigned char> pixels((size_t)glWidth * (size_t)glHeight * 4u);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, glWidth, glHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        FILE* fp = fopen(filename.c_str(), "wb");
        if (!fp) {
            LOGI("Couldn't save screenshot (open failed): %s\n", filename.c_str());
            return;
        }

        png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!pngPtr) {
            fclose(fp);
            return;
        }

        png_infop infoPtr = png_create_info_struct(pngPtr);
        if (!infoPtr) {
            png_destroy_write_struct(&pngPtr, NULL);
            fclose(fp);
            return;
        }

        if (setjmp(png_jmpbuf(pngPtr))) {
            png_destroy_write_struct(&pngPtr, &infoPtr);
            fclose(fp);
            return;
        }

        png_init_io(pngPtr, fp);
        png_set_IHDR(pngPtr, infoPtr, glWidth, glHeight, 8, PNG_COLOR_TYPE_RGBA,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(pngPtr, infoPtr);

        std::vector<png_bytep> rows((size_t)glHeight);
        const int stride = glWidth * 4;
        for (int y = 0; y < glHeight; ++y)
            rows[(size_t)y] = (png_bytep)(pixels.data() + (size_t)(glHeight - 1 - y) * (size_t)stride);

        png_write_image(pngPtr, rows.data());
        png_write_end(pngPtr, NULL);

        png_destroy_write_struct(&pngPtr, &infoPtr);
        fclose(fp);

        LOGI("Saved screenshot: %s\n", filename.c_str());
    }

    bool isSuperFast() { return true; }

    bool supportsTouchscreen() { return false; }

    int getScreenWidth()  { return 854; }
    int getScreenHeight() { return 480; }

    float getPixelsPerMillimeter() { return 4.0f; }

    int checkLicense() { return 0; }
    bool hasBuyButtonWhenInvalidLicense() { return false; }

    TextureData loadTexture(const std::string& filename_, bool textureFolder)
    {
        TextureData out;

        std::string filename = textureFolder
            ? "data/images/" + filename_
            : filename_;

        std::ifstream source(filename.c_str(), std::ios::binary);
        if (!source) {
            LOGI("Couldn't find file: %s\n", filename.c_str());
            return out;
        }

        png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!pngPtr)
            return out;

        png_infop infoPtr = png_create_info_struct(pngPtr);
        if (!infoPtr) {
            png_destroy_read_struct(&pngPtr, NULL, NULL);
            return out;
        }

        png_set_read_fn(pngPtr, (void*)&source, png_linux_readFile);
        png_read_info(pngPtr, infoPtr);

        // Normalize any PNG format (palette, grayscale, tRNS, etc.) to RGBA.
        // The renderer assumes 4 bytes per pixel for every loaded texture.
        int colorType = png_get_color_type(pngPtr, infoPtr);
        int bitDepth = png_get_bit_depth(pngPtr, infoPtr);
        if (bitDepth == 16)
            png_set_strip_16(pngPtr);
        if (colorType == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(pngPtr);
        if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
            png_set_expand_gray_1_2_4_to_8(pngPtr);
        if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(pngPtr);
        if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(pngPtr);
        if (!(colorType & PNG_COLOR_MASK_ALPHA))
            png_set_filler(pngPtr, 0xFF, PNG_FILLER_AFTER);

        png_read_update_info(pngPtr, infoPtr);

        out.w = png_get_image_width(pngPtr, infoPtr);
        out.h = png_get_image_height(pngPtr, infoPtr);

        png_bytep* rowPtrs = new png_bytep[out.h];
        out.data = new unsigned char[4 * out.w * out.h];
        out.memoryHandledExternally = false;

        int stride = 4 * out.w;
        for (int i = 0; i < out.h; ++i)
            rowPtrs[i] = (png_bytep)&out.data[i * stride];

        png_read_image(pngPtr, rowPtrs);
        png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
        delete[] rowPtrs;
        source.close();

        return out;
    }

    BinaryBlob readAssetFile(const std::string& filename)
    {
        std::ifstream file(("data/" + filename).c_str(), std::ios::binary | std::ios::ate);
        if (!file)
            return BinaryBlob();

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        BinaryBlob blob;
        blob.size = (int)size;
        blob.data = new unsigned char[size];
        if (!file.read((char*)blob.data, size)) {
            delete[] blob.data;
            return BinaryBlob();
        }
        return blob;
    }

    std::string getDateString(int s)
    {
        std::stringstream ss;
        ss << s << " s (UTC)";
        return ss.str();
    }
};

#endif /* APPPLATFORM_LINUX_H__ */

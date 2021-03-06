#pragma once

#include <vector>
#include <cstdint>

/**
 * Helper class to read bitmap data from a *.bmp file.
 */
class bitmap_image
{
public:
    /**
     * Read a *.bmp file and extract bitmap data.
     * @param fileName
     */
    bitmap_image(const char* fileName);
    /**
     * Destructor.
     */
    ~bitmap_image();
    /**
     * Return the bitmap data.
     * Each pixel is represented by 3 bytes.
     * @return Bitmap data.
     */
    const std::vector< char >& data() const;
    /**
     * Return width of the bitmap in pixels.
     * @return Width of the bitmap in pixels.
     */
    int width() const;
    /**
     * Return height of the bitmap in pixels.
     * @return Height of the bitmap in pixels.
     */
    int height() const;

private:
    /**
     * Size of the BMP header in bytes.
     */
    constexpr static int HEADER_SIZE = 54;
    /**
     * First byte of the header signature.
     */
    constexpr static int HEADER_SIGNATURE_1 = 'B';
    /**
     * Second byte of the header signature.
     */
    constexpr static int HEADER_SIGNATURE_2 = 'M';
    /**
     * BMP uses 3 bytes per pixel: R, G and B.
     */
    constexpr static int BYTES_PER_PIXEL = 3;
    /**
     * Offset in the header for 4-bytes data size value.
     */
    constexpr static int DATA_SIZE_OFFSET = 0x22;
    /**
     * Offset in the header for 4-bytes data pointer.
     */
    constexpr static int DATA_POINTER_OFFSET = 0x0A;
    /**
     * Offset in the header for 4-bytes width value.
     */
    constexpr static int WIDTH_OFFSET = 0x12;
    /**
     * Offset in the header for 4-bytes height value.
     */
    constexpr static int HEIGHT_OFFSET = 0x16;

    /**
     * Bitmap data array.
     */
    std::vector< char > m_data;
    /**
     * Bitmap width in pixels.
     */
    int32_t m_width;
    /**
     * Bitmap height in pixels.
     */
    int32_t m_height;
};

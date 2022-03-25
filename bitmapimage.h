#pragma once

#include <fstream>

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
    bitmap_image(const char* fileName)
        : m_data(nullptr)
        , m_data_size(0)
        , m_width(0)
        , m_height(0)
    {
        // Open the file.
        std::ifstream ifs(fileName);
        if (!ifs) {
            return;
        }
        // Read BMP header.
        char header[HEADER_SIZE];
        ifs.read(header, HEADER_SIZE);
        if (!ifs) {
            return;
        }
        // Verify header signature.
        if (header[0] != HEADER_SIGNATURE_1 || header[1] != HEADER_SIGNATURE_2){
            return;
        }
        // Read bitmap metadata.
        m_data_size = *(int32_t*)(&header[DATA_SIZE_OFFSET]);
        m_width = *(int32_t*)(&header[WIDTH_OFFSET]);
        m_height = *(int32_t*)(&header[HEIGHT_OFFSET]);
        if (m_width == 0 || m_height == 0) {
            m_data_size = 0;
            m_width = 0;
            m_height = 0;
            return;
        }
        // Read bitmap data.
        int dataPointer = *(int32_t*)(&header[DATA_POINTER_OFFSET]);
        // If size of pointer are zero we should use default values.
        if (m_data_size == 0) {
            // Derive from the image size.
            m_data_size = m_width * m_height * BYTES_PER_PIXEL;
        }
        if (dataPointer == 0){
            // By default data starts right after header.
            dataPointer = HEADER_SIZE;
        }
        // Read bitmap data.
        m_data = new char[m_data_size];
        ifs.read(m_data, m_data_size);
        if (!ifs.good() && !ifs.eof()) {
            delete[] m_data;
            m_data = nullptr;
            m_data_size = 0;
            m_width = 0;
            m_height = 0;
        }
    }
    /**
     * Destructor.
     */
    ~bitmap_image()
    {
        delete[] m_data;
        m_data = nullptr;
        m_data_size = 0;
        m_width = 0;
        m_height = 0;

    }
    /**
     * Return a pointer to the bitmap data.
     * Each pixel is represented by 3 bytes.
     * The pointer is only walid while the bitmap_image object exists.
     * @return
     */
    char* data()
    {
        return m_data;
    }
    /**
     * Return size of the bitmap data.
     * @return Size of the bitmap data in bytes.
     */
    int size()
    {
        return m_data_size;
    }
    /**
     * Return width of the bitmap in pixels.
     * @return Width of the bitmap in pixels.
     */
    int width()
    {
        return m_width;
    }
    /**
     * Return height of the bitmap in pixels.
     * @return Height of the bitmap in pixels.
     */
    int height()
    {
        return m_height;
    }

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
    char* m_data;
    /**
     * Bitmap data array size.
     */
    int32_t m_data_size;
    /**
     * Bitmap width in pixels.
     */
    int32_t m_width;
    /**
     * Bitmap height in pixels.
     */
    int32_t m_height;
};

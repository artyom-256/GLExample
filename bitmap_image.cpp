#include "bitmap_image.h"

bitmap_image::bitmap_image(const char* fileName)
    : m_width(0)
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
    int dataSize = *(int32_t*)(&header[DATA_SIZE_OFFSET]);
    m_width = *(int32_t*)(&header[WIDTH_OFFSET]);
    m_height = *(int32_t*)(&header[HEIGHT_OFFSET]);
    if (m_width == 0 || m_height == 0) {
        m_width = 0;
        m_height = 0;
        return;
    }
    // Read bitmap data.
    int dataPointer = *(int32_t*)(&header[DATA_POINTER_OFFSET]);
    // If size of pointer are zero we should use default values.
    if (dataSize == 0) {
        // Derive from the image size.
        dataSize = m_width * m_height * BYTES_PER_PIXEL;
    }
    if (dataPointer == 0){
        // By default data starts right after header.
        dataPointer = HEADER_SIZE;
    }
    // Read bitmap data.
    m_data.resize(dataSize);
    ifs.read(&m_data[0], dataSize);
    if (!ifs.good() && !ifs.eof()) {
        m_data.clear();
        m_width = 0;
        m_height = 0;
    }
}

bitmap_image::~bitmap_image()
{
    m_width = 0;
    m_height = 0;
}

const std::vector< char >& bitmap_image::data() const
{
    return m_data;
}

int bitmap_image::width() const
{
    return m_width;
}

int bitmap_image::height() const
{
    return m_height;
}

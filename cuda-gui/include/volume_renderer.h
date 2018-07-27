#include <driver_types.h>
typedef unsigned int uint;
void set_box_size(float *size);
void setTextureFilterMode(bool bLinearFilter);
void reset_render_buffer(int width, int height);
void initCuda(void *h_volume, cudaExtent volumeSize);
void update_volume(void *h_volume, cudaExtent volumeSize);
void freeCudaBuffers();
void render_kernel(dim3 gridSize,
    dim3 blockSize,
    uint *d_output,
    uint imageW,
    uint imageH,
    float density,
    float slice_position,
    float volume_rendering);
void copyInvViewMatrix(float *invViewMatrix, size_t sizeofMatrix);
void set_box_res(int *res);
void set_mirroring(bool *new_mirroring);

typedef unsigned char uchar;
typedef unsigned char uint8;
typedef unsigned int uint;
typedef float real;

struct VolumeRenderer {
    dim3 blockSize;
    dim3 gridSize;
    float3 viewRotation;
    float3 viewTranslation;
    float invViewMatrix[12];
    real volume_rendering = 0.0f;

    real density = 0.01f;
    real slice = 0.99999f;
    bool linearFiltering = false;

    uint pbo;  // OpenGL pixel buffer object
    uint tex;  // OpenGL texture object
    cudaGraphicsResource
        *cuda_pbo_resource;  // CUDA Graphics Resource (to transfer PBO)

    std::vector<uint8> img_data;
    int channel;

    VolumeRenderer();

    void sample();

    // render image using CUDA
    void render();

    void display();

    ~VolumeRenderer();

    void initPixelBuffer();
};
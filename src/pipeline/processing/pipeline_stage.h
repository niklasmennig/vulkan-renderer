// single processing pipeline stage
// uses compute shaders to process render data
class ProcessingPipelineStage {
    // called when renderer is resized
    void on_resize();

    // allocate data buffers, perform processor initialization
    void initialize();

    // perform processing step
    void process();
};
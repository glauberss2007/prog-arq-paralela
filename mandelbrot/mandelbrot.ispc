// Função auxiliar que calcula se um ponto complexo (c_re, c_im) pertence ao conjunto de Mandelbrot
// Retorna o número de iterações até divergir (ou maxIterations se convergir)
static inline int mandel(float c_re, float c_im, int count) {
    float z_re = c_re, z_im = c_im;  // Inicializa z = c
    int i;
    
    for (i = 0; i < count; ++i) {
        // Critério de divergência: |z| > 2
        if (z_re * z_re + z_im * z_im > 4.f)
           break;

        // Iteração: z = z² + c
        float new_re = z_re*z_re - z_im*z_im;  // Parte real de z²
        float new_im = 2.f * z_re * z_im;      // Parte imaginária de z²
        z_re = c_re + new_re;                  // z² + c
        z_im = c_im + new_im;
    }

    return i;  // Retorna o número de iterações
}

// Versão vetorizada usando foreach do ISPC (SIMD puro)
export void mandelbrot_ispc(uniform float x0, uniform float y0, 
                            uniform float x1, uniform float y1,
                            uniform int width, uniform int height, 
                            uniform int maxIterations,
                            uniform int output[])
{
    // Calcula o tamanho do passo entre pixels
    float dx = (x1 - x0) / width;
    float dy = (y1 - y0) / height;

    // Loop paralelo sobre todos os pixels da imagem
    // O ISPC automaticamente vetoriza este loop usando SIMD
    foreach (j = 0 ... height, i = 0 ... width) {
            // Converte coordenadas de pixel para coordenadas complexas
            float x = x0 + i * dx;
            float y = y0 + j * dy;

            // Calcula índice linear no array de output
            int index = j * width + i;
            
            // Calcula valor do pixel
            output[index] = mandel(x, y, maxIterations);
    }
}

// Kernel modificado para suportar tasking (paralelismo multicore)
task void mandelbrot_ispc_task(uniform float x0, uniform float y0, 
                               uniform float x1, uniform float y1,
                               uniform int width, uniform int height,
                               uniform int rowsPerTask,      // Linhas por task
                               uniform int maxIterations,
                               uniform int output[])
{
    // taskIndex é uma variável built-in do ISPC que identifica a task atual
    uniform int ystart = taskIndex * rowsPerTask;  // Linha inicial desta task
    uniform int yend = ystart + rowsPerTask;       // Linha final desta task
    
    uniform float dx = (x1 - x0) / width;
    uniform float dy = (y1 - y0) / height;
    
    // Processa apenas as linhas atribuídas a esta task
    foreach (j = ystart ... yend, i = 0 ... width) {
            float x = x0 + i * dx;
            float y = y0 + j * dy;
            
            int index = j * width + i;
            output[index] = mandel(x, y, maxIterations);
    }
}

// Versão que combina SIMD com paralelismo multicore (tasks)
export void mandelbrot_ispc_withtasks(uniform float x0, uniform float y0,
                                      uniform float x1, uniform float y1,
                                      uniform int width, uniform int height,
                                      uniform int maxIterations,
                                      uniform int output[])
{
    // Divide a imagem em 2 tasks (cada uma processa metade das linhas)
    uniform int rowsPerTask = height / 2;

    // Lança 2 tasks em paralelo
    // Cada task executará mandelbrot_ispc_task com uma parte diferente da imagem
    launch[2] mandelbrot_ispc_task(x0, y0, x1, y1,
                                     width, height,
                                     rowsPerTask,
                                     maxIterations,
                                     output); 
}
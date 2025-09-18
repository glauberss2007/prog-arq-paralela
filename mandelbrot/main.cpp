#include <stdio.h>
#include <algorithm>
#include <getopt.h>

#include "CycleTimer.h"          // Medição precisa de tempo
#include "mandelbrot_ispc.h"     // Funções ISPC (implementação SIMD)

// Declarações de funções externas
extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);

extern void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations,
    int output[]);

extern void writePPMImage(
    int* data,
    int width, int height,
    const char *filename,
    int maxIterations);

// Função de verificação de resultados
bool verifyResult (int *gold, int *result, int width, int height) {
    int i, j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (gold[i * width + j] != result[i * width + j]) {
                printf ("Mismatch : [%d][%d], Expected : %d, Actual : %d\n",
                            i, j, gold[i * width + j], result[i * width + j]);
                return 0;
            }
        }
    }

    return 1;
}

// Função para escalar e deslocar a região de visualização
void scaleAndShift(float& x0, float& x1, float& y0, float& y1,
              float scale,
              float shiftX, float shiftY)
{
    x0 *= scale;
    x1 *= scale;
    y0 *= scale;
    y1 *= scale;
    x0 += shiftX;
    x1 += shiftX;
    y0 += shiftY;
    y1 += shiftY;
}

using namespace ispc;

// Função de ajuda para mostrar uso do programa
void usage(const char* progname) {
    printf("Usage: %s [options]\n", progname);
    printf("Program Options:\n");
    printf("  -t  --tasks        Run ISPC code implementation with tasks\n");
    printf("  -v  --view <INT>   Use specified view settings\n");
    printf("  -?  --help         This message\n");
}

int main(int argc, char** argv) {
    // Parâmetros da imagem e do cálculo
    const unsigned int width = 1200;    // Largura da imagem
    const unsigned int height = 800;    // Altura da imagem
    const int maxIterations = 256;      // Máximo de iterações por pixel

    // Região inicial do plano complexo a ser renderizada
    float x0 = -2;  // Coordenada x mínima
    float x1 = 1;   // Coordenada x máxima  
    float y0 = -1;  // Coordenada y mínima
    float y1 = 1;   // Coordenada y máxima

    bool useTasks = false;  // Flag para usar versão com tasks

    // Parse das opções de linha de comando
    int opt;
    static struct option long_options[] = {
        {"tasks", 0, 0, 't'},
        {"view",  1, 0, 'v'},
        {"help",  0, 0, '?'},
        {0 ,0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "tv:?", long_options, NULL)) != EOF) {
        switch (opt) {
        case 't':
            useTasks = true;  // Ativar modo com tasks
            break;
        case 'v':
        {
            int viewIndex = atoi(optarg);
            // Mudar configurações de visualização
            if (viewIndex == 2) {
                // Zoom em uma região específica do fractal
                float scaleValue = .015f;   // Fator de zoom
                float shiftX = -.986f;      // Deslocamento em X
                float shiftY = .30f;        // Deslocamento em Y
                scaleAndShift(x0, x1, y0, y1, scaleValue, shiftX, shiftY);
            } else if (viewIndex > 1) {
                fprintf(stderr, "Invalid view index\n");
                return 1;
            }
            break;
        }
        case '?':
        default:
            usage(argv[0]);
            return 1;
        }
    }

    // Alocação de buffers para os resultados
    int *output_serial = new int[width*height];        // Versão serial
    int *output_ispc = new int[width*height];          // Versão ISPC (SIMD)
    int *output_ispc_tasks = new int[width*height];    // Versão ISPC com tasks

    // Inicialização do buffer serial
    for (unsigned int i = 0; i < width * height; ++i)
        output_serial[i] = 0;

    // --- EXECUÇÃO SERIAL (3 execuções para timing robusto) ---
    double minSerial = 1e30;  // Inicializa com valor muito alto
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        mandelbrotSerial(x0, y0, x1, y1, width, height, 0, height, maxIterations, output_serial);
        double endTime = CycleTimer::currentSeconds();
        minSerial = std::min(minSerial, endTime - startTime);  // Mantém o menor tempo
    }

    printf("[mandelbrot serial]:\t\t[%.3f] ms\n", minSerial * 1000);
    writePPMImage(output_serial, width, height, "mandelbrot-serial.ppm", maxIterations);

    // --- EXECUÇÃO ISPC (SIMD) ---
    for (unsigned int i = 0; i < width * height; ++i)
        output_ispc[i] = 0;

    double minISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        mandelbrot_ispc(x0, y0, x1, y1, width, height, maxIterations, output_ispc);
        double endTime = CycleTimer::currentSeconds();
        minISPC = std::min(minISPC, endTime - startTime);
    }

    printf("[mandelbrot ispc]:\t\t[%.3f] ms\n", minISPC * 1000);
    writePPMImage(output_ispc, width, height, "mandelbrot-ispc.ppm", maxIterations);

    // Verificação se os resultados são idênticos
    if (! verifyResult (output_serial, output_ispc, width, height)) {
        printf ("Error : ISPC output differs from sequential output\n");
        // Limpeza de memória
        delete[] output_serial;
        delete[] output_ispc;
        delete[] output_ispc_tasks;
        return 1;
    }

    // --- EXECUÇÃO ISPC COM TASKS (Multicore + SIMD) ---
    for (unsigned int i = 0; i < width * height; ++i) {
        output_ispc_tasks[i] = 0;
    }

    double minTaskISPC = 1e30;
    if (useTasks) {
        for (int i = 0; i < 3; ++i) {
            double startTime = CycleTimer::currentSeconds();
            mandelbrot_ispc_withtasks(x0, y0, x1, y1, width, height, maxIterations, output_ispc_tasks);
            double endTime = CycleTimer::currentSeconds();
            minTaskISPC = std::min(minTaskISPC, endTime - startTime);
        }

        printf("[mandelbrot multicore ispc]:\t[%.3f] ms\n", minTaskISPC * 1000);
        writePPMImage(output_ispc_tasks, width, height, "mandelbrot-task-ispc.ppm", maxIterations);

        // Verificação dos resultados com tasks
        if (! verifyResult (output_serial, output_ispc_tasks, width, height)) {
            printf ("Error : ISPC output differs from sequential output\n");
            return 1;
        }
    }

    // Cálculo e exibição dos speedups
    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial/minISPC);
    if (useTasks) {
        printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial/minTaskISPC);
    }

    // Limpeza de memória
    delete[] output_serial;
    delete[] output_ispc;
    delete[] output_ispc_tasks;

    return 0;
}
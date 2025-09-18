#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <chrono>
#include <thread>
#include <immintrin.h> // Para AVX2
#include <cmath>

// Configurações
const int WIDTH = 800;
const int HEIGHT = 800;
const int MAX_ITERATIONS = 1000;
const double X_MIN = -2.0;
const double X_MAX = 1.0;
const double Y_MIN = -1.5;
const double Y_MAX = 1.5;

// Estrutura para armazenar dados de tempo
struct TimingData {
    double serial_time;
    double simd_time;
    double threaded_time;
    double simd_threaded_time;
};

// Versão serial básica
void mandelbrot_serial(std::vector<int>& iterations, int start_y, int end_y) {
    double x_scale = (X_MAX - X_MIN) / WIDTH;
    double y_scale = (Y_MAX - Y_MIN) / HEIGHT;

    for (int y = start_y; y < end_y; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double zx = 0.0, zy = 0.0;
            double cx = X_MIN + x * x_scale;
            double cy = Y_MIN + y * y_scale;
            
            int iter = 0;
            while (zx * zx + zy * zy < 4.0 && iter < MAX_ITERATIONS) {
                double temp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = temp;
                iter++;
            }
            
            iterations[y * WIDTH + x] = iter;
        }
    }
}

// Versão com AVX2 (SIMD)
void mandelbrot_simd(std::vector<int>& iterations, int start_y, int end_y) {
    double x_scale = (X_MAX - X_MIN) / WIDTH;
    double y_scale = (Y_MAX - Y_MIN) / HEIGHT;

    for (int y = start_y; y < end_y; y++) {
        double cy = Y_MIN + y * y_scale;
        
        for (int x = 0; x < WIDTH; x += 4) {
            if (x + 3 >= WIDTH) continue; // Evitar acesso fora dos limites
            
            // Preparar 4 pontos em paralelo
            __m256d zx = _mm256_setzero_pd();
            __m256d zy = _mm256_setzero_pd();
            
            double cx_vals[4] = {
                X_MIN + (x) * x_scale,
                X_MIN + (x+1) * x_scale,
                X_MIN + (x+2) * x_scale,
                X_MIN + (x+3) * x_scale
            };
            __m256d cx = _mm256_loadu_pd(cx_vals);
            __m256d const_cy = _mm256_set1_pd(cy);
            
            __m256i iters = _mm256_setzero_si256();
            __m256i ones = _mm256_set1_epi64x(1);
            __m256i max_iters = _mm256_set1_epi64x(MAX_ITERATIONS);
            
            for (int i = 0; i < MAX_ITERATIONS; i++) {
                // Calcular zx^2 e zy^2
                __m256d zx2 = _mm256_mul_pd(zx, zx);
                __m256d zy2 = _mm256_mul_pd(zy, zy);
                
                // Verificar condição de escape
                __m256d mag2 = _mm256_add_pd(zx2, zy2);
                __m256d escape_mask = _mm256_cmp_pd(mag2, _mm256_set1_pd(4.0), _CMP_LT_OQ);
                
                // Se todos escaparam, sair
                if (_mm256_movemask_pd(escape_mask) == 0) break;
                
                // Atualizar contadores de iteração
                __m256i mask = _mm256_castpd_si256(escape_mask);
                iters = _mm256_add_epi64(iters, _mm256_and_si256(mask, ones));
                
                // Calcular novo z
                __m256d new_zx = _mm256_add_pd(_mm256_sub_pd(zx2, zy2), cx);
                __m256d new_zy = _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd(2.0), 
                                           _mm256_mul_pd(zx, zy)), const_cy);
                
                zx = new_zx;
                zy = new_zy;
            }
            
            // Armazenar resultados
            int64_t result[4];
            _mm256_storeu_si256((__m256i*)result, iters);
            
            for (int i = 0; i < 4; i++) {
                if (x + i < WIDTH) {
                    iterations[y * WIDTH + x + i] = result[i];
                }
            }
        }
    }
}

// Função para processamento multi-thread
template<typename Func>
void process_threaded(std::vector<int>& iterations, Func func, int num_threads) {
    std::vector<std::thread> threads;
    int rows_per_thread = HEIGHT / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        int start_y = i * rows_per_thread;
        int end_y = (i == num_threads - 1) ? HEIGHT : start_y + rows_per_thread;
        
        threads.emplace_back([&, start_y, end_y]() {
            func(iterations, start_y, end_y);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Gerar imagem PPM
void save_ppm(const std::vector<int>& iterations, const std::string& filename) {
    std::ofstream file(filename);
    file << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n";
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int iter = iterations[y * WIDTH + x];
            
            // Mapear iterações para cores
            unsigned char r, g, b;
            if (iter == MAX_ITERATIONS) {
                r = g = b = 0; // Preto para pontos no conjunto
            } else {
                // Esquema de cores simples
                r = static_cast<unsigned char>((iter * 5) % 256);
                g = static_cast<unsigned char>((iter * 7) % 256);
                b = static_cast<unsigned char>((iter * 11) % 256);
            }
            
            file << static_cast<int>(r) << " " 
                 << static_cast<int>(g) << " " 
                 << static_cast<int>(b) << " ";
        }
        file << "\n";
    }
}

// Medir tempo de execução
template<typename Func>
double measure_time(Func func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

// Função principal
int main() {
    std::vector<int> iterations_serial(WIDTH * HEIGHT);
    std::vector<int> iterations_simd(WIDTH * HEIGHT);
    std::vector<int> iterations_threaded(WIDTH * HEIGHT);
    std::vector<int> iterations_simd_threaded(WIDTH * HEIGHT);
    
    TimingData timing;
    const int num_threads = std::thread::hardware_concurrency();
    
    std::cout << "Iniciando cálculo do Conjunto de Mandelbrot..." << std::endl;
    std::cout << "Resolução: " << WIDTH << "x" << HEIGHT << std::endl;
    std::cout << "Máximo de iterações: " << MAX_ITERATIONS << std::endl;
    std::cout << "Número de threads disponíveis: " << num_threads << std::endl;
    
    // Versão serial
    std::cout << "\nExecutando versão serial..." << std::endl;
    timing.serial_time = measure_time([&]() {
        mandelbrot_serial(iterations_serial, 0, HEIGHT);
    });
    std::cout << "Tempo serial: " << timing.serial_time << "s" << std::endl;
    
    // Versão SIMD
    std::cout << "\nExecutando versão SIMD (AVX2)..." << std::endl;
    timing.simd_time = measure_time([&]() {
        mandelbrot_simd(iterations_simd, 0, HEIGHT);
    });
    std::cout << "Tempo SIMD: " << timing.simd_time << "s" << std::endl;
    
    // Versão multi-thread
    std::cout << "\nExecutando versão multi-thread (" << num_threads << " threads)..." << std::endl;
    timing.threaded_time = measure_time([&]() {
        process_threaded(iterations_threaded, mandelbrot_serial, num_threads);
    });
    std::cout << "Tempo multi-thread: " << timing.threaded_time << "s" << std::endl;
    
    // Versão SIMD + multi-thread
    std::cout << "\nExecutando versão SIMD + multi-thread..." << std::endl;
    timing.simd_threaded_time = measure_time([&]() {
        process_threaded(iterations_simd_threaded, mandelbrot_simd, num_threads);
    });
    std::cout << "Tempo SIMD + multi-thread: " << timing.simd_threaded_time << "s" << std::endl;
    
    // Calcular speedups
    double speedup_simd = timing.serial_time / timing.simd_time;
    double speedup_threaded = timing.serial_time / timing.threaded_time;
    double speedup_simd_threaded = timing.serial_time / timing.simd_threaded_time;
    
    std::cout << "\n=== RESULTADOS ===" << std::endl;
    std::cout << "Speedup SIMD: " << speedup_simd << "x" << std::endl;
    std::cout << "Speedup Multi-thread: " << speedup_threaded << "x" << std::endl;
    std::cout << "Speedup SIMD + Multi-thread: " << speedup_simd_threaded << "x" << std::endl;
    std::cout << "Eficiência paralela: " << (speedup_simd_threaded / num_threads) * 100 << "%" << std::endl;
    
    // Salvar imagens
    std::cout << "\nSalvando imagens..." << std::endl;
    save_ppm(iterations_serial, "mandelbrot_serial.ppm");
    save_ppm(iterations_simd, "mandelbrot_simd.ppm");
    save_ppm(iterations_threaded, "mandelbrot_threaded.ppm");
    save_ppm(iterations_simd_threaded, "mandelbrot_simd_threaded.ppm");
    
    std::cout << "Imagens salvas como mandelbrot_*.ppm" << std::endl;
    
    return 0;
}
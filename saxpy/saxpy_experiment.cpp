#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <immintrin.h>
#include <cmath>
#include <algorithm>
#include <numeric>

// Configurações
const size_t VECTOR_SIZE = 100000000; // 100 milhões de elementos
const int NUM_TRIALS = 10;
const int NUM_THREADS = std::thread::hardware_concurrency();
const float ALPHA = 2.5f; // Valor constante para o saxpy

// Estrutura para resultados
struct BenchmarkResult {
    double serial_time;
    double simd_time;
    double threaded_time;
    double simd_threaded_time;
    double bandwidth_serial;    // GB/s
    double bandwidth_simd;      // GB/s
    double bandwidth_threaded;  // GB/s
    double bandwidth_simd_threaded; // GB/s
    double speedup_simd;
    double speedup_threaded;
    double speedup_simd_threaded;
    double efficiency_simd;
    double efficiency_threaded;
};

// Gerar vetores de dados aleatórios
void generate_data(std::vector<float>& x, std::vector<float>& y, size_t size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1000.0f, 1000.0f);
    
    for (size_t i = 0; i < size; ++i) {
        x[i] = dis(gen);
        y[i] = dis(gen);
    }
}

// Verificar resultados (deve ser y = alpha * x + y)
bool verify_results(const std::vector<float>& x, const std::vector<float>& y, 
                   const std::vector<float>& result, float alpha, float tolerance = 1e-6f) {
    for (size_t i = 0; i < x.size(); ++i) {
        float expected = alpha * x[i] + y[i];
        if (std::abs(result[i] - expected) > tolerance) {
            std::cout << "Erro na posição " << i << ": esperado " << expected 
                      << ", obtido " << result[i] << std::endl;
            return false;
        }
    }
    return true;
}

// SAXPY serial (implementação de referência)
void saxpy_serial(float alpha, const std::vector<float>& x, std::vector<float>& y) {
    for (size_t i = 0; i < x.size(); ++i) {
        y[i] = alpha * x[i] + y[i];
    }
}

// SAXPY com SIMD (AVX2)
void saxpy_simd(float alpha, const std::vector<float>& x, std::vector<float>& y) {
    const size_t size = x.size();
    const size_t simd_size = size - (size % 8); // AVX2 processa 8 floats por vez
    
    __m256 alpha_vec = _mm256_set1_ps(alpha);
    
    // Processar blocos de 8 elementos
    for (size_t i = 0; i < simd_size; i += 8) {
        __m256 x_vec = _mm256_loadu_ps(&x[i]);
        __m256 y_vec = _mm256_loadu_ps(&y[i]);
        
        // y = alpha * x + y
        __m256 result = _mm256_fmadd_ps(alpha_vec, x_vec, y_vec);
        _mm256_storeu_ps(&y[i], result);
    }
    
    // Processar elementos restantes serialmente
    for (size_t i = simd_size; i < size; ++i) {
        y[i] = alpha * x[i] + y[i];
    }
}

// SAXPY multi-thread
void saxpy_threaded(float alpha, const std::vector<float>& x, std::vector<float>& y, int num_threads) {
    std::vector<std::thread> threads;
    const size_t chunk_size = x.size() / num_threads;
    
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? x.size() : start + chunk_size;
        
        threads.emplace_back([&, start, end]() {
            for (size_t j = start; j < end; ++j) {
                y[j] = alpha * x[j] + y[j];
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// SAXPY SIMD + multi-thread
void saxpy_simd_threaded(float alpha, const std::vector<float>& x, std::vector<float>& y, int num_threads) {
    std::vector<std::thread> threads;
    const size_t total_size = x.size();
    const size_t chunk_size = (total_size + num_threads - 1) / num_threads;
    
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = std::min(start + chunk_size, total_size);
        
        threads.emplace_back([&, start, end]() {
            const size_t local_size = end - start;
            const size_t simd_size = local_size - (local_size % 8);
            
            __m256 alpha_vec = _mm256_set1_ps(alpha);
            
            for (size_t j = start; j < start + simd_size; j += 8) {
                __m256 x_vec = _mm256_loadu_ps(&x[j]);
                __m256 y_vec = _mm256_loadu_ps(&y[j]);
                
                __m256 result = _mm256_fmadd_ps(alpha_vec, x_vec, y_vec);
                _mm256_storeu_ps(&y[j], result);
            }
            
            for (size_t j = start + simd_size; j < end; ++j) {
                y[j] = alpha * x[j] + y[j];
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Medir tempo de execução e bandwidth
template<typename Func>
double measure_time_and_bandwidth(Func func, size_t data_size_bytes, double& bandwidth) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    
    double time_seconds = std::chrono::duration<double>(end - start).count();
    bandwidth = (data_size_bytes / (1024.0 * 1024.0 * 1024.0)) / time_seconds; // GB/s
    
    return time_seconds;
}

// Executar benchmark múltiplas vezes
template<typename Func>
BenchmarkResult run_benchmark(Func benchmark_func, const std::string& name) {
    std::vector<double> times;
    std::vector<double> bandwidths;
    
    for (int i = 0; i < NUM_TRIALS; ++i) {
        double bandwidth;
        double time = measure_time_and_bandwidth([&]() { benchmark_func(); }, 
                                               VECTOR_SIZE * sizeof(float) * 3, // x, y, e resultado
                                               bandwidth);
        times.push_back(time);
        bandwidths.push_back(bandwidth);
    }
    
    // Remover outliers (máximo e mínimo)
    auto min_max = std::minmax_element(times.begin(), times.end());
    times.erase(min_max.first);
    times.erase(min_max.second);
    
    min_max = std::minmax_element(bandwidths.begin(), bandwidths.end());
    bandwidths.erase(min_max.first);
    bandwidths.erase(min_max.second);
    
    BenchmarkResult result;
    result.serial_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    result.bandwidth_serial = std::accumulate(bandwidths.begin(), bandwidths.end(), 0.0) / bandwidths.size();
    
    return result;
}

// Executar experimento completo
void run_saxpy_experiment() {
    std::cout << "=== EXPERIMENTO SAXPY (Single-precision AX + Y) ===" << std::endl;
    std::cout << "Tamanho dos vetores: " << VECTOR_SIZE << " elementos" << std::endl;
    std::cout << "Total de dados: " << (VECTOR_SIZE * sizeof(float) * 3 / (1024.0 * 1024.0 * 1024.0)) 
              << " GB" << std::endl;
    std::cout << "Número de threads: " << NUM_THREADS << std::endl;
    std::cout << "Número de trials: " << NUM_TRIALS << std::endl;
    std::cout << "Alpha: " << ALPHA << std::endl;
    
    // Alocar memória
    std::vector<float> x(VECTOR_SIZE);
    std::vector<float> y(VECTOR_SIZE);
    std::vector<float> y_ref(VECTOR_SIZE); // Para verificação
    
    // Gerar dados
    std::cout << "Gerando dados..." << std::endl;
    generate_data(x, y, VECTOR_SIZE);
    y_ref = y; // Backup para verificação
    
    BenchmarkResult results;
    
    // Versão serial (referência)
    std::cout << "\nExecutando SAXPY serial..." << std::endl;
    auto y_serial = y;
    results.serial_time = measure_time_and_bandwidth(
        [&]() { saxpy_serial(ALPHA, x, y_serial); },
        VECTOR_SIZE * sizeof(float) * 3,
        results.bandwidth_serial
    );
    
    // Verificar resultado serial
    if (!verify_results(x, y_ref, y_serial, ALPHA)) {
        std::cout << "ERRO: Versão serial produziu resultado incorreto!" << std::endl;
        return;
    }
    
    // Versão SIMD
    std::cout << "Executando SAXPY SIMD..." << std::endl;
    auto y_simd = y;
    results.simd_time = measure_time_and_bandwidth(
        [&]() { saxpy_simd(ALPHA, x, y_simd); },
        VECTOR_SIZE * sizeof(float) * 3,
        results.bandwidth_simd
    );
    
    // Verificar resultado SIMD
    if (!verify_results(x, y_ref, y_simd, ALPHA)) {
        std::cout << "ERRO: Versão SIMD produziu resultado incorreto!" << std::endl;
        return;
    }
    
    // Versão multi-thread
    std::cout << "Executando SAXPY multi-thread..." << std::endl;
    auto y_threaded = y;
    results.threaded_time = measure_time_and_bandwidth(
        [&]() { saxpy_threaded(ALPHA, x, y_threaded, NUM_THREADS); },
        VECTOR_SIZE * sizeof(float) * 3,
        results.bandwidth_threaded
    );
    
    // Verificar resultado multi-thread
    if (!verify_results(x, y_ref, y_threaded, ALPHA)) {
        std::cout << "ERRO: Versão multi-thread produziu resultado incorreto!" << std::endl;
        return;
    }
    
    // Versão SIMD + multi-thread
    std::cout << "Executando SAXPY SIMD + multi-thread..." << std::endl;
    auto y_simd_threaded = y;
    results.simd_threaded_time = measure_time_and_bandwidth(
        [&]() { saxpy_simd_threaded(ALPHA, x, y_simd_threaded, NUM_THREADS); },
        VECTOR_SIZE * sizeof(float) * 3,
        results.bandwidth_simd_threaded
    );
    
    // Verificar resultado SIMD + multi-thread
    if (!verify_results(x, y_ref, y_simd_threaded, ALPHA)) {
        std::cout << "ERRO: Versão SIMD+multi-thread produziu resultado incorreto!" << std::endl;
        return;
    }
    
    // Calcular speedups e eficiências
    results.speedup_simd = results.serial_time / results.simd_time;
    results.speedup_threaded = results.serial_time / results.threaded_time;
    results.speedup_simd_threaded = results.serial_time / results.simd_threaded_time;
    
    results.efficiency_simd = (results.speedup_simd / 8.0) * 100.0; // AVX2 tem 8 lanes
    results.efficiency_threaded = (results.speedup_threaded / NUM_THREADS) * 100.0;
    
    // Exibir resultados
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "RESULTADOS DO EXPERIMENTO SAXPY" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "Serial:      " << results.serial_time << "s, " 
              << results.bandwidth_serial << " GB/s" << std::endl;
    
    std::cout << "SIMD:        " << results.simd_time << "s, " 
              << results.bandwidth_simd << " GB/s, "
              << "Speedup: " << results.speedup_simd << "x, "
              << "Eficiência: " << results.efficiency_simd << "%" << std::endl;
    
    std::cout << "Multi-thread:" << results.threaded_time << "s, " 
              << results.bandwidth_threaded << " GB/s, "
              << "Speedup: " << results.speedup_threaded << "x, "
              << "Eficiência: " << results.efficiency_threaded << "%" << std::endl;
    
    std::cout << "SIMD+Thread: " << results.simd_threaded_time << "s, " 
              << results.bandwidth_simd_threaded << " GB/s, "
              << "Speedup: " << results.speedup_simd_threaded << "x" << std::endl;
    
    std::cout << "\nANÁLISE DE BANDWIDTH:" << std::endl;
    std::cout << "Aumento de bandwidth SIMD: " << (results.bandwidth_simd / results.bandwidth_serial) << "x" << std::endl;
    std::cout << "Aumento de bandwidth Multi-thread: " << (results.bandwidth_threaded / results.bandwidth_serial) << "x" << std::endl;
    std::cout << "Aumento de bandwidth Combinado: " << (results.bandwidth_simd_threaded / results.bandwidth_serial) << "x" << std::endl;
    
    // Salvar resultados em CSV
    std::ofstream csv_file("saxpy_results.csv");
    csv_file << "Implementação,Tempo(s),Bandwidth(GB/s),Speedup,Eficiência(%)\n";
    csv_file << "Serial," << results.serial_time << "," << results.bandwidth_serial << ",1.0,100.0\n";
    csv_file << "SIMD," << results.simd_time << "," << results.bandwidth_simd << "," 
             << results.speedup_simd << "," << results.efficiency_simd << "\n";
    csv_file << "Multi-thread," << results.threaded_time << "," << results.bandwidth_threaded << "," 
             << results.speedup_threaded << "," << results.efficiency_threaded << "\n";
    csv_file << "SIMD+Multi-thread," << results.simd_threaded_time << "," << results.bandwidth_simd_threaded << "," 
             << results.speedup_simd_threaded << ",-\n";
    
    csv_file.close();
    std::cout << "\nResultados salvos em saxpy_results.csv" << std::endl;
    
    // Análise de escalabilidade
    std::cout << "\nANÁLISE DE ESCALABILIDADE:" << std::endl;
    std::cout << "Speedup teórico máximo SIMD (AVX2): 8x" << std::endl;
    std::cout << "Speedup teórico máximo Threading: " << NUM_THREADS << "x" << std::endl;
    std::cout << "Speedup teórico máximo combinado: " << (8 * NUM_THREADS) << "x" << std::endl;
    
    std::cout << "Speedup alcançado SIMD: " << results.speedup_simd << "x (" 
              << (results.speedup_simd / 8.0 * 100.0) << "% do teórico)" << std::endl;
    
    std::cout << "Speedup alcançado Threading: " << results.speedup_threaded << "x (" 
              << (results.speedup_threaded / NUM_THREADS * 100.0) << "% do teórico)" << std::endl;
    
    std::cout << "Speedup alcançado combinado: " << results.speedup_simd_threaded << "x (" 
              << (results.speedup_simd_threaded / (8.0 * NUM_THREADS) * 100.0) << "% do teórico)" << std::endl;
}

// Teste de escalabilidade com diferentes tamanhos de vetor
void run_scalability_test() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "TESTE DE ESCALABILIDADE COM DIFERENTES TAMANHOS" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::vector<size_t> sizes = {
        1000000,    // 1M
        10000000,   // 10M
        50000000,   // 50M
        100000000,  // 100M
        200000000   // 200M
    };
    
    std::ofstream scalability_file("saxpy_scalability.csv");
    scalability_file << "Tamanho,SerialTime,SIMDTime,ThreadedTime,SIMDThreadedTime,"
                     << "SerialBW,SIMDBW,ThreadedBW,SIMDThreadedBW\n";
    
    for (size_t size : sizes) {
        std::cout << "\nTestando tamanho: " << size << " elementos (" 
                  << (size * sizeof(float) * 3 / (1024.0 * 1024.0 * 1024.0)) << " GB)" << std::endl;
        
        std::vector<float> x(size);
        std::vector<float> y(size);
        generate_data(x, y, size);
        
        double time_serial, bw_serial;
        double time_simd, bw_simd;
        double time_threaded, bw_threaded;
        double time_simd_threaded, bw_simd_threaded;
        
        // Serial
        auto y_serial = y;
        time_serial = measure_time_and_bandwidth(
            [&]() { saxpy_serial(ALPHA, x, y_serial); },
            size * sizeof(float) * 3,
            bw_serial
        );
        
        // SIMD
        auto y_simd = y;
        time_simd = measure_time_and_bandwidth(
            [&]() { saxpy_simd(ALPHA, x, y_simd); },
            size * sizeof(float) * 3,
            bw_simd
        );
        
        // Multi-thread
        auto y_threaded = y;
        time_threaded = measure_time_and_bandwidth(
            [&]() { saxpy_threaded(ALPHA, x, y_threaded, NUM_THREADS); },
            size * sizeof(float) * 3,
            bw_threaded
        );
        
        // SIMD + Multi-thread
        auto y_simd_threaded = y;
        time_simd_threaded = measure_time_and_bandwidth(
            [&]() { saxpy_simd_threaded(ALPHA, x, y_simd_threaded, NUM_THREADS); },
            size * sizeof(float) * 3,
            bw_simd_threaded
        );
        
        scalability_file << size << ","
                         << time_serial << "," << time_simd << "," << time_threaded << "," << time_simd_threaded << ","
                         << bw_serial << "," << bw_simd << "," << bw_threaded << "," << bw_simd_threaded << "\n";
        
        std::cout << "  Serial: " << time_serial << "s, " << bw_serial << " GB/s" << std::endl;
        std::cout << "  SIMD: " << time_simd << "s, " << bw_simd << " GB/s" << std::endl;
        std::cout << "  Threaded: " << time_threaded << "s, " << bw_threaded << " GB/s" << std::endl;
        std::cout << "  SIMD+Threaded: " << time_simd_threaded << "s, " << bw_simd_threaded << " GB/s" << std::endl;
    }
    
    scalability_file.close();
    std::cout << "\nDados de escalabilidade salvos em saxpy_scalability.csv" << std::endl;
}

int main() {
    // Executar experimento principal
    run_saxpy_experiment();
    
    // Executar teste de escalabilidade
    run_scalability_test();
    
    return 0;
}
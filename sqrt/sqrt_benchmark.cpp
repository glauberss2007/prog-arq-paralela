#include <iostream>
#include <fstream>  // Adicionado este include
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <immintrin.h>
#include <cmath>
#include <algorithm>

// Configurações
const size_t ARRAY_SIZE = 20000000; // 20 milhões
const int NUM_TRIALS = 10;
const int NUM_THREADS = std::thread::hardware_concurrency();

// Estrutura para resultados
struct BenchmarkResult {
    double serial_time;
    double simd_time;
    double threaded_time;
    double simd_threaded_time;
    double speedup_simd;
    double speedup_threaded;
    double speedup_simd_threaded;
};

// Gerar array com diferentes distribuições
enum class DataDistribution {
    UNIFORM,        // Valores uniformemente distribuídos
    NORMAL,         // Distribuição normal
    EXPONENTIAL,    // Distribuição exponencial
    SPARSE,         // Muitos zeros/poucos valores altos
    SKEWED          // Distribuição assimétrica
};

std::vector<float> generate_data(DataDistribution distribution, size_t size) {
    std::vector<float> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    
    switch (distribution) {
        case DataDistribution::UNIFORM: {
            std::uniform_real_distribution<float> dis(0.0f, 1000.0f);
            for (size_t i = 0; i < size; ++i) {
                data[i] = dis(gen);
            }
            break;
        }
        case DataDistribution::NORMAL: {
            std::normal_distribution<float> dis(500.0f, 200.0f);
            for (size_t i = 0; i < size; ++i) {
                data[i] = std::abs(dis(gen)); // Valores positivos
            }
            break;
        }
        case DataDistribution::EXPONENTIAL: {
            std::exponential_distribution<float> dis(0.001f);
            for (size_t i = 0; i < size; ++i) {
                data[i] = dis(gen);
            }
            break;
        }
        case DataDistribution::SPARSE: {
            std::uniform_real_distribution<float> dis(0.0f, 1.0f);
            std::uniform_real_distribution<float> high_dis(100.0f, 10000.0f);
            for (size_t i = 0; i < size; ++i) {
                if (dis(gen) < 0.01f) { // 1% de valores altos
                    data[i] = high_dis(gen);
                } else {
                    data[i] = 0.0f;
                }
            }
            break;
        }
        case DataDistribution::SKEWED: {
            std::uniform_real_distribution<float> dis(0.0f, 1.0f);
            for (size_t i = 0; i < size; ++i) {
                data[i] = std::pow(dis(gen), 3.0f) * 1000.0f; // Distribuição assimétrica
            }
            break;
        }
    }
    
    return data;
}

// Versão serial usando std::sqrt
void sqrt_serial(const std::vector<float>& input, std::vector<float>& output) {
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = std::sqrt(input[i]);
    }
}

// Versão SIMD usando instruções AVX
void sqrt_simd(const std::vector<float>& input, std::vector<float>& output) {
    const size_t size = input.size();
    const size_t simd_size = size - (size % 8); // AVX processa 8 floats por vez
    
    // Processar blocos de 8 elementos
    for (size_t i = 0; i < simd_size; i += 8) {
        __m256 vec = _mm256_loadu_ps(&input[i]);
        __m256 result = _mm256_sqrt_ps(vec);
        _mm256_storeu_ps(&output[i], result);
    }
    
    // Processar elementos restantes serialmente
    for (size_t i = simd_size; i < size; ++i) {
        output[i] = std::sqrt(input[i]);
    }
}

// Função para processamento multi-thread
void sqrt_threaded(const std::vector<float>& input, std::vector<float>& output, int num_threads) {
    std::vector<std::thread> threads;
    const size_t chunk_size = input.size() / num_threads;
    
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? input.size() : start + chunk_size;
        
        threads.emplace_back([&, start, end]() {
            for (size_t j = start; j < end; ++j) {
                output[j] = std::sqrt(input[j]);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Versão SIMD + multi-thread
void sqrt_simd_threaded(const std::vector<float>& input, std::vector<float>& output, int num_threads) {
    std::vector<std::thread> threads;
    const size_t chunk_size = (input.size() + num_threads - 1) / num_threads;
    
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = std::min(start + chunk_size, input.size());
        
        threads.emplace_back([&, start, end]() {
            const size_t local_size = end - start;
            const size_t simd_size = local_size - (local_size % 8);
            
            for (size_t j = start; j < start + simd_size; j += 8) {
                __m256 vec = _mm256_loadu_ps(&input[j]);
                __m256 result = _mm256_sqrt_ps(vec);
                _mm256_storeu_ps(&output[j], result);
            }
            
            for (size_t j = start + simd_size; j < end; ++j) {
                output[j] = std::sqrt(input[j]);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Medir tempo de execução
template<typename Func>
double measure_time(Func func, int num_trials = NUM_TRIALS) {
    double total_time = 0.0;
    
    for (int i = 0; i < num_trials; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        total_time += std::chrono::duration<double>(end - start).count();
    }
    
    return total_time / num_trials;
}

// Verificar precisão dos resultados
double calculate_error(const std::vector<float>& ref, const std::vector<float>& test) {
    double total_error = 0.0;
    size_t count = 0;
    
    for (size_t i = 0; i < ref.size(); ++i) {
        if (ref[i] > 0.0f) { // Evitar divisão por zero
            double error = std::abs(ref[i] - test[i]) / ref[i];
            total_error += error;
            count++;
        }
    }
    
    return (count > 0) ? total_error / count : 0.0;
}

// Analisar estatísticas dos dados
void analyze_data(const std::vector<float>& data, const std::string& name) {
    float min_val = *std::min_element(data.begin(), data.end());
    float max_val = *std::max_element(data.begin(), data.end());
    
    // Calcular média
    double sum = 0.0;
    for (float val : data) {
        sum += val;
    }
    double mean = sum / data.size();
    
    // Calcular desvio padrão
    double variance = 0.0;
    for (float val : data) {
        variance += (val - mean) * (val - mean);
    }
    variance /= data.size();
    double stddev = std::sqrt(variance);
    
    // Calcular percentis
    std::vector<float> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());
    float p25 = sorted_data[data.size() * 0.25];
    float p50 = sorted_data[data.size() * 0.50];
    float p75 = sorted_data[data.size() * 0.75];
    float p95 = sorted_data[data.size() * 0.95];
    
    std::cout << "\n=== ESTATÍSTICAS " << name << " ===" << std::endl;
    std::cout << "Mínimo: " << min_val << std::endl;
    std::cout << "Máximo: " << max_val << std::endl;
    std::cout << "Média: " << mean << std::endl;
    std::cout << "Desvio padrão: " << stddev << std::endl;
    std::cout << "Percentil 25: " << p25 << std::endl;
    std::cout << "Mediana (50): " << p50 << std::endl;
    std::cout << "Percentil 75: " << p75 << std::endl;
    std::cout << "Percentil 95: " << p95 << std::endl;
    
    // Contar zeros
    size_t zero_count = std::count(data.begin(), data.end(), 0.0f);
    std::cout << "Zeros: " << zero_count << " (" << (zero_count * 100.0 / data.size()) << "%)" << std::endl;
}

// Executar benchmark para uma distribuição específica
BenchmarkResult run_benchmark(DataDistribution distribution) {
    std::cout << "Gerando dados com distribuição: ";
    switch (distribution) {
        case DataDistribution::UNIFORM: std::cout << "UNIFORM"; break;
        case DataDistribution::NORMAL: std::cout << "NORMAL"; break;
        case DataDistribution::EXPONENTIAL: std::cout << "EXPONENTIAL"; break;
        case DataDistribution::SPARSE: std::cout << "SPARSE"; break;
        case DataDistribution::SKEWED: std::cout << "SKEWED"; break;
    }
    std::cout << std::endl;
    
    auto input = generate_data(distribution, ARRAY_SIZE);
    std::vector<float> output_serial(ARRAY_SIZE);
    std::vector<float> output_simd(ARRAY_SIZE);
    std::vector<float> output_threaded(ARRAY_SIZE);
    std::vector<float> output_simd_threaded(ARRAY_SIZE);
    std::vector<float> output_reference(ARRAY_SIZE);
    
    BenchmarkResult result;
    
    // Calcular referência (std::sqrt mais preciso)
    std::cout << "Calculando referência..." << std::endl;
    sqrt_serial(input, output_reference);
    
    // Benchmark serial
    std::cout << "Executando versão serial..." << std::endl;
    result.serial_time = measure_time([&]() {
        sqrt_serial(input, output_serial);
    });
    
    // Benchmark SIMD
    std::cout << "Executando versão SIMD..." << std::endl;
    result.simd_time = measure_time([&]() {
        sqrt_simd(input, output_simd);
    });
    
    // Benchmark multi-thread
    std::cout << "Executando versão multi-thread..." << std::endl;
    result.threaded_time = measure_time([&]() {
        sqrt_threaded(input, output_threaded, NUM_THREADS);
    });
    
    // Benchmark SIMD + multi-thread
    std::cout << "Executando versão SIMD + multi-thread..." << std::endl;
    result.simd_threaded_time = measure_time([&]() {
        sqrt_simd_threaded(input, output_simd_threaded, NUM_THREADS);
    });
    
    // Calcular speedups
    result.speedup_simd = result.serial_time / result.simd_time;
    result.speedup_threaded = result.serial_time / result.threaded_time;
    result.speedup_simd_threaded = result.serial_time / result.simd_threaded_time;
    
    // Verificar erros
    double error_simd = calculate_error(output_reference, output_simd);
    double error_threaded = calculate_error(output_reference, output_threaded);
    double error_simd_threaded = calculate_error(output_reference, output_simd_threaded);
    
    std::cout << "Erro médio SIMD: " << error_simd * 100 << "%" << std::endl;
    std::cout << "Erro médio multi-thread: " << error_threaded * 100 << "%" << std::endl;
    std::cout << "Erro médio SIMD+threaded: " << error_simd_threaded * 100 << "%" << std::endl;
    
    return result;
}

int main() {
    std::cout << "=== BENCHMARK DE CÁLCULO DE RAÍZ QUADRADA ===" << std::endl;
    std::cout << "Tamanho do array: " << ARRAY_SIZE << " elementos" << std::endl;
    std::cout << "Número de threads: " << NUM_THREADS << std::endl;
    std::cout << "Número de trials: " << NUM_TRIALS << std::endl;
    
    // Gerar dados para análise
    std::vector<DataDistribution> distributions = {
        DataDistribution::UNIFORM,
        DataDistribution::NORMAL,
        DataDistribution::EXPONENTIAL,
        DataDistribution::SPARSE,
        DataDistribution::SKEWED
    };
    
    std::vector<std::string> dist_names = {
        "UNIFORM", "NORMAL", "EXPONENTIAL", "SPARSE", "SKEWED"
    };
    
    std::vector<BenchmarkResult> results;
    
    // Analisar estatísticas de cada distribuição
    for (int i = 0; i < distributions.size(); ++i) {
        auto data = generate_data(distributions[i], 100000); // Amostra menor para análise
        analyze_data(data, dist_names[i]);
    }
    
    // Executar benchmarks para cada distribuição
    for (int i = 0; i < distributions.size(); ++i) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "BENCHMARK PARA DISTRIBUIÇÃO: " << dist_names[i] << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        auto result = run_benchmark(distributions[i]);
        results.push_back(result);
        
        std::cout << "\nRESULTADOS:" << std::endl;
        std::cout << "Tempo serial: " << result.serial_time << "s" << std::endl;
        std::cout << "Tempo SIMD: " << result.simd_time << "s" << std::endl;
        std::cout << "Tempo multi-thread: " << result.threaded_time << "s" << std::endl;
        std::cout << "Tempo SIMD+multi-thread: " << result.simd_threaded_time << "s" << std::endl;
        std::cout << "Speedup SIMD: " << result.speedup_simd << "x" << std::endl;
        std::cout << "Speedup multi-thread: " << result.speedup_threaded << "x" << std::endl;
        std::cout << "Speedup SIMD+multi-thread: " << result.speedup_simd_threaded << "x" << std::endl;
    }
    
    // Salvar resultados em CSV
    std::ofstream csv_file("sqrt_benchmark_results.csv");
    csv_file << "Distribution,SerialTime,SimdTime,ThreadedTime,SimdThreadedTime,"
             << "SpeedupSimd,SpeedupThreaded,SpeedupSimdThreaded\n";
    
    for (int i = 0; i < results.size(); ++i) {
        csv_file << dist_names[i] << ","
                 << results[i].serial_time << ","
                 << results[i].simd_time << ","
                 << results[i].threaded_time << ","
                 << results[i].simd_threaded_time << ","
                 << results[i].speedup_simd << ","
                 << results[i].speedup_threaded << ","
                 << results[i].speedup_simd_threaded << "\n";
    }
    
    csv_file.close();
    std::cout << "\nResultados salvos em sqrt_benchmark_results.csv" << std::endl;
    
    return 0;
}
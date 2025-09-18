#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')  # Usar backend não-gráfico
import matplotlib.pyplot as plt
import numpy as np
import subprocess
import csv
import os

def run_single_experiment(width, height, max_iter):
    """Executa um único experimento e retorna os tempos"""
    print(f"Testando {width}x{height}, iterações: {max_iter}")
    
    # Executar o programa C++
    result = subprocess.run([
        './mandelbrot', 
        str(width), 
        str(height), 
        str(max_iter)
    ], capture_output=True, text=True)
    
    output = result.stdout
    print(output)
    
    # Extrair tempos do output
    times = {}
    for line in output.split('\n'):
        if 'Tempo serial:' in line:
            times['serial'] = float(line.split(':')[1].replace('s', '').strip())
        elif 'Tempo multi-thread:' in line:
            times['threaded'] = float(line.split(':')[1].replace('s', '').strip())
    
    return times

def run_experiments():
    """Executa todos os experimentos"""
    resolutions = [400, 800, 1200]
    iterations = [500, 1000, 2000]
    
    results = []
    
    for res in resolutions:
        for max_iter in iterations:
            times = run_single_experiment(res, res, max_iter)
            
            if 'serial' in times and 'threaded' in times:
                results.append({
                    'resolution': res,
                    'iterations': max_iter,
                    'serial_time': times['serial'],
                    'threaded_time': times['threaded'],
                    'speedup': times['serial'] / times['threaded']
                })
    
    return results

def save_results(results, filename='results.csv'):
    """Salva resultados em CSV"""
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['resolution', 'iterations', 'serial_time', 'threaded_time', 'speedup'])
        
        for r in results:
            writer.writerow([
                r['resolution'],
                r['iterations'],
                r['serial_time'],
                r['threaded_time'],
                r['speedup']
            ])

def plot_results(results, output_file='analysis_results.png'):
    """Gera gráficos dos resultados"""
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 12))
    
    # Dados organizados por resolução
    resolutions = sorted(set(r['resolution'] for r in results))
    colors = ['red', 'green', 'blue', 'orange', 'purple']
    
    # Gráfico 1: Speedup vs Iterações por Resolução
    for i, res in enumerate(resolutions):
        res_data = [r for r in results if r['resolution'] == res]
        iterations = [r['iterations'] for r in res_data]
        speedups = [r['speedup'] for r in res_data]
        
        ax1.plot(iterations, speedups, 'o-', color=colors[i], 
                label=f'{res}x{res}', linewidth=2, markersize=8)
    
    ax1.set_xlabel('Número de Iterações', fontsize=12)
    ax1.set_ylabel('Speedup', fontsize=12)
    ax1.set_title('Speedup vs Complexidade (Iterações) por Resolução', fontsize=14)
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    # Gráfico 2: Tempos de execução
    serial_times = [r['serial_time'] for r in results]
    threaded_times = [r['threaded_time'] for r in results]
    experiment_labels = [f'{r["resolution"]}x{r["resolution"]}\n{r["iterations"]} iter' 
                        for r in results]
    
    x_pos = np.arange(len(results))
    width = 0.35
    
    ax2.bar(x_pos - width/2, serial_times, width, label='Serial', alpha=0.8)
    ax2.bar(x_pos + width/2, threaded_times, width, label='Multi-thread', alpha=0.8)
    
    ax2.set_xlabel('Configuração do Experimento', fontsize=12)
    ax2.set_ylabel('Tempo de Execução (s)', fontsize=12)
    ax2.set_title('Comparação de Tempos de Execução', fontsize=14)
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels(experiment_labels, rotation=45, ha='right')
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3)
    
    # Gráfico 3: Speedup médio por resolução
    avg_speedups = []
    for res in resolutions:
        res_data = [r for r in results if r['resolution'] == res]
        avg_speedups.append(np.mean([r['speedup'] for r in res_data]))
    
    ax3.bar([str(r) for r in resolutions], avg_speedups, 
            color=colors[:len(resolutions)], alpha=0.8)
    ax3.set_xlabel('Resolução', fontsize=12)
    ax3.set_ylabel('Speedup Médio', fontsize=12)
    ax3.set_title('Speedup Médio por Resolução', fontsize=14)
    ax3.grid(True, alpha=0.3)
    
    # Gráfico 4: Eficiência
    efficiencies = []
    for r in results:
        # Assumindo 8 cores, ajuste conforme seu hardware
        efficiency = (r['speedup'] / 8) * 100
        efficiencies.append(min(efficiency, 100))  # Limitar a 100%
    
    ax4.scatter([r['resolution'] for r in results], efficiencies, 
               c=[r['iterations'] for r in results], cmap='viridis', s=100)
    ax4.set_xlabel('Resolução', fontsize=12)
    ax4.set_ylabel('Eficiência (%)', fontsize=12)
    ax4.set_title('Eficiência vs Resolução e Iterações', fontsize=14)
    ax4.grid(True, alpha=0.3)
    
    # Adicionar barra de cores para iterações
    cbar = plt.colorbar(ax4.collections[0], ax=ax4)
    cbar.set_label('Iterações', fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Gráficos salvos em {output_file}")

def main():
    # Compilar o programa primeiro
    print("Compilando programa C++...")
    subprocess.run(['make', 'clean'])
    compile_result = subprocess.run(['make'], capture_output=True, text=True)
    
    if compile_result.returncode != 0:
        print("Erro na compilação:")
        print(compile_result.stderr)
        return
    
    print("Compilação bem-sucedida!")
    
    # Executar experimentos
    print("\nIniciando experimentos...")
    results = run_experiments()
    
    if results:
        # Salvar resultados
        save_results(results)
        print(f"Resultados salvos em results.csv")
        
        # Gerar gráficos
        plot_results(results)
        
        # Mostrar resumo
        print("\n=== RESUMO DOS RESULTADOS ===")
        for r in results:
            print(f"{r['resolution']}x{r['resolution']}, {r['iterations']} iter: "
                  f"Speedup = {r['speedup']:.2f}x")
        
        avg_speedup = np.mean([r['speedup'] for r in results])
        print(f"\nSpeedup médio: {avg_speedup:.2f}x")
        
    else:
        print("Nenhum resultado foi obtido.")

if __name__ == "__main__":
    main()
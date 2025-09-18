#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import subprocess
import csv

def run_benchmark():
    """Executa o benchmark C++"""
    print("Compilando programa...")
    subprocess.run(['make', 'clean'])
    compile_result = subprocess.run(['make'], capture_output=True, text=True)
    
    if compile_result.returncode != 0:
        print("Erro na compilação:")
        print(compile_result.stderr)
        return None
    
    print("Executando benchmark...")
    result = subprocess.run(['./sqrt_benchmark'], capture_output=True, text=True)
    print(result.stdout)
    
    if result.stderr:
        print("Erros:")
        print(result.stderr)
    
    # Ler resultados do CSV
    try:
        df = pd.read_csv('sqrt_benchmark_results.csv')
        return df
    except FileNotFoundError:
        print("Arquivo de resultados não encontrado")
        return None

def analyze_results(df):
    """Analisa e plota os resultados"""
    if df is None:
        print("Nenhum dado para analisar")
        return
    
    # Configurar estilo dos gráficos
    plt.style.use('seaborn-v0_8')
    sns.set_palette("husl")
    
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
    
    # Gráfico 1: Tempos de execução
    distributions = df['Distribution']
    time_columns = ['SerialTime', 'SimdTime', 'ThreadedTime', 'SimdThreadedTime']
    time_labels = ['Serial', 'SIMD', 'Multi-thread', 'SIMD+Multi-thread']
    
    x = np.arange(len(distributions))
    width = 0.2
    
    for i, col in enumerate(time_columns):
        ax1.bar(x + i * width, df[col], width, label=time_labels[i], alpha=0.8)
    
    ax1.set_xlabel('Distribuição de Dados', fontsize=12)
    ax1.set_ylabel('Tempo (s)', fontsize=12)
    ax1.set_title('Tempos de Execução por Distribuição', fontsize=14)
    ax1.set_xticks(x + 1.5 * width)
    ax1.set_xticklabels(distributions, rotation=45, ha='right')
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    # Gráfico 2: Speedups
    speedup_columns = ['SpeedupSimd', 'SpeedupThreaded', 'SpeedupSimdThreaded']
    speedup_labels = ['SIMD', 'Multi-thread', 'SIMD+Multi-thread']
    
    for i, col in enumerate(speedup_columns):
        ax2.plot(distributions, df[col], 'o-', linewidth=2, markersize=8, 
                label=speedup_labels[i])
    
    ax2.axhline(y=1, color='red', linestyle='--', alpha=0.5, label='Baseline (Serial)')
    ax2.set_xlabel('Distribuição de Dados', fontsize=12)
    ax2.set_ylabel('Speedup', fontsize=12)
    ax2.set_title('Speedup por Distribuição', fontsize=14)
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3)
    ax2.tick_params(axis='x', rotation=45)
    
    # Gráfico 3: Comparação de técnicas
    avg_speedups = [df[col].mean() for col in speedup_columns]
    ax3.bar(speedup_labels, avg_speedups, alpha=0.8)
    ax3.set_xlabel('Técnica', fontsize=12)
    ax3.set_ylabel('Speedup Médio', fontsize=12)
    ax3.set_title('Speedup Médio por Técnica', fontsize=14)
    ax3.grid(True, alpha=0.3)
    
    # Adicionar valores nas barras
    for i, v in enumerate(avg_speedups):
        ax3.text(i, v + 0.1, f'{v:.2f}x', ha='center', fontweight='bold')
    
    # Gráfico 4: Eficiência relativa
    efficiency_simd = (df['SpeedupSimd'] / df['SpeedupSimdThreaded']) * 100
    efficiency_threaded = (df['SpeedupThreaded'] / df['SpeedupSimdThreaded']) * 100
    
    x_pos = np.arange(len(distributions))
    width = 0.35
    
    ax4.bar(x_pos - width/2, efficiency_simd, width, label='Eficiência SIMD', alpha=0.8)
    ax4.bar(x_pos + width/2, efficiency_threaded, width, label='Eficiência Multi-thread', alpha=0.8)
    
    ax4.set_xlabel('Distribuição de Dados', fontsize=12)
    ax4.set_ylabel('Eficiência Relativa (%)', fontsize=12)
    ax4.set_title('Eficiência Relativa das Técnicas', fontsize=14)
    ax4.set_xticks(x_pos)
    ax4.set_xticklabels(distributions, rotation=45, ha='right')
    ax4.legend(fontsize=10)
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('sqrt_analysis.png', dpi=300, bbox_inches='tight')
    print("Gráficos salvos em sqrt_analysis.png")
    
    # Análise estatística
    print("\n=== ANÁLISE ESTATÍSTICA ===")
    print("Speedup médio SIMD:", df['SpeedupSimd'].mean())
    print("Speedup médio Multi-thread:", df['SpeedupThreaded'].mean())
    print("Speedup médio Combinado:", df['SpeedupSimdThreaded'].mean())
    
    # Correlação entre distribuição e speedup
    print("\nCorrelação com distribuição (ranking):")
    for col in speedup_columns:
        correlation = df[col].corr(pd.Series(range(len(df))))
        print(f"{col}: {correlation:.3f}")

def main():
    """Função principal"""
    print("=== ANÁLISE DE PERFORMANCE DE CÁLCULO DE RAÍZ QUADRADA ===")
    
    # Executar benchmark
    df = run_benchmark()
    
    if df is not None:
        # Analisar resultados
        analyze_results(df)
        
        # Mostrar tabela de resultados
        print("\n=== TABELA DE RESULTADOS ===")
        print(df.to_string(index=False))
        
        # Salvar análise detalhada
        with open('detailed_analysis.txt', 'w') as f:
            f.write("ANÁLISE DETALHADA DOS RESULTADOS\n")
            f.write("=" * 50 + "\n\n")
            
            for _, row in df.iterrows():
                f.write(f"DISTRIBUIÇÃO: {row['Distribution']}\n")
                f.write(f"  Speedup SIMD: {row['SpeedupSimd']:.2f}x\n")
                f.write(f"  Speedup Multi-thread: {row['SpeedupThreaded']:.2f}x\n")
                f.write(f"  Speedup Combinado: {row['SpeedupSimdThreaded']:.2f}x\n")
                f.write(f"  Eficiência SIMD: {(row['SpeedupSimd']/row['SpeedupSimdThreaded'])*100:.1f}%\n")
                f.write(f"  Eficiência Threading: {(row['SpeedupThreaded']/row['SpeedupSimdThreaded'])*100:.1f}%\n\n")
    
    print("Análise concluída!")

if __name__ == "__main__":
    main()
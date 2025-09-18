#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import subprocess

def run_experiment():
    """Executa o experimento C++"""
    print("Compilando programa...")
    subprocess.run(['make', 'clean'])
    compile_result = subprocess.run(['make'], capture_output=True, text=True)
    
    if compile_result.returncode != 0:
        print("Erro na compilação:")
        print(compile_result.stderr)
        return None, None
    
    print("Executando experimento SAXPY...")
    result = subprocess.run(['./saxpy_experiment'], capture_output=True, text=True)
    print(result.stdout)
    
    if result.stderr:
        print("Erros:")
        print(result.stderr)
    
    # Ler resultados
    try:
        df_main = pd.read_csv('saxpy_results.csv')
        df_scalability = pd.read_csv('saxpy_scalability.csv')
        return df_main, df_scalability
    except FileNotFoundError as e:
        print(f"Arquivos de resultados não encontrados: {e}")
        return None, None

def clean_numeric_columns(df):
    """Limpa colunas numéricas removendo caracteres não numéricos"""
    for col in df.columns:
        if col != 'Implementação':
            # Converter para string, remover caracteres não numéricos e converter para float
            df[col] = df[col].astype(str).str.replace('[^\d\.\-]', '', regex=True)
            df[col] = pd.to_numeric(df[col], errors='coerce')
    return df

def analyze_results(df_main, df_scalability):
    """Analisa e plota os resultados"""
    if df_main is None or df_scalability is None:
        print("Nenhum dado para analisar")
        return
    
    # Limpar colunas numéricas
    df_main = clean_numeric_columns(df_main)
    df_scalability = clean_numeric_columns(df_scalability)
    
    # Configurar plots
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
    
    # Gráfico 1: Speedups
    implementations = df_main['Implementação']
    speedups = df_main['Speedup']
    
    colors = ['lightblue', 'lightgreen', 'lightcoral', 'gold']
    bars = ax1.bar(implementations, speedups, color=colors, alpha=0.8)
    ax1.set_xlabel('Implementação')
    ax1.set_ylabel('Speedup')
    ax1.set_title('Speedup das Implementações SAXPY')
    ax1.tick_params(axis='x', rotation=45)
    ax1.grid(True, alpha=0.3)
    
    # Adicionar valores nas barras
    for bar, speedup in zip(bars, speedups):
        height = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2., height + 0.05,
                f'{speedup:.2f}x', ha='center', va='bottom')
    
    # Gráfico 2: Bandwidth
    bandwidths = df_main['Bandwidth(GB/s)']
    bars = ax2.bar(implementations, bandwidths, color=colors, alpha=0.8)
    ax2.set_xlabel('Implementação')
    ax2.set_ylabel('Bandwidth (GB/s)')
    ax2.set_title('Bandwidth de Memória das Implementações')
    ax2.tick_params(axis='x', rotation=45)
    ax2.grid(True, alpha=0.3)
    
    for bar, bw in zip(bars, bandwidths):
        height = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2., height + 0.1,
                f'{bw:.2f} GB/s', ha='center', va='bottom')
    
    # Gráfico 3: Escalabilidade de tempo
    sizes_gb = df_scalability['Tamanho'] * 4 * 3 / (1024**3)  # Convert to GB
    ax3.plot(sizes_gb, df_scalability['SerialTime'], 'o-', label='Serial', linewidth=2)
    ax3.plot(sizes_gb, df_scalability['SIMDTime'], 'o-', label='SIMD', linewidth=2)
    ax3.plot(sizes_gb, df_scalability['ThreadedTime'], 'o-', label='Multi-thread', linewidth=2)
    ax3.plot(sizes_gb, df_scalability['SIMDThreadedTime'], 'o-', label='SIMD+Threaded', linewidth=2)
    
    ax3.set_xlabel('Tamanho dos Dados (GB)')
    ax3.set_ylabel('Tempo de Execução (s)')
    ax3.set_title('Escalabilidade de Tempo por Tamanho de Dados')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    ax3.set_xscale('log')
    ax3.set_yscale('log')
    
    # Gráfico 4: Escalabilidade de bandwidth
    ax4.plot(sizes_gb, df_scalability['SerialBW'], 'o-', label='Serial', linewidth=2)
    ax4.plot(sizes_gb, df_scalability['SIMDBW'], 'o-', label='SIMD', linewidth=2)
    ax4.plot(sizes_gb, df_scalability['ThreadedBW'], 'o-', label='Multi-thread', linewidth=2)
    ax4.plot(sizes_gb, df_scalability['SIMDThreadedBW'], 'o-', label='SIMD+Threaded', linewidth=2)
    
    ax4.set_xlabel('Tamanho dos Dados (GB)')
    ax4.set_ylabel('Bandwidth (GB/s)')
    ax4.set_title('Escalabilidade de Bandwidth por Tamanho de Dados')
    ax4.legend()
    ax4.grid(True, alpha=0.3)
    ax4.set_xscale('log')
    
    plt.tight_layout()
    plt.savefig('saxpy_analysis.png', dpi=300, bbox_inches='tight')
    print("Gráficos salvos em saxpy_analysis.png")
    
    # Análise estatística
    print("\n=== ANÁLISE ESTATÍSTICA ===")
    print("Performance das implementações:")
    for _, row in df_main.iterrows():
        imp = row['Implementação']
        speedup = row['Speedup']
        bw = row['Bandwidth(GB/s)']
        print(f"{imp}: {speedup:.2f}x speedup, {bw:.2f} GB/s")
    
    # Análise de eficiência - verificar se a coluna existe
    if 'Eficiência(%)' in df_main.columns:
        simd_efficiency = df_main[df_main['Implementação'] == 'SIMD']['Eficiência(%)'].values
        threaded_efficiency = df_main[df_main['Implementação'] == 'Multi-thread']['Eficiência(%)'].values
        
        if len(simd_efficiency) > 0:
            print(f"\nEficiência SIMD: {simd_efficiency[0]:.1f}%")
        if len(threaded_efficiency) > 0:
            print(f"Eficiência Threading: {threaded_efficiency[0]:.1f}%")
    else:
        print("\nColuna de eficiência não encontrada nos resultados")
    
    # Análise de escalabilidade
    print("\nAnálise de escalabilidade:")
    max_bw = df_scalability[['SerialBW', 'SIMDBW', 'ThreadedBW', 'SIMDThreadedBW']].max().max()
    print(f"Bandwidth máximo alcançado: {max_bw:.2f} GB/s")
    
    # Calcular speedups médios da escalabilidade
    avg_speedup_simd = (df_scalability['SerialTime'] / df_scalability['SIMDTime']).mean()
    avg_speedup_threaded = (df_scalability['SerialTime'] / df_scalability['ThreadedTime']).mean()
    avg_speedup_combined = (df_scalability['SerialTime'] / df_scalability['SIMDThreadedTime']).mean()
    
    print(f"Speedup médio SIMD: {avg_speedup_simd:.2f}x")
    print(f"Speedup médio Threaded: {avg_speedup_threaded:.2f}x")
    print(f"Speedup médio Combinado: {avg_speedup_combined:.2f}x")
    
    # Salvar relatório
    with open('saxpy_analysis_report.txt', 'w') as f:
        f.write("RELATÓRIO DE ANÁLISE SAXPY\n")
        f.write("=" * 50 + "\n\n")
        
        f.write("RESULTADOS PRINCIPAIS:\n")
        for _, row in df_main.iterrows():
            imp = row['Implementação']
            speedup = row['Speedup']
            bw = row['Bandwidth(GB/s)']
            f.write(f"{imp}: Speedup={speedup:.2f}x, BW={bw:.2f} GB/s\n")
        
        f.write("\nANÁLISE DE ESCALABILIDADE:\n")
        f.write(f"Speedup médio SIMD: {avg_speedup_simd:.2f}x\n")
        f.write(f"Speedup médio Threaded: {avg_speedup_threaded:.2f}x\n")
        f.write(f"Speedup médio Combinado: {avg_speedup_combined:.2f}x\n")
        f.write(f"Bandwidth máximo: {max_bw:.2f} GB/s\n")
        
        f.write("\nOBSERVAÇÕES:\n")
        f.write("1. A operação SAXPY é limitada por bandwidth de memória\n")
        f.write("2. SIMD sozinho pode não mostrar grande melhoria devido ao bottleneck de memória\n")
        f.write("3. Multi-threading ajuda a saturar o bandwidth disponível\n")
        f.write("4. A combinação SIMD+threading maximiza o aproveitamento de recursos\n")
        f.write("5. Resultados podem variar com diferentes arquiteturas de memória\n")

def main():
    """Função principal"""
    print("=== ANÁLISE DO EXPERIMENTO SAXPY ===")
    
    # Executar experimento
    df_main, df_scalability = run_experiment()
    
    if df_main is not None and df_scalability is not None:
        # Analisar resultados
        analyze_results(df_main, df_scalability)
        
        # Mostrar resultados
        print("\n=== RESULTADOS DETALHADOS ===")
        print("Resultados principais:")
        print(df_main.to_string(index=False))
        
        print("\nDados de escalabilidade:")
        print(df_scalability.to_string(index=False))
    
    print("Análise concluída!")

if __name__ == "__main__":
    main()
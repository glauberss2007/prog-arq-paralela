#!/bin/bash
# Script de instalação para dependências de programação paralela

set -e  # Exit on error

echo "=== INSTALANDO DEPENDÊNCIAS PARA EXPERIMENTOS PARALELOS ==="

# Atualizar sistema
echo "Atualizando sistema..."
sudo apt update && sudo apt upgrade -y

# Instalar compiladores e ferramentas básicas
echo "Instalando compiladores e ferramentas..."
sudo apt install -y \
    g++ \
    make \
    cmake \
    build-essential \
    git \
    wget \
    curl \
    python3 \
    python3-pip \
    python3-dev \
    python3-venv

# Instalar bibliotecas de desenvolvimento paralelo
echo "Instalando bibliotecas de desenvolvimento paralelo..."
sudo apt install -y \
    libomp-dev \
    libtbb-dev \
    libnuma-dev \
    libhwloc-dev \
    libboost-all-dev

# Instalar ferramentas de análise de performance
echo "Instalando ferramentas de análise..."
sudo apt install -y \
    perf \
    htop \
    numactl \
    valgrind \
    sysstat \
    linux-tools-common \
    linux-tools-$(uname -r)

# Instalar ISPC
echo "Instalando ISPC..."
./install_ispc.sh

# Instalar pacotes Python
echo "Instalando pacotes Python..."
pip3 install --upgrade pip
pip3 install matplotlib numpy pandas seaborn scipy jupyter notebook

# Verificar instalações
echo "=== VERIFICAÇÃO DAS INSTALAÇÕES ==="
g++ --version
echo "CMake: $(cmake --version | head -n1)"
echo "Python: $(python3 --version)"
echo "Pip: $(pip3 --version)"
echo "ISPC: $(ispc --version || echo 'ISPC não instalado')"

echo "=== INSTALAÇÃO CONCLUÍDA ==="
echo "Dependências instaladas com sucesso!"
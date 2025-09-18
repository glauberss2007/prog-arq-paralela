#!/bin/bash
# Script de instalação do ISPC (Intel SPMD Program Compiler)

set -e  # Exit on error

echo "=== INSTALANDO ISPC ==="

# Versão do ISPC a ser instalada
ISPC_VERSION="1.21.1"
ARCH="linux"
TARGET_DIR="$HOME/ispc"

# Criar diretório de instalação
mkdir -p "$TARGET_DIR"
cd "$TARGET_DIR"

# Download do ISPC
echo "Baixando ISPC versão $ISPC_VERSION..."
wget "https://github.com/ispc/ispc/releases/download/v${ISPC_VERSION}/ispc-v${ISPC_VERSION}-${ARCH}.tar.gz"

# Extrair arquivo
echo "Extraindo..."
tar -xzf "ispc-v${ISPC_VERSION}-${ARCH}.tar.gz"

# Mover binário para diretório de instalação
mv "ispc-v${ISPC_VERSION}-${ARCH}/bin/ispc" .
chmod +x ispc

# Adicionar ao PATH
echo "Adicionando ISPC ao PATH..."
echo "export PATH=\"$TARGET_DIR:\$PATH\"" >> ~/.bashrc
export PATH="$TARGET_DIR:$PATH"

# Verificar instalação
if command -v ispc &> /dev/null; then
    echo "ISPC instalado com sucesso!"
    ispc --version
else
    echo "Erro na instalação do ISPC"
    exit 1
fi

# Limpar arquivos temporários
rm -rf "ispc-v${ISPC_VERSION}-${ARCH}" "ispc-v${ISPC_VERSION}-${ARCH}.tar.gz"

echo "=== INSTALAÇÃO DO ISPC CONCLUÍDA ==="
# Compilador C++ para arquitetura 64-bit
CXX=g++ -m64

# Flags de compilação:
# -I../common -Iobjs/      : Diretórios de include
# -O3                      : Otimização máxima
# -Wall                    : Todos os warnings
# -fPIC                    : Position Independent Code (para bibliotecas)
# -ffp-contract=off        : Desabilita contração de FP (mantém precisão)
CXXFLAGS=-I../common -Iobjs/ -O3 -Wall -fPIC -ffp-contract=off

# Compilador ISPC
ISPC=ispc

# Nota: requer suporte a AVX2
# Desabilitando FMA do AVX2 pois causa diferença no output comparado à referência no Mandelbrot
# Flags ISPC:
# -O3                      : Otimização máxima
# --target=avx2-i32x8      : Alvo AVX2 com vetores de 8 inteiros de 32 bits
# --arch=x86-64            : Arquitetura x86-64
# --opt=disable-fma        : Desabilita Fused Multiply-Add (para consistência numérica)
# --pic                    : Position Independent Code
ISPCFLAGS=-O3 --target=avx2-i32x8 --arch=x86-64 --opt=disable-fma --pic

# Nome do aplicativo final
APP_NAME=mandelbrot_ispc

# Diretório para objetos compilados
OBJDIR=objs

# Diretório comum com código compartilhado
COMMONDIR=../common

# Arquivo para manipulação de imagens PPM
PPM_CXX=$(COMMONDIR)/ppm.cpp
PPM_OBJ=$(addprefix $(OBJDIR)/, $(subst $(COMMONDIR)/,, $(PPM_CXX:.cpp=.o)))

# Sistema de tasks para paralelismo
TASKSYS_CXX=$(COMMONDIR)/tasksys.cpp
TASKSYS_LIB=-lpthread  # Biblioteca de threads POSIX
TASKSYS_OBJ=$(addprefix $(OBJDIR)/, $(subst $(COMMONDIR)/,, $(TASKSYS_CXX:.cpp=.o)))

# Alvo padrão
default: $(APP_NAME)

# Phony targets (não correspondem a arquivos)
.PHONY: dirs clean

# Criar diretório de objetos
dirs:
		/bin/mkdir -p $(OBJDIR)/

# Limpar arquivos compilados
clean:
		/bin/rm -rf $(OBJDIR) *.ppm *~ $(APP_NAME)

# Lista de todos os objetos necessários
OBJS=$(OBJDIR)/main.o $(OBJDIR)/mandelbrotSerial.o $(OBJDIR)/mandelbrot_ispc.o $(PPM_OBJ) $(TASKSYS_OBJ)

# Linkar o aplicativo final
$(APP_NAME): dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lm $(TASKSYS_LIB)

# Regra genérica para compilar arquivos .cpp
$(OBJDIR)/%.o: %.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

# Regra para compilar arquivos .cpp do diretório comum
$(OBJDIR)/%.o: $(COMMONDIR)/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

# Dependência: main.o precisa dos headers gerados
$(OBJDIR)/main.o: $(OBJDIR)/mandelbrot_ispc.h $(COMMONDIR)/CycleTimer.h

# Regra especial para compilar arquivos .ispc (gera .o e .h)
$(OBJDIR)/%_ispc.h $(OBJDIR)//%_ispc.o: %.ispc
		$(ISPC) $(ISPCFLAGS) $< -o $(OBJDIR)/$*_ispc.o -h $(OBJDIR)/$*_ispc.h
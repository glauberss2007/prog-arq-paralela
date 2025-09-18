/*

  15418 Spring 2012 note: This code was modified from example code
  originally provided by Intel.  To comply with Intel's open source
  licensing agreement, their copyright is retained below.

  -----------------------------------------------------------------

  Copyright (c) 2010-2011, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Função auxiliar que calcula se um ponto complexo (c_re, c_im) pertence ao conjunto de Mandelbrot
// Retorna o número de iterações até divergir (ou maxIterations se convergir)
static inline int mandel(float c_re, float c_im, int count)
{
    float z_re = c_re, z_im = c_im;  // Inicializa z = c (ponto no plano complexo)
    int i;
    
    // Itera a fórmula z = z² + c até divergir ou atingir o limite máximo
    for (i = 0; i < count; ++i) {

        // Critério de divergência: |z| > 2 (comparação com 4.f para evitar raiz quadrada)
        if (z_re * z_re + z_im * z_im > 4.f)
            break;

        // Calcula z² = (z_re² - z_im²) + (2*z_re*z_im)i
        float new_re = z_re*z_re - z_im*z_im;  // Parte real de z²
        float new_im = 2.f * z_re * z_im;      // Parte imaginária de z²
        
        // z = z² + c
        z_re = c_re + new_re;
        z_im = c_im + new_im;
    }

    return i;  // Retorna o número de iterações (usado para colorir o pixel)
}

//
// MandelbrotSerial --
//
// Compute an image visualizing the mandelbrot set.  The resulting
// array contains the number of iterations required before the complex
// number corresponding to a pixel could be rejected from the set.
//
// * x0, y0, x1, y1 describe the complex coordinates mapping
//   into the image viewport.
// * width, height describe the size of the output image
// * startRow, totalRows describe how much of the image to compute
void mandelbrotSerial(
    float x0, float y0, float x1, float y1,    // Região do plano complexo a renderizar
    int width, int height,                     // Dimensões da imagem em pixels
    int startRow, int totalRows,               // Linhas específicas a processar (para paralelismo)
    int maxIterations,                         // Número máximo de iterações por pixel
    int output[])                              // Array de saída (width * height)
{
    // Calcula o incremento (tamanho do passo) entre pixels vizinhos
    // no plano complexo
    float dx = (x1 - x0) / width;
    float dy = (y1 - y0) / height;

    // Calcula a última linha a processar
    int endRow = startRow + totalRows;

    // Loop externo: linhas da imagem (de cima para baixo)
    for (int j = startRow; j < endRow; j++) {
        // Loop interno: colunas da imagem (da esquerda para a direita)
        for (int i = 0; i < width; ++i) {
            // Converte coordenadas de pixel (i, j) para coordenadas complexas (x, y)
            float x = x0 + i * dx;
            float y = y0 + j * dy;

            // Calcula índice linear no array 1D de output
            int index = (j * width + i);
            
            // Calcula valor do pixel (número de iterações até divergência)
            output[index] = mandel(x, y, maxIterations);
        }
    }
}
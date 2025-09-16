# Programacao e Arquitetura Paralela 

  A Programação e Arquitetura Paralela representam um paradigma fundamental na computação moderna, focando-se na decomposição de problemas em tarefas simultâneas que são executadas em múltiplos núcleos de processamento ou unidades computacionais especializadas. Este campo estuda não apenas as técnicas de software para dividir e coordenar trabalho—como sincronização, comunicação e balanceamento de carga—mas também os princípios de hardware que permitem essa execução concorrente, incluindo hierarquias de memória, coerência de cache e a organização de processadores multi-core e aceleradores como GPUs. O domínio dessa área é essencial para extrair desempenho e eficiência de sistemas contemporâneos, onde o poder de processamento é conquistado pela exploração inteligente de recursos paralelos, e não mais por ganhos automáticos de clock ou ILP.

## Por que paralelismo e eficiência

  O paralelismo tornou-se imperativo devido ao esgotamento das estratégias tradicionais de melhoria de desempenho. O fim do scaling de frequência, limitado pela parede de energia (power wall), e a estagnação do paralelismo em nível de instrução (ILP) significaram que os ganhos de performance não são mais automáticos com cada nova geração de hardware. Para continuar obtendo ganhos significativos de velocidade, a única alternativa é explorar explicitamente a execução simultânea em múltiplos núcleos de processamento e unidades especializadas, o que demanda que os programas sejam conscientemente projetados para serem paralelos.

  No entanto, simplesmente executar tarefas em paralelo não é suficiente; a eficiência é crucial. Um programa paralelo pode ser mais rápido que sua versão sequencial, mas ainda assim fazer um uso pobre dos recursos computacionais, resultando em baixo speedup em relação ao número de processadores empregados. A verdadeira eficiência é alcançada ao entender e otimizar o movimento de dados, minimizando os custos de comunicação e sincronização e maximizando a localidade para reduzir os longos tempos de latência no acesso à memória. Portanto, dominar o paralelismo e a eficiência é essencial para liberar o potencial de desempenho escondido nas modernas arquiteturas multi-core e heterogéneas.

## Arquitetura de um processador mult-core moderno

  A arquitetura de um processador mult-core moderno é projetada para explorar múltiplas formas de paralelismo visando alto desempenho e eficiência. Em contraste com a era pré-multicore, onde os transistores eram  utilizados para melhorar a execução de um único fluxo de instruções (através de execução superscalar, previsão de desvio e caches maiores), os processadores modernos empregam esses recursos para integrar múltiplos núcleos de processamento independentes. Cada núcleo pode executar um fluxo de instruções distinto (thread), permitindo a execução simultânea de tarefas independentes. Além disso, para amplificar a capacidade computacional dentro de cada núcleo, é comum a existência de unidades SIMD (Single Instruction, Multiple Data), que permitem que uma única instrução opere sobre múltiplos dados simultaneamente, sendo crucial para workloads data-parallel. Essa abordagem multicore é complementada por hierarquias de memória cache complexas (L1, L2, L3) compartilhadas ou privadas, que reduzem a latência de acesso aos dados.

  Outro pilar fundamental dessa arquitetura é o multi-threading em hardware, que visa mitigar o impacto da alta latência de acesso à memória principal. Técnicas como o multi-threading entrelaçado (interleaved) ou simultâneo (SMT, como a Hyper-Threading da Intel) permitem que um núcleo físico mantenha o estado de execução de múltiplas threads e rapidamente alterne entre elas quando uma encontra uma operação de longa latência, como um acesso à memória. Isso mantém as unidades de execução (ALUs) ocupadas, melhorando a utilização geral do núcleo. Processadores orientados a throughput extremo, como GPUs, levam esse conceito ao limite, possuindo milhares de threads ativas concorrentemente (ex: 64 warps por SM no NVIDIA V100) e ALUs SIMD muito largas, exigindo que aplicações forneçam enormes quantidades de trabalho paralelo e coerente para que toda essa capacidade seja utilizada de forma eficiente.

  A arquitetura de um processador mult-core moderno também deve ser compreendida através das lentes de **latência** e **largura de banda (bandwidth)**, que são conceitos críticos para o desempenho paralelo. A latência refere-se ao tempo necessário para acessar um dado específico na memória, enquanto a largura de banda diz respeito à taxa total de transferência de dados que o sistema de memória pode sustentar. Assim como em uma estrada com múltiplas faixas, aumentar a largura de banda (adicionando mais "lanes" ou melhorando a eficiência do transporte) permite que mais dados sejam movidos simultaneamente, mas a latência individual de cada acesso permanece um desafio. Processadores modernos empregam hierarquias de cache complexas e técnicas de pré-busca (prefetching) para mitigar a latência, mas quando a demanda por dados excede a largura de banda disponível, a execução torna-se limitada por banda, tornando-se um gargalo significativo. Isso é particularmente relevante em GPUs e outros aceleradores de alto throughput, onde a capacidade de computação pode superar em muito a capacidade de alimentação de dados da memória.

  Além disso, a apresentação introduz o **ISPC (Intel SPMD Program Compiler)** como uma abstração de programação paralela que permite aos desenvolvedores expressar paralelismo de dados de forma intuitiva, usando construções como `foreach` para delegar ao compilador a distribuição de trabalho entre instâncias de programa. O modelo SPMD (Single Program, Multiple Data) abstrai a execução paralela como um "gang" de instâncias lógicas que operam sobre diferentes dados, enquanto a implementação subjacente utiliza instruções SIMD para eficiência computacional. No entanto, essa abstração não esconde completamente os detalhes de implementação; o programador ainda deve estar ciente de questões como coerência de fluxo de controle, acesso não contíguo à memória (que pode exigir operações de "gather" custosas) e a necessidade de operações de redução entre instâncias. Essa dualidade entre abstração e implementação é fundamental: enquanto o ISPC eleva o nível de expressividade, ele ainda exige um entendimento sólido do hardware para evitar armadilhas de desempenho e garantir correção, especialmente em operações com dependências ou acesso irregular à memória.

## O basico de Programação Paralela

  Os fundamentos da programação paralela envolvem um processo estruturado de transformar um problema sequencial em um programa que pode explorar a execução simultânea em múltiplos núcleos ou processadores. Este processo é dividido em quatro etapas principais: decomposição, atribuição (assignment), orquestração e mapeamento. A decomposição consiste em identificar e quebrar o problema em tarefas menores que podem ser executadas em paralelo, garantindo que dependências críticas sejam respeitadas. A Lei de Amdahl destaca que a presença de qualquer porção sequencial inevitável no código limita o speedup máximo alcançável, tornando a identificação de independência entre tarefas um passo crucial. Em seguida, a atribuição distribui essas tarefas entre "workers" (como threads ou instâncias de programa), buscando balancear a carga de trabalho e minimizar a comunicação. Essa distribuição pode ser estática (definida antecipadamente) ou dinâmica (ajustada durante a execução), e é frequentemente facilitada por abstrações de linguagem como o foreach no ISPC ou por sistemas de runtime com pools de threads.

A orquestração lida com a coordenação e a sincronização necessárias para garantir corretude quando as tarefas acessam dados compartilhados ou possuem dependências. Isso inclui o uso de primitivas como locks para exclusão mútua em seções críticas e barriers para dividir a execução em fases sincronizadas. Por fim, o mapeamento traduz as threads lógicas em recursos físicos de hardware, como núcleos de CPU ou unidades de execução em GPU, considerando fatores como localidade de dados e eficiência. A apresentação ilustra esses conceitos com um caso prático de um solver de grade 2D, mostrando como a reestruturação do algoritmo (usando coloração vermelho-preto) pode revelar mais paralelismo e como diferentes modelos de programação (como espaço de endereçamento compartilhado com threads SPMD ou paradigma data-parallel) oferecem trade-offs entre controle, abstração e desempenho. O entendimento desses fundamentos é essencial para desenvolver programas paralelos eficientes que realmente aproveitem o poder de hardware moderno.

## OpenMP

  O OpenMP (Open Multi-Processing) é um padrão amplamente utilizado para programação paralela em sistemas de memória compartilhada. Ele permite que desenvolvedores criem aplicações paralelas de forma incremental, adicionando diretivas de compilação (pragma) ao código C/C++ existente, sem a necessidade de reescrever completamente o programa. O modelo de programação do OpenMP é baseado em threads, onde um thread principal (master) forka uma equipe (team) de threads escravas (slaves) para executar blocos de código em paralelo, sincronizando-se e juntando-se ao término da execução. Essa abordagem simplifica a paralelização de loops e tarefas, tornando-a acessível mesmo para programadores com experiência limitada em concorrência.

  Um dos recursos mais poderosos do OpenMP é a diretiva parallel for, que automaticamente distribui iterações de loops entre threads, com opções de escalonamento (agendamento) flexíveis como static, dynamic, guided e runtime. Isso permite otimizar o balanceamento de carga, especialmente em casos onde as iterações têm custos computacionais variados. Além disso, o OpenMP oferece mecanismos robustos para sincronização e controle de escopo de variáveis. Cláusulas como reduction simplificam operações de redução (como somatórios) sem a necessidade de seções críticas manuais, enquanto critical, atomic e locks garantem exclusão mútua em acessos a dados compartilhados, prevenindo condições de corrida. O controle de escopo (com shared, private e default(none)) permite gerenciar explicitamente como as variáveis são compartilhadas entre threads, essencial para evitar erros sutis. Em resumo, o OpenMP é uma ferramenta versátil e eficiente para explorar o paralelismo em sistemas multicore, equilibrando simplicidade e controle para obter ganhos de desempenho significativos com mudanças mínimas no código-fonte.

## Otimizações de desempenho: distribuição de carga e escalonamento

A otimização de desempenho em programas paralelos envolve um equilíbrio delicado entre distribuição eficiente de carga e minimização de sobrecargas. O objetivo principal é garantir que todos os processadores estejam ocupados durante a maior parte do tempo de execução, evitando ociosidade e maximizando o aproveitamento dos recursos. Isso pode ser alcançado por meio de técnicas como atribuição estática — quando a carga de trabalho é previsível e uniforme — ou atribuição dinâmica — quando a carga é irregular ou imprevisível, utilizando filas de tarefas compartilhadas ou distribuídas com mecanismos como work stealing.

Além disso, é crucial ajustar o tamanho das tarefas (granularidade) para equilibrar a sobrecarga de sincronização e o potencial de paralelismo. Abordagens como fork-join e agendamento inteligente (por exemplo, executando tarefas longas primeiro) ajudam a reduzir ociosidade e melhorar o balanceamento. Sistemas como Cilk Plus implementam escalonamento com roubo de trabalho (work stealing), que promove boa localidade e baixa contenção, assegurando que que quebras de carga sejam tratadas de forma eficiente sem comprometer o desempenho geral.

## Otimizações de desempenho: comunicação e localidade

  A otimização de comunicação e localidade é fundamental para o desempenho em sistemas paralelos, pois evita gargalos relacionados ao acesso à memória e à transferência de dados. A localidade refere-se à organização dos acessos à memória de forma a maximizar a reutilização de dados já presentes em caches próximos ao processador, reduzindo assim a necessidade de comunicação com memórias mais lentas ou remotas. Técnicas como blocking (blocagem) e fusão de loops são exemplos clássicos para melhorar a localidade temporal, permitindo que os dados permaneçam em cache por mais tempo e reduzindo a quantidade de comunicação artifactual (aquela gerada por detalhes de implementação do sistema, como transferência de linhas de cache desnecessárias).

## Arquitetura de GPUs e Programação em CUDA

## Processamento data-parallel

## Distributed Data-Parallel Computing Using Spark

## Coerência de Cache

## Consistência de Memória

## Estratégias de sincronização de grão-fino: Locks

## Sincronização grão-fino e programação Lock-free

## Linguagens para Domínios Específicos (DSLs)

## Memória Transacional

## Processadores especializados (aceleradores)

## Sistemas para Processamento Paralelo de Grafos

## Programação para Hardware Especializado

## Avaliação de Redes Neurais Profundas

## Referencias

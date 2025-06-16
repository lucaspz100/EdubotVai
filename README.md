Edubot Maze Solver - Algoritmo de Tremaux
Visão Geral

Este projeto implementa um algoritmo inteligente para um robô Edubot resolver labirintos de forma autônoma. O robô explora o ambiente, mapeia o seu progresso e utiliza uma variação do Algoritmo de Tremaux para tomar decisões informadas, garantindo que todos os caminhos sejam explorados de forma eficiente até encontrar a saída.

O código foi desenvolvido em C++ e utiliza a biblioteca EdubotLib.hpp para interagir com os sensores e atuadores do robô.
Funcionalidades Principais

    Mapeamento em Tempo Real: O robô constrói um mapa interno do labirinto numa matriz 2D enquanto se move. O estado do mapa é impresso no console a cada passo, permitindo a visualização do progresso.

        S: Ponto de partida.

        R: Posição atual do robô.

        |: Interseção (junção com mais de uma saída).

        E: Saída do labirinto.

        1, 2...: Contagem de visitas em um corredor.

    Algoritmo de Decisão Inteligente (Tremaux): Em cada interseção, o robô segue um conjunto de regras para decidir o seu próximo movimento:

        Prioriza caminhos não visitados (contagem 0).

        Se todos os caminhos já foram visitados, escolhe o menos visitado.

        Em caso de empate, prefere virar à esquerda.

        Caminhos e interseções visitados duas vezes são marcados como "totalmente explorados".

    Lógica de "Beco sem Saída": Quando o robô encontra um beco sem saída, ele marca o caminho, gira 180 graus e retorna.

    Detecção de Saída: O robô deteta automaticamente que encontrou a saída do labirinto quando um número suficiente de sensores indica um grande espaço aberto.

    Rotinas de Calibração Avançada: Para combater erros de odometria, o robô executa rotinas de ajuste fino para se posicionar com precisão antes de fazer uma curva, usando as paredes como referência absoluta.

Estrutura do Código

O código é modular e está dividido em várias funções:

    main(): O ciclo principal que orquestra todas as operações: perceção, decisão, ação e mapeamento.

    decidir_e_executar_movimento(): O "cérebro" do robô. Contém toda a lógica de decisão baseada no mapa e nas regras do algoritmo de Tremaux.

    atualizar_mapeamento(): Responsável por atualizar a matriz mapa_visitas após cada movimento, incrementando a contagem de visitas ou marcando novas interseções.

    imprimir_mapa(): Gera a visualização do mapa no console a cada ciclo.

    calibrar_antes_de_virar(): Função unificada que tenta se alinhar com a parede frontal ou lateral antes de executar uma rotação, aumentando a precisão.

    obter_coordenadas_matriz(): Converte a posição física do robô (metros) para as coordenadas da matriz do mapa.

    snap_angle_to_grid(): Arredonda o ângulo do robô para o eixo de 90 graus mais próximo para estabilizar os cálculos de navegação.

Configurações Globais

Todas as principais variáveis que controlam o comportamento do robô estão definidas como constantes no início do ficheiro, permitindo um ajuste fácil:

const int LARGURA_MATRIZ = 15;                     // Largura da matriz que representa o mapa
const int ALTURA_MATRIZ = 15;                      // Altura da matriz
const double DISTANCIA_PAREDE = 0.40;              // Distância mínima para considerar um caminho lateral livre
const double DISTANCIA_PAREDE_FRENTE = 0.55;       // Distância mínima para considerar o caminho frontal livre
const double TAMANHO_CELULA = 0.55;                // Tamanho de cada célula do mapa em metros
const int MARCADOR_INTERSECAO = -1;                // Valor especial na matriz para identificar uma interseção
const int MARCADOR_SAIDA = -2;                     // Marcador para a saída
const double DISTANCIA_ALVO_PAREDE = 0.10;         // Distância exata que o robô tenta manter de uma parede em frente
const double DISTANCIA_ALVO_PAREDE_DIAGONAL = 0.15; // Distância alvo para a parede ao calibrar lateralmente
const int TEMPO_ROTACAO = 1500;                    // Tempo de espera em milissegundos após uma rotação para estabilizar
const double VELOCIDADE_PADRAO = 0.3;              // Velocidade padrão de movimento do robô

Como Usar

    Dependências: Este projeto requer a biblioteca EdubotLib.hpp e deve ser compilado num ambiente que a inclua.

    Compilação: Abra o projeto na Edubot IDE (ou ambiente compatível) e compile o ficheiro Main.cpp.

    Execução: Execute o programa com o robô (físico ou simulado) posicionado dentro de um labirinto. O robô começará a exploração de forma autônoma.

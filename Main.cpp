#include "libs/EdubotLib.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm> // Necessário para std::sort

// --- 1. Configurações Globais ---
const int LARGURA_MATRIZ = 10;
const int ALTURA_MATRIZ = 10;
const double DISTANCIA_PAREDE = 0.40;
const double DISTANCIA_PAREDE_FRENTE = 0.55;
const double TAMANHO_CELULA = 0.6;
const int MARCADOR_INTERSECAO = -1;
const double DISTANCIA_ALVO_PAREDE = 0.15; // Distância exata para parar em frente a uma parede
const int TEMPO_ROTACAO = 1000; // Tempo de espera em milissegundos após uma rotação

// Posição inicial do robô
const int START_X = LARGURA_MATRIZ / 2;
const int START_Y = ALTURA_MATRIZ / 2;

int mapa_visitas[LARGURA_MATRIZ][ALTURA_MATRIZ] = {0};

// --- 2. Declaração das Funções Auxiliares ---
struct Opcao {
    int rotacao; // -90, 0, 90
    int visitas;
};

void obter_coordenadas_matriz(EdubotLib* edubotLib, int& x, int& y);
void imprimir_mapa(int robo_x, int robo_y);
int decidir_e_executar_movimento(EdubotLib* edubotLib, int robo_x, int robo_y, bool frente_livre, bool esquerda_livre, bool direita_livre);
void atualizar_mapeamento(int& mapa_x, int& mapa_y, int& prev_mapa_x, int& prev_mapa_y, int acao_rotacao, bool frente_livre, bool esquerda_livre, bool direita_livre);


// --- 3. Programa Principal ---
int main() {
    EdubotLib* edubotLib = new EdubotLib();
    double sonar[7];
    
    int mapa_x = START_X;
    int mapa_y = START_Y;
    int prev_mapa_x = mapa_x;
    int prev_mapa_y = mapa_y;

    if (edubotLib->connect()) {
        edubotLib->sleepMilliseconds(500); 

        while (edubotLib->isConnected()) { 
            
            imprimir_mapa(mapa_x, mapa_y);
            edubotLib->stop(); 

            // 1. Percepção
            for(int i = 0; i < 7; i++) {
                sonar[i] = edubotLib->getSonar(i); 
            }
            
            // 2. VERIFICAÇÃO ÚNICA dos caminhos
            bool frente_livre = sonar[3] > DISTANCIA_PAREDE_FRENTE;
            bool esquerda_livre = sonar[6] > DISTANCIA_PAREDE;
            bool direita_livre = sonar[0] > DISTANCIA_PAREDE;
            
            // LINHA ADICIONADA PARA IMPRIMIR OS VALORES (1=livre, 0=bloqueado)
            std::cout << "Status dos Caminhos -> Frente: " << frente_livre 
                      << ", Esquerda: " << esquerda_livre 
                      << ", Direita: " << direita_livre << std::endl;

            // 3. Ação: Robô decide e se move
            int acao_rotacao = decidir_e_executar_movimento(edubotLib, mapa_x, mapa_y, frente_livre, esquerda_livre, direita_livre);

            // 4. Mapeamento: Atualiza a posição e o mapa
            obter_coordenadas_matriz(edubotLib, mapa_x, mapa_y);
            atualizar_mapeamento(mapa_x, mapa_y, prev_mapa_x, prev_mapa_y, acao_rotacao, frente_livre, esquerda_livre, direita_livre);
        }

        edubotLib->disconnect();
    } else {
        std::cout << "Could not connect on robot" << std::endl;
    }
    return 0;
}


// --- 4. Implementação das Funções ---

void imprimir_mapa(int robo_x, int robo_y) {
    // Comando para limpar o console (multiplataforma)
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif

    std::cout << "--- Mapa de Contagem e Intersecoes ---" << std::endl;
    std::cout << "(R=Robo, S=Partida, |=Intersecao, 1,2..=Visitas)" << std::endl;
    for (int y = 0; y < ALTURA_MATRIZ; ++y) {
        for (int x = 0; x < LARGURA_MATRIZ; ++x) {
            std::cout << std::left;
            if (y == robo_y && x == robo_x) {
                std::cout << std::setw(2) << "R";
            } else if (y == START_Y && x == START_X) {
                std::cout << std::setw(2) << "S";
            } else {
                int valor_celula = mapa_visitas[x][y];
                if (valor_celula == 0) std::cout << std::setw(2) << ".";
                else if (valor_celula == MARCADOR_INTERSECAO) std::cout << std::setw(2) << "|";
                else std::cout << std::setw(2) << valor_celula;
            }
        }
        std::cout << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
}

void obter_coordenadas_matriz(EdubotLib* edubotLib, int& x, int& y) {
    double x_real = edubotLib->getX(); 
    double y_real = edubotLib->getY(); 
    x = std::round((x_real / TAMANHO_CELULA) + (LARGURA_MATRIZ / 2));
    y = std::round(-(y_real / TAMANHO_CELULA) + (ALTURA_MATRIZ / 2));
}

void atualizar_mapeamento(int& mapa_x, int& mapa_y, int& prev_mapa_x, int& prev_mapa_y, int acao_rotacao, bool frente_livre, bool esquerda_livre, bool direita_livre) {
    if (mapa_x != prev_mapa_x || mapa_y != prev_mapa_y) {
        if (mapa_x >= 0 && mapa_x < LARGURA_MATRIZ && mapa_y >= 0 && mapa_y < ALTURA_MATRIZ) {
            
            int caminhos_livres = (frente_livre ? 1 : 0) + (esquerda_livre ? 1 : 0) + (direita_livre ? 1 : 0);

            if (caminhos_livres > 1 && mapa_visitas[mapa_x][mapa_y] == 0) {
                mapa_visitas[mapa_x][mapa_y] = MARCADOR_INTERSECAO;
            } else if (mapa_visitas[mapa_x][mapa_y] != MARCADOR_INTERSECAO) {
                mapa_visitas[mapa_x][mapa_y]++;
            }
        }
        prev_mapa_x = mapa_x;
        prev_mapa_y = mapa_y;
    } 
    else if (acao_rotacao == 180) {
        if (mapa_x >= 0 && mapa_x < LARGURA_MATRIZ && mapa_y >= 0 && mapa_y < ALTURA_MATRIZ &&
            mapa_visitas[mapa_x][mapa_y] != MARCADOR_INTERSECAO) {
            mapa_visitas[mapa_x][mapa_y]++;
        }
    }
}

int decidir_e_executar_movimento(EdubotLib* edubotLib, int robo_x, int robo_y, bool frente_livre, bool esquerda_livre, bool direita_livre) {
    std::vector<Opcao> opcoes_validas;

    if (frente_livre) {
        opcoes_validas.push_back({0, mapa_visitas[robo_x][robo_y-1]});
    }
    if (esquerda_livre) {
        opcoes_validas.push_back({90, mapa_visitas[robo_x-1][robo_y]});
    }
    if (direita_livre) {
        opcoes_validas.push_back({-90, mapa_visitas[robo_x+1][robo_y]});
    }

    // <<-- LÓGICA REFINADA PARA MARCAR INTERSEÇÕES EXPLORADAS -->>
    if (mapa_visitas[robo_x][robo_y] == MARCADOR_INTERSECAO) {
        bool todas_saidas_de_corredor_exploradas = true;
        int saidas_para_corredor = 0;

        if (opcoes_validas.empty()) {
            todas_saidas_de_corredor_exploradas = false; 
        }

        for (const auto& opcao : opcoes_validas) {
            // Considera apenas as saídas que NÃO são outras interseções
            if (opcao.visitas != MARCADOR_INTERSECAO) {
                saidas_para_corredor++;
                // Se encontrar um único caminho de corredor que não foi totalmente explorado,
                // a interseção atual ainda não está esgotada.
                if (opcao.visitas < 2) {
                    todas_saidas_de_corredor_exploradas = false;
                    break;
                }
            }
        }

        // A interseção só é marcada como '2' se tiver pelo menos uma saída para um corredor
        // e TODAS essas saídas de corredor já tiverem sido visitadas 2 vezes.
        if (saidas_para_corredor > 0 && todas_saidas_de_corredor_exploradas) {
            mapa_visitas[robo_x][robo_y] = 2;
        }
    }

    if (opcoes_validas.empty()) {
    	   edubotLib->sleepMilliseconds(100);
        edubotLib->rotate(180);
        edubotLib->sleepMilliseconds(TEMPO_ROTACAO);
        return 180;
    }

    std::sort(opcoes_validas.begin(), opcoes_validas.end(), [](const Opcao& a, const Opcao& b) {
        if (a.visitas != b.visitas) return a.visitas < b.visitas;
        return a.rotacao > b.rotacao; // Prefere Esquerda > Frente > Direita
    });

    Opcao melhor_opcao = opcoes_validas[0];

    if (melhor_opcao.rotacao != 0) {
    	edubotLib->sleepMilliseconds(100);
        edubotLib->rotate(melhor_opcao.rotacao);
        edubotLib->sleepMilliseconds(TEMPO_ROTACAO);
    }
    
    // Se o robô decidiu andar para frente
    double x_inicial = edubotLib->getX();
    double y_inicial = edubotLib->getY();
    double distancia_percorrida = 0.0;
    
    edubotLib->move(0.5); 

    // Loop de movimento principal com dupla condição de paragem
    while (distancia_percorrida < TAMANHO_CELULA && edubotLib->getSonar(3) > (DISTANCIA_ALVO_PAREDE + 0.05) ) {
        double dx = edubotLib->getX() - x_inicial;
        double dy = edubotLib->getY() - y_inicial;
        distancia_percorrida = std::sqrt(dx * dx + dy * dy);
        edubotLib->sleepMilliseconds(20); 
    }
    edubotLib->stop();

    // Se o robô parou perto de uma parede (e não porque completou a célula)
    if (edubotLib->getSonar(3) < TAMANHO_CELULA) {
        double distancia_atual = edubotLib->getSonar(3);
        double erro_distancia = distancia_atual - DISTANCIA_ALVO_PAREDE;
        double tolerancia = 0.01; // 1cm

        // Loop de ajuste fino para chegar à distância exata
        while(std::abs(erro_distancia) > tolerancia) {
            if (erro_distancia > 0) edubotLib->move(0.1); // Muito longe, avança devagar
            else edubotLib->move(-0.1); // Muito perto, recua devagar
            
            edubotLib->sleepMilliseconds(50);
            edubotLib->stop();
            
            distancia_atual = edubotLib->getSonar(3);
            erro_distancia = distancia_atual - DISTANCIA_ALVO_PAREDE;
        }
    }

    return melhor_opcao.rotacao;
}

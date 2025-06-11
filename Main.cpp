#include "libs/EdubotLib.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm> // Necessário para std::sort

// --- Adicionado para definir M_PI se não existir ---
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- 1. Configurações Globais ---
const int LARGURA_MATRIZ = 20;
const int ALTURA_MATRIZ = 20;
const double DISTANCIA_PAREDE = 0.20;
const double DISTANCIA_PAREDE_FRENTE = 0.35;
const double TAMANHO_CELULA = 0.35;
const int MARCADOR_INTERSECAO = -1;
const double DISTANCIA_ALVO_PAREDE = 0.06; // Distância exata para parar em frente a uma parede
const double DISTANCIA_ALVO_PAREDE_DIAGONAL = 0.15; // Distância exata para parar em frente a uma parede
const int TEMPO_ROTACAO = 1500; // Tempo de espera em milissegundos após uma rotação
const double VELOCIDADE = 0.3;

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
int decidir_e_executar_movimento(EdubotLib* edubotLib, int robo_x, int robo_y, bool frente_livre, bool esquerda_livre, bool direita_livre, double theta);
void atualizar_mapeamento(int& mapa_x, int& mapa_y, int& prev_mapa_x, int& prev_mapa_y, int acao_rotacao, bool frente_livre, bool esquerda_livre, bool direita_livre);
double snap_angle_to_grid(double angle);
bool verificar_saida_labirinto(const double sonar[]);
void calibrar_antes_de_virar(EdubotLib* edubotLib, int rotacao); // <<-- NOVA FUNÇÃO UNIFICADA


// --- 3. Programa Principal ---
int main() {
    EdubotLib* edubotLib = new EdubotLib();
    double sonar[7];
    double theta;
    
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
            theta = edubotLib->getTheta(); 

            // 2. Verificação de Saída
            if (verificar_saida_labirinto(sonar)) {
                std::cout << "LABIRINTO CONCLUIDO!" << std::endl;
                edubotLib->stop();
                break; 
            }
            
            // 3. Verificação dos Caminhos
            bool frente_livre = sonar[3] > DISTANCIA_PAREDE_FRENTE;
            bool esquerda_livre = sonar[6] > DISTANCIA_PAREDE;
            bool direita_livre = sonar[0] > DISTANCIA_PAREDE;
            
            std::cout << "Status dos Caminhos -> Frente: " << frente_livre 
                      << ", Esquerda: " << esquerda_livre 
                      << ", Direita: " << direita_livre << std::endl;

            // 4. Ação
            int acao_rotacao = decidir_e_executar_movimento(edubotLib, mapa_x, mapa_y, frente_livre, esquerda_livre, direita_livre, theta);
            
            // 5. Mapeamento
            obter_coordenadas_matriz(edubotLib, mapa_x, mapa_y);
            
            for(int i = 0; i < 7; i++) {
                sonar[i] = edubotLib->getSonar(i);
            }
            frente_livre = sonar[3] > DISTANCIA_PAREDE_FRENTE;
            esquerda_livre = sonar[6] > DISTANCIA_PAREDE;
            direita_livre = sonar[0] > DISTANCIA_PAREDE;

            atualizar_mapeamento(mapa_x, mapa_y, prev_mapa_x, prev_mapa_y, acao_rotacao, frente_livre, esquerda_livre, direita_livre);
        }

        edubotLib->disconnect();
    } else {
        std::cout << "Could not connect on robot" << std::endl;
    }
    return 0;
}


// --- 4. Implementação das Funções ---

bool verificar_saida_labirinto(const double sonar[]) {
    int sensores_livres = 0;
    for (int i = 0; i < 7; ++i) {
        if (sonar[i] > 2.0) {
            sensores_livres++;
        }
    }
    return sensores_livres > 3;
}

void imprimir_mapa(int robo_x, int robo_y) {
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

double snap_angle_to_grid(double angle) {
    if (angle >= -45.0 && angle < 45.0) return 0.0;
    if (angle >= 45.0 && angle < 135.0) return 90.0;
    if (angle >= -135.0 && angle < -45.0) return -90.0;
    return 180.0;
}

// <<-- NOVA FUNÇÃO DE CALIBRAÇÃO UNIFICADA -->>
void calibrar_antes_de_virar(EdubotLib* edubotLib, int rotacao) {
	edubotLib->sleepMilliseconds(100); 
	edubotLib->stop();
    double sonar_frente = edubotLib->getSonar(3);

    // Se houver parede na frente, calibra por ela
    if (sonar_frente < ((TAMANHO_CELULA*2)-DISTANCIA_ALVO_PAREDE) ) {
        int num_celulas = round((sonar_frente - DISTANCIA_ALVO_PAREDE) / TAMANHO_CELULA);
        if (num_celulas < 1){
        	num_celulas = 0;
        } else{
        		double distancia_alvo_dinamica = (num_celulas * TAMANHO_CELULA) + DISTANCIA_ALVO_PAREDE+0.05;
	        double erro_distancia = sonar_frente - distancia_alvo_dinamica;
	        double tolerancia = 0.01;
	
	        if (std::abs(erro_distancia) < 0.25) {
	            std::cout << "Calibrando para distancia frontal de " << distancia_alvo_dinamica << "m..." << std::endl;
	            while(std::abs(erro_distancia) > tolerancia) {
	                if (erro_distancia > 0) edubotLib->move(0.1);
	                else edubotLib->move(-0.1);
	                edubotLib->sleepMilliseconds(50);
	                edubotLib->stop();
	                sonar_frente = edubotLib->getSonar(3);
	                erro_distancia = sonar_frente - distancia_alvo_dinamica;
	            }
	            std::cout << "Distancia frontal calibrada." << std::endl;
	        }
        }

	        
    } 
    // Senão, tenta calibrar pela parede lateral
    else {
        std::cout << "Verificando necessidade de ajuste lateral..." << std::endl;
        int sonar_index = (rotacao == -90) ? 1 : 5;
        if (edubotLib->getSonar(sonar_index) < 0.5) {
            std::cout << "Ajustando distancia lateral para a curva..." << std::endl;
            const double angulo_sonar_rad = 60.0 * M_PI / 180.0;
            double distancia_perp = cos(angulo_sonar_rad) * edubotLib->getSonar(sonar_index);
            double erro_distancia = distancia_perp - DISTANCIA_ALVO_PAREDE_DIAGONAL;
            double tolerancia = 0.01;
            while(std::abs(erro_distancia) > tolerancia) {
                if (erro_distancia > 0) edubotLib->move(0.1);
                else edubotLib->move(-0.1);
                edubotLib->sleepMilliseconds(50);
                edubotLib->stop();
                distancia_perp = cos(angulo_sonar_rad) * edubotLib->getSonar(sonar_index);
                erro_distancia = distancia_perp - DISTANCIA_ALVO_PAREDE_DIAGONAL;
            }
            std::cout << "Distancia lateral para curva ajustada." << std::endl;
        }
    }
}


int decidir_e_executar_movimento(EdubotLib* edubotLib, int robo_x, int robo_y, bool frente_livre, bool esquerda_livre, bool direita_livre, double theta) {
    std::vector<Opcao> opcoes_validas;
    double angulo_grid = snap_angle_to_grid(theta);

    double angulo_rad = angulo_grid * M_PI / 180.0;
    
    int dx_frente = round(cos(angulo_rad));
    int dy_frente = round(sin(angulo_rad));
    dy_frente = -dy_frente;

    int x_frente = robo_x + dx_frente;
    int y_frente = robo_y + dy_frente;
    
    int x_direita = robo_x + dy_frente;
    int y_direita = robo_y - dx_frente;
    
    int x_esquerda = robo_x - dy_frente;
    int y_esquerda = robo_y + dx_frente;

    if (frente_livre) opcoes_validas.push_back({0, mapa_visitas[x_frente][y_frente]});
    if (esquerda_livre) opcoes_validas.push_back({90, mapa_visitas[x_esquerda][y_esquerda]});
    if (direita_livre) opcoes_validas.push_back({-90, mapa_visitas[x_direita][y_direita]});

    if (mapa_visitas[robo_x][robo_y] == MARCADOR_INTERSECAO) {
        bool todas_saidas_de_corredor_exploradas = true;
        int saidas_para_corredor = 0;
        if (opcoes_validas.empty()) todas_saidas_de_corredor_exploradas = true; 

        for (const auto& opcao : opcoes_validas) {
            if (opcao.visitas != MARCADOR_INTERSECAO) {
                saidas_para_corredor++;
                if (opcao.visitas < 2) {
                    todas_saidas_de_corredor_exploradas = false;
                    break;
                }
            }
        }
        if (saidas_para_corredor > 0 && todas_saidas_de_corredor_exploradas) {
            mapa_visitas[robo_x][robo_y] = 2;
        }
    }

    if (opcoes_validas.empty()) {
        edubotLib->rotate(180);
        edubotLib->sleepMilliseconds(TEMPO_ROTACAO);
        return 180;
    }

    std::sort(opcoes_validas.begin(), opcoes_validas.end(), [](const Opcao& a, const Opcao& b) {
        if (a.visitas != b.visitas) return a.visitas < b.visitas;
        return a.rotacao > b.rotacao;
    });

    Opcao melhor_opcao = opcoes_validas[0];

    // Se a melhor opção é virar...
    if (melhor_opcao.rotacao != 0) {
        // ...chama a rotina de calibração unificada.
        calibrar_antes_de_virar(edubotLib, melhor_opcao.rotacao);
        
        edubotLib->rotate(melhor_opcao.rotacao);
        edubotLib->sleepMilliseconds(TEMPO_ROTACAO);
    }
    
    double x_inicial = edubotLib->getX();
    double y_inicial = edubotLib->getY();
    double distancia_percorrida = 0.0;
    edubotLib->move(VELOCIDADE); 

    while (distancia_percorrida < TAMANHO_CELULA && edubotLib->getSonar(3) > (DISTANCIA_ALVO_PAREDE + 0.05)) {
        double dx = edubotLib->getX() - x_inicial;
        double dy = edubotLib->getY() - y_inicial;
        distancia_percorrida = std::sqrt(dx * dx + dy * dy);
        edubotLib->sleepMilliseconds(20); 
    }
    edubotLib->stop();

    if (edubotLib->getSonar(3) < TAMANHO_CELULA) {
        double distancia_atual = edubotLib->getSonar(3);
        double erro_distancia = distancia_atual - DISTANCIA_ALVO_PAREDE;
        double tolerancia = 0.01;
        while(std::abs(erro_distancia) > tolerancia) {
            if (erro_distancia > 0) edubotLib->move(0.1);
            else edubotLib->move(-0.1);
            edubotLib->sleepMilliseconds(50);
            edubotLib->stop();
            distancia_atual = edubotLib->getSonar(3);
            erro_distancia = distancia_atual - DISTANCIA_ALVO_PAREDE;
        }
    }

    return melhor_opcao.rotacao;
}

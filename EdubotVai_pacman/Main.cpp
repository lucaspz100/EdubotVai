#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <cmath> // Para a função abs() e cos()
#include "EdubotLib.hpp" 

// ------- CONFIGURAÇÕES DO LABIRINTO E DO ALGORITMO -------
const int LARGURA_LABIRINTO = 20;
const int ALTURA_LABIRINTO = 20;
const int TEMPO_POR_CELULA = 1310;

enum Direcao { NORTE, LESTE, SUL, OESTE };

const int SONAR_FRONTAL = 3, SONAR_DIREITA_EXTREMO = 6, SONAR_DIREITA_INTERNO = 5, SONAR_ESQUERDA_EXTREMO = 0, SONAR_ESQUERDA_INTERNO = 1;
const double DISTANCIA_PAREDE = 0.35;
const double DISTANCIA_SAIDA = 2.0;
const double TURNING_RANGE = 0.035; // Espaço mínimo (20cm) necessário para virar

// =================================================================
// CONSTANTES PARA OS CONTROLADORES PID
// =================================================================
// PID para ajuste de DISTÂNCIA frontal
const double DISTANCIA_ALVO_PAREDE = 0.04; 
const double KP_DIST = 1.5;  // Ganho Proporcional para distância
const double KI_DIST = 0.01; // Ganho Integral para distância
const double KD_DIST = 0.1;  // Ganho Derivativo para distância

const double PI = 3.14159265358979323846;
const double RAIO_SENSORES = 0.11; // 110mm convertidos para metros
const double DISTANCIA_ENTRE_SONARES = RAIO_SENSORES * (1.0 - std::cos(30.0 * PI / 180.0));

// A Célula armazena apenas as marcações para a navegação
struct Celula {
    int marcacoes[4] = {0, 0, 0, 0};
};

// --- FUNÇÕES DE CALIBRAÇÃO ---

void calibrarDistanciaPID(EdubotLib* edubotLib) {
    std::cout << "CALIBRANDO: Iniciando ajuste de distancia com PID..." << std::endl;
    double erro = 0.0, erro_anterior = 0.0, integral = 0.0;
    for(int i = 0; i < 50; ++i) {
        erro = edubotLib->getSonar(SONAR_FRONTAL) - DISTANCIA_ALVO_PAREDE;
        if (std::abs(erro) < 0.01) { edubotLib->stop(); return; }
        integral += erro;
        if (std::abs(integral * KI_DIST) > 0.1) integral = (integral > 0 ? 0.1/KI_DIST : -0.1/KI_DIST);
        double derivativo = erro - erro_anterior;
        double saida = (KP_DIST * erro) + (KI_DIST * integral) + (KD_DIST * derivativo);
        if (saida > 0.25) saida = 0.25; if (saida < -0.25) saida = -0.25;
        edubotLib->move(saida);
        erro_anterior = erro;
        edubotLib->sleepMilliseconds(30);
    }
    edubotLib->stop();
}

// =================================================================
// FUNÇÕES DE SEGURANÇA E MOVIMENTO ATUALIZADAS
// =================================================================
void recalibrar_antes_de_virar(EdubotLib* edubotLib) {
    std::cout << "RECALIBRANDO antes do giro..." << std::endl;
    // Calibra a distância se houver parede na frente
    if (edubotLib->getSonar(SONAR_FRONTAL) < DISTANCIA_PAREDE) {
        calibrarDistanciaPID(edubotLib);
    }
}

// ALTERAÇÃO: Função revertida para a versão mais simples e segura.
void garantir_espaco_de_giro(EdubotLib* edubotLib, int angulo_rotacao) {
    // Se vai virar à DIREITA (>0), precisa de espaço à ESQUERDA.
    // Se vai virar à ESQUERDA (<0), precisa de espaço à DIREITA.
    int sonar_a_verificar = (angulo_rotacao > 0) ? SONAR_ESQUERDA_EXTREMO : SONAR_DIREITA_EXTREMO;
    std::string nome_lado_verificado = (angulo_rotacao > 0) ? "esquerdo" : "direito";
    
    while (edubotLib->getSonar(sonar_a_verificar) < TURNING_RANGE) {
        std::cout << "Pouco espaco no lado " << nome_lado_verificado << " para virar! Recuando..." << std::endl;
        edubotLib->move(-0.2); // Move para trás lentamente
        edubotLib->sleepMilliseconds(400);
        edubotLib->stop();
        edubotLib->sleepMilliseconds(200); 
    }
}

void recuperarDeColisao(EdubotLib* edubotLib, int tipoColisao) {
    std::cout << "COLISAO DETECTADA! Iniciando manobra de recuperacao..." << std::endl;
    
    if (tipoColisao > 0) {
        std::cout << "Movendo para tras..." << std::endl;
        edubotLib->move(-0.2);
    } else {
        std::cout << "Movendo para frente..." << std::endl;
        edubotLib->move(0.2);
    }
    edubotLib->sleepMilliseconds(300); 
    edubotLib->stop();
    edubotLib->sleepMilliseconds(200);

    switch (tipoColisao) {
        case 1: std::cout << "Girando para a direita para desviar..." << std::endl; edubotLib->rotate(45); edubotLib->sleepMilliseconds(1000); break;
        case 2: std::cout << "Girando para a esquerda para desviar..." << std::endl; edubotLib->rotate(-45); edubotLib->sleepMilliseconds(1000); break;
        case 3: std::cout << "Girando 90 graus para a esquerda..." << std::endl; edubotLib->rotate(-90); edubotLib->sleepMilliseconds(1500); break;
        case -1: std::cout << "Girando para a direita para desviar..." << std::endl; edubotLib->rotate(45); edubotLib->sleepMilliseconds(1000); break;
        case -2: std::cout << "Girando para a esquerda para desviar..." << std::endl; edubotLib->rotate(-45); edubotLib->sleepMilliseconds(1000); break;
        case -3: std::cout << "Girando 90 graus para a esquerda..." << std::endl; edubotLib->rotate(-90); edubotLib->sleepMilliseconds(1500); break;
    }
    edubotLib->stop();
    std::cout << "Manobra de recuperacao concluida." << std::endl;
}

int verificarColisao(EdubotLib* edubotLib) {
    bool fe = edubotLib->getBumper(0);
    bool fd = edubotLib->getBumper(1);
    bool te = edubotLib->getBumper(2);
    bool td = edubotLib->getBumper(3);

    if (fe && fd) return 3; if (fe) return 1; if (fd) return 2;
    if (te && td) return -3; if (te) return -1; if (td) return -2;
    
    return 0;
}

// --- FUNÇÕES DE MOVIMENTO (Atualizadas para incluir recalibração pré-giro) ---

void virarDireita(EdubotLib* edubotLib, Direcao& dir) {
    std::cout << "MOVENDO: Virando 90 graus para a direita..." << std::endl;
    garantir_espaco_de_giro(edubotLib, 90);
    recalibrar_antes_de_virar(edubotLib);
    edubotLib->rotate(90);
    edubotLib->sleepMilliseconds(1500);
    edubotLib->stop();
    dir = (Direcao)((dir + 1) % 4);
}

void virarEsquerda(EdubotLib* edubotLib, Direcao& dir) {
    std::cout << "MOVENDO: Virando 90 graus para a esquerda..." << std::endl;
    garantir_espaco_de_giro(edubotLib, -90);
    recalibrar_antes_de_virar(edubotLib);
    edubotLib->rotate(-90);
    edubotLib->sleepMilliseconds(1500);
    edubotLib->stop();
    dir = (Direcao)((dir + 3) % 4);
}

void darMeiaVolta(EdubotLib* edubotLib, Direcao& dir) {
    std::cout << "MOVENDO: Dando meia volta (180 graus)..." << std::endl;
    garantir_espaco_de_giro(edubotLib, 90);
    garantir_espaco_de_giro(edubotLib, -90);
    recalibrar_antes_de_virar(edubotLib);
    edubotLib->rotate(180);
    edubotLib->sleepMilliseconds(2500);
    edubotLib->stop();
    dir = (Direcao)((dir + 2) % 4);
}

void moverFisicamenteParaFrente(EdubotLib* edubotLib) {
    std::cout << "MOVENDO: Para frente..." << std::endl;
    edubotLib->move(0.3);
    edubotLib->sleepMilliseconds(TEMPO_POR_CELULA);
    edubotLib->stop();
    edubotLib->sleepMilliseconds(200);
}

void moverVirtualmenteParaFrente(std::vector<std::vector<Celula>>& labirinto, int& x, int& y, Direcao dir) {
    Direcao direcaoOposta = (Direcao)((dir + 2) % 4);
    labirinto[x][y].marcacoes[dir]++;
    if (x >= 0 && x < LARGURA_LABIRINTO && y >= 0 && y < ALTURA_LABIRINTO) {
        if (dir == NORTE) y++; else if (dir == LESTE) x++; else if (dir == SUL) y--; else if (dir == OESTE) x--;
        if (x >= 0 && x < LARGURA_LABIRINTO && y >= 0 && y < ALTURA_LABIRINTO) {
            labirinto[x][y].marcacoes[direcaoOposta]++;
        }
    }
}

bool verificarSaidaDoLabirinto(EdubotLib* edubotLib) {
    int contadorSensoresAbertos = 0;
    const int TOTAL_SONARES = 7;
    for (int id = 0; id < TOTAL_SONARES; ++id) {
        if (edubotLib->getSonar(id) >= DISTANCIA_SAIDA) contadorSensoresAbertos++;
    }
    return contadorSensoresAbertos > 3;
}

int main() {
    EdubotLib* edubotLib = new EdubotLib();
    std::vector<std::vector<Celula>> labirinto(LARGURA_LABIRINTO, std::vector<Celula>(ALTURA_LABIRINTO));
    
    int roboX = LARGURA_LABIRINTO / 2;
    int roboY = ALTURA_LABIRINTO / 2;
    
    Direcao roboDir = LESTE;
    int contadorDeMovimentos = 0;

    if (edubotLib->connect()) {
        std::cout << "Conectado! Iniciando algoritmo de Trémaux com recuperacao de colisao." << std::endl;
        
        while (true) {
            std::cout << "\n--- CICLO " << contadorDeMovimentos + 1 << " ---" << std::endl;
            std::cout << "Estado Atual -> Pos: (" << roboX << ", " << roboY << "), Dir: " << roboDir << std::endl;
            
            edubotLib->sleepMilliseconds(200);

            int tipoColisao = verificarColisao(edubotLib);
            if (tipoColisao != 0) {
                recuperarDeColisao(edubotLib, tipoColisao);
                continue;
            }

            if (verificarSaidaDoLabirinto(edubotLib)) {
                std::cout << "\n\n*** SAIDA DO LABIRINTO ENCONTRADA! ***" << std::endl;
                break;
            }

            bool paredeFrente = edubotLib->getSonar(SONAR_FRONTAL) < DISTANCIA_PAREDE;
            bool paredeDireita = edubotLib->getSonar(SONAR_DIREITA_EXTREMO) < DISTANCIA_PAREDE;
            bool paredeEsquerda = edubotLib->getSonar(SONAR_ESQUERDA_EXTREMO) < DISTANCIA_PAREDE;

            if (paredeFrente) {
                calibrarDistanciaPID(edubotLib);
                paredeFrente = edubotLib->getSonar(SONAR_FRONTAL) < DISTANCIA_PAREDE;
            }

            Direcao dirFrente = roboDir;
            Direcao dirDireita = (Direcao)((roboDir + 1) % 4);
            Direcao dirEsquerda = (Direcao)((roboDir + 3) % 4);
            
            int marcaFrente = !paredeFrente ? labirinto[roboX][roboY].marcacoes[dirFrente] : -1;
            int marcaDireita = !paredeDireita ? labirinto[roboX][roboY].marcacoes[dirDireita] : -1;
            int marcaEsquerda = !paredeEsquerda ? labirinto[roboX][roboY].marcacoes[dirEsquerda] : -1;
            
            if (marcaDireita == 0) {
                virarDireita(edubotLib, roboDir);
            } else if (marcaFrente == 0) {
                // Segue em frente, não precisa de rotação
            } else if (marcaEsquerda == 0) {
                virarEsquerda(edubotLib, roboDir);
            } else if (marcaDireita == 1) {
                virarDireita(edubotLib, roboDir);
            } else if (marcaFrente == 1) {
                // Segue em frente
            } else if (marcaEsquerda == 1) {
                virarEsquerda(edubotLib, roboDir);
            } else {
                darMeiaVolta(edubotLib, roboDir);
            }
            
            moverVirtualmenteParaFrente(labirinto, roboX, roboY, roboDir);
            moverFisicamenteParaFrente(edubotLib);
            contadorDeMovimentos++;
        }

        std::cout << "\nExploracao concluida." << std::endl;
        edubotLib->disconnect();
    } else {
        std::cout << "Nao foi possivel conectar ao robo." << std::endl;
    }

    delete edubotLib;
    return 0;
}

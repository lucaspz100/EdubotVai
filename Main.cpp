#include <iostream>

#include <vector>

#include <iomanip>

#include <cstdlib>

#include <cmath> // Para a funÃ§Ã£o abs() e cos()

#include "EdubotLib.hpp" 



// ------- CONFIGURAÃ‡Ã•ES DO LABIRINTO E DO ALGORITMO -------

const int LARGURA_LABIRINTO = 20;

const int ALTURA_LABIRINTO = 20;

const int DISTANCIA_POR_CELULA = 3500;



enum Direção { NORTE, LESTE, SUL, OESTE };



const int SONAR_FRONTAL = 3, SONAR_DIREITA_EXTREMO = 6, SONAR_DIREITA_INTERNO = 5, SONAR_ESQUERDA_EXTREMO = 0, SONAR_ESQUERDA_INTERNO = 1;

const double DISTANCIA_PAREDE = 20.0;

const double DISTANCIA_SAIDA = 150.0;

const double TURNING_RANGE = 20.0; // EspaÃ§o mÃ­nimo (20cm) necessÃ¡rio para virar



// =================================================================

// CONSTANTES PARA OS CONTROLADORES PID

// =================================================================

// PID para ajuste de DISTÃ‚NCIA frontal

const double DISTANCIA_ALVO_PAREDE = 0.04; 

const double KP_DIST = 1.0;  // Ganho Proporcional para distÃ¢ncia

const double KI_DIST = 0.01; // Ganho Integral para distÃ¢ncia

const double KD_DIST = 0.1;  // Ganho Derivativo para distÃ¢ncia



const double PI = 3.14159265358979323846;

const double RAIO_SENSORES = 0.11; // 110mm convertidos para metros

const double DISTANCIA_ENTRE_SONARES = 0.0181;







const int ANG = 80.0; 








// A CÃ©lula armazena apenas as marcaÃ§Ãµes para a navegaÃ§Ã£o

struct Celula {

    int marcações[4] = {0, 0, 0, 0};

};



// --- FUNÃ‡Ã•ES DE CALIBRAÃ‡ÃƒO ---



void verificarSeBateu(EdubotLib* edubotLib) {

    int contadorSensoresAbertos = 0;

    const int TOTAL_BUMPERS = 7;

    for (int id = 0; id < TOTAL_BUMPERS; ++id) {

        if (edubotLib->getBumper(id)) contadorSensoresAbertos++;

    }

    if(contadorSensoresAbertos >1) {

    edubotLib->neutral();

    edubotLib->move(-0.1);

    edubotLib->sleepMilliseconds(400);

    }

}



void calibrarDistanciaPID(EdubotLib* edubotLib) {
    std::cout << "CALIBRANDO: Iniciando ajuste de distancia com PID..." << std::endl;
    double erro = 0.0, erro_anterior = 0.0, integral = 0.0;

    // É uma boa prática "zerar" o contador de tempo antes de iniciar o loop.
    // A primeira chamada pode ser descartada para garantir que o primeiro 'dt' seja válido.
    edubotLib->getEncoderCountDT(); 

    // Usar um timeout é mais robusto do que um número fixo de iterações.
    // Ex: Tentar o ajuste por no máximo 2 segundos.
    auto inicio = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - inicio).count() < 2) {
        
        // 1. Obter o tempo decorrido (delta time, dt)
        // ASSUMINDO que getEncoderCountDT() retorna o tempo em MILISSEGUNDOS.
        int dt_ms = edubotLib->getEncoderCountDT(); 
        
        // Se a biblioteca não retornar um valor válido, pule a iteração para evitar divisão por zero.
        if (dt_ms <= 0) {
            continue; 
        }

        // Converter para segundos para manter as unidades consistentes (m/s, etc.)
        double dt = static_cast<double>(dt_ms) / 1000.0;

        // ---- Início do cálculo do PID ----
        
        // Erro Proporcional (P)
        erro = edubotLib->getSonar(SONAR_FRONTAL) - DISTANCIA_ALVO_PAREDE;

        // Condição de parada se o robô estiver estável no alvo
        if (std::abs(erro) < 0.01) {
            edubotLib->stop();
            std::cout << "PID: Alvo de distância atingido." << std::endl;
            return;
        }

        // 2. Termo Integral (I), corrigido com dt
        // Acumula o erro ao longo do tempo.
        integral += erro * dt; 
        
        // Anti-windup: Limita o termo integral para evitar que ele cresça indefinidamente.
        if (std::abs(integral * KI_DIST) > 0.1) {
            integral = (integral > 0 ? 0.1 / KI_DIST : -0.1 / KI_DIST);
        }

        // 3. Termo Derivativo (D), corrigido com dt
        // Calcula a "velocidade" da mudança do erro.
        double derivativo = (erro - erro_anterior) / dt;

        // 4. Saída do PID
        // Soma das três componentes (Proporcional, Integral, Derivativa)
        double saida = (KP_DIST * erro) + (KI_DIST * integral) + (KD_DIST * derivativo);

        // Limita a velocidade máxima de saída para segurança
        if (saida > 0.25) saida = 0.25;
        if (saida < -0.25) saida = -0.25;

        // Aplica a velocidade calculada ao motor
        edubotLib->move(saida);

        // Guarda o erro atual para ser usado como "erro_anterior" na próxima iteração
        erro_anterior = erro;

        // NÃO é mais necessário usar sleep(), pois o cálculo agora é baseado no tempo real.
        // O loop rodará o mais rápido possível, e o 'dt' garantirá que o cálculo seja correto.
    }

    edubotLib->stop();
    std::cout << "PID: Timeout da calibração atingido." << std::endl;
}



// =================================================================

// FUNÃ‡Ã•ES DE SEGURANÃ‡A E MOVIMENTO ATUALIZADAS

// =================================================================

void recalibrar antes de virar(EdubotLib* edubotLib) {

    std::cout << "RECALIBRANDO antes do giro..." << std::endl;

    // Calibra a distÃ¢ncia se houver parede na frente

    if (edubotLib->getSonar(SONAR_FRONTAL) < DISTANCIA_PAREDE) {

        calibrarDistanciaPID(edubotLib);

    }

}



// ALTERAÃ‡ÃƒO: FunÃ§Ã£o revertida para a versão mais simples e segura.

void garantir espaço de giro(EdubotLib* edubotLib, int ângulo rotação) {

    // Se vai virar Ã  DIREITA (>0), precisa de espaÃ§o Ã  ESQUERDA.

    // Se vai virar Ã  ESQUERDA (<0), precisa de espaÃ§o Ã  DIREITA.

    int sonar a verificar = (ângulo rotação > 0) ? SONAR_ESQUERDA_EXTREMO : SONAR_DIREITA_EXTREMO;

    std::string nome lado verificado = (ângulo rotação > 0) ? "esquerdo" : "direito";

    

    while (edubotLib->getSonar(sonar a verificar) < TURNING_RANGE) {

        std::cout << "Pouco espaço no lado " << nome lado verificado << " para virar! Recuando..." << std::endl;

        edubotLib->rotate(-ângulo rotação/45);

        edubotLib->sleepMilliseconds(400);

        edubotLib->move(-0.2); // Move para trÃ¡s lentamente

        edubotLib->sleepMilliseconds(200);

        edubotLib->rotate(ângulo rotação/45);

        edubotLib->sleepMilliseconds(400);

        edubotLib->move(0.2); // Move para trÃ¡s lentamente

        edubotLib->sleepMilliseconds(200);

        edubotLib->stop();

        edubotLib->sleepMilliseconds(200);

    }

}





// --- FUNÃ‡Ã•ES DE MOVIMENTO (Atualizadas para incluir recalibraÃ§Ã£o prÃ©-giro) ---



void virarDireita(EdubotLib* edubotLib, Direção& dir) {

    std::cout << "MOVENDO: Virando 90 graus para a direita..." << std::endl;

    edubotLib->sleepMilliseconds(200);

    garantir espaço de giro(edubotLib, 90);

    recalibrar antes de virar(edubotLib);

    edubotLib->rotate(ANG);

    edubotLib->sleepMilliseconds(1500);

    edubotLib->stop();

    dir = (Direção)((dir + 1) % 4);

    std::cout << edubotLib->getSonar(6) << std::endl;

}



void virarEsquerda(EdubotLib* edubotLib, Direção& dir) {

    std::cout << "MOVENDO: Virando 90 graus para a esquerda..." << std::endl;

    edubotLib->sleepMilliseconds(200);

    garantir espaço de giro(edubotLib, -90);

    recalibrar antes de virar(edubotLib);

    edubotLib->rotate(-ANG);

    edubotLib->sleepMilliseconds(1500);

    edubotLib->stop();

    dir = (Direção)((dir + 3) % 4);

    std::cout << edubotLib->getSonar(0) << std::endl;

}



void darMeiaVolta(EdubotLib* edubotLib, Direção& dir) {

    std::cout << "MOVENDO: Dando meia volta (180 graus)..." << std::endl;

    garantir espaço de giro(edubotLib, 90);

    garantir espaço de giro(edubotLib, -90);

    recalibrar antes de virar(edubotLib);

    edubotLib->rotate(ANG * 2);

    edubotLib->sleepMilliseconds(2500);

    edubotLib->stop();

    dir = (Direção)((dir + 2) % 4);

    std::cout << edubotLib->getSonar(6) << std::endl;

    std::cout << edubotLib->getSonar(0) << std::endl;

    std::cout << edubotLib->getSonar(3) << std::endl;

}



void moverFisicamenteParaFrente(EdubotLib* edubotLib) {

	std::cout << "MOVENDO: Para frente por " << DISTANCIA_POR_CELULA << "m..." << std::endl;

	

	int encoder_esquerda_inicio = edubotLib->getEncoderCountLeft();

	int encoder_direita_inicio = edubotLib->getEncoderCountRight();

	double distancia percorrida = 0.0;

	

	

	edubotLib->move(0.1); // Inicia o movimento

	

	while(distancia percorrida <= DISTANCIA_POR_CELULA) {

	verificarSeBateu(edubotLib);

	int encoder_esquerda = edubotLib->getEncoderCountLeft();

	int encoder_direita = edubotLib->getEncoderCountRight();

	distancia percorrida = std::sqrt(std::pow(encoder_esquerda - encoder_esquerda_inicio, 2) + std::pow(encoder_direita - encoder_direita_inicio, 2));

	edubotLib->sleepMilliseconds(10); // Pequena pausa para não sobrecarregar

	}

	

	edubotLib->stop();

	std::cout << "MOVENDO: Movimento concluído." << std::endl;

	edubotLib->sleepMilliseconds(200);

}



void moverVirtualmenteParaFrente(std::vector<std::vector<Celula>>& labirinto, int& x, int& y, Direção dir) {

    Direção direcaoOposta = (Direção)((dir + 2) % 4);

    labirinto[x][y].marcações[dir]++;

    if (x >= 0 && x < LARGURA_LABIRINTO && y >= 0 && y < ALTURA_LABIRINTO) {

        if (dir == NORTE) y++; else if (dir == LESTE) x++; else if (dir == SUL) y--; else if (dir == OESTE) x--;

        if (x >= 0 && x < LARGURA_LABIRINTO && y >= 0 && y < ALTURA_LABIRINTO) {

            labirinto[x][y].marcações[direcaoOposta]++;

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

    

    Direção roboDir = LESTE;

    int contadorDeMovimentos = 0;



    if (edubotLib->connect()) {

        std::cout << "Conectado! Iniciando algoritmo de TrÃ©maux com recuperação de colisão." << std::endl;

        

        while (true) {

            std::cout << "\n--- CICLO " << contadorDeMovimentos + 1 << " ---" << std::endl;

            std::cout << "Estado Atual -> Pos: (" << roboX << ", " << roboY << "), Dir: " << roboDir << std::endl;

            

            edubotLib->sleepMilliseconds(200);

            





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



            Direção dirFrente = roboDir;

            Direção dirDireita = (Direção)((roboDir + 1) % 4);

            Direção dirEsquerda = (Direção)((roboDir + 3) % 4);

            

            int marcaFrente = !paredeFrente ? labirinto[roboX][roboY].marcações[dirFrente] : -1;

            int marcaDireita = !paredeDireita ? labirinto[roboX][roboY].marcações[dirDireita] : -1;

            int marcaEsquerda = !paredeEsquerda ? labirinto[roboX][roboY].marcações[dirEsquerda] : -1;

            

            if (marcaDireita == 0) {

                virarDireita(edubotLib, roboDir);

            } else if (marcaFrente == 0) {

                // Segue em frente, nÃ£o precisa de rotaÃ§Ã£o

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

		edubotLib->sleepMilliseconds(100);

            

        }



        std::cout << "\nExploracao concluída." << std::endl;

        edubotLib->disconnect();

    } else {

        std::cout << "Nao foi possível conectar ao robô." << std::endl;

    }



    delete edubotLib;

    return 0;

}
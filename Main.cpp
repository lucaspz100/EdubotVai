#include <iostream>
#include <vector>
#include <map>
#include <queue>    // Para BFS
#include <algorithm> // Para std::reverse (ao reconstruir caminho)
#include <utility>  // Para std::pair
#include <iostream>
#include <cmath>
#include "libs/EdubotLib.hpp"



#define linha 3
#define coluna 100
#define TAMANHO 0.5
double intersecao[linha][coluna]={0};


bool carregarLabirintoDeArquivo(const std::string& nomeArquivo, 
                               std::vector<std::vector<int>>& matrizLabirinto, 
                               double escala = 1.0) {
    std::vector<Parede> paredes;
    std::ifstream arquivo(nomeArquivo);

    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o ficheiro: " << nomeArquivo << std::endl;
        return false;
    }

    std::string linha;
    double maxX = 0.0, maxY = 0.0;
    bool primeiraLinhaDado = true; // Para ignorar o cabeçalho e pegar minX, minY iniciais

    // Ignorar a primeira linha (cabeçalho)
    if (std::getline(arquivo, linha)) {
        // Linha de cabeçalho ignorada: "# x0, y0, x1, y1, wall Width"
    }

    while (std::getline(arquivo, linha)) {
        if (linha.empty() || linha[0] == '#') { // Ignorar linhas vazias ou comentários adicionais
            continue;
        }

        std::stringstream ss(linha);
        std::string valor;
        Parede p;
        std::vector<double> valores;

        while(std::getline(ss, valor, ',')) {
            try {
                valores.push_back(std::stod(valor));
            } catch (const std::invalid_argument& ia) {
                std::cerr << "Aviso: Valor inválido '" << valor << "' na linha: " << linha << ". Ignorando este valor." << std::endl;
                // Continuar a tentar ler os outros valores da linha se possível, ou invalidar a linha
            }
        }

        if (valores.size() >= 5) { // Precisa de pelo menos 5 valores
            p.x0 = valores[0];
            p.y0 = valores[1];
            p.x1 = valores[2];
            p.y1 = valores[3];
            p.largura = valores[4];
            paredes.push_back(p);

            // Atualizar maxX e maxY para determinar dimensões da matriz
            maxX = std::max({maxX, p.x0, p.x1});
            maxY = std::max({maxY, p.y0, p.y1});
        } else {
             std::cerr << "Aviso: Linha com dados insuficientes: " << linha << ". Ignorando linha." << std::endl;
        }
    }
    arquivo.close();

    if (paredes.empty()) {
        std::cerr << "Nenhuma parede válida encontrada no ficheiro." << std::endl;
        return false;
    }

    // Determinar dimensões da matriz com base na escala e coordenadas máximas
    // Adiciona uma pequena margem para garantir que todas as coordenadas caibam
    int numCols = static_cast<int>(std::ceil(maxX / escala)) + 1;
    int numLinhas = static_cast<int>(std::ceil(maxY / escala)) + 1;

    std::cout << "Dimensões calculadas para a matriz: " << numLinhas << " linhas x " 
              << numCols << " colunas (escala: " << escala << ")" << std::endl;

    // Inicializar matriz com 0 (caminho livre)
    matrizLabirinto.assign(numLinhas, std::vector<int>(numCols, 0));

    // "Desenhar" as paredes na matriz
    for (const auto& p : paredes) {
        // Coordenadas do centro da parede (para paredes com espessura)
        // Consideramos que (x0,y0)-(x1,y1) é o eixo central da parede.
        double min_x_parede_fisica = std::min(p.x0, p.x1) - p.largura / 2.0;
        double max_x_parede_fisica = std::max(p.x0, p.x1) + p.largura / 2.0;
        double min_y_parede_fisica = std::min(p.y0, p.y1) - p.largura / 2.0;
        double max_y_parede_fisica = std::max(p.y0, p.y1) + p.largura / 2.0;

        // Converter para índices da matriz
        // (floor para o início, ceil para o fim para garantir cobertura)
        int r_start = static_cast<int>(std::floor(min_y_parede_fisica / escala));
        int r_end = static_cast<int>(std::floor(max_y_parede_fisica / escala)); // -1 pode ser necessário se ceil for exclusivo
        int c_start = static_cast<int>(std::floor(min_x_parede_fisica / escala));
        int c_end = static_cast<int>(std::floor(max_x_parede_fisica / escala));

        // Garantir que os índices são válidos e iterar para marcar as paredes
        for (int r = std::max(0, r_start); r <= std::min(numLinhas - 1, r_end); ++r) {
            for (int c = std::max(0, c_start); c <= std::min(numCols - 1, c_end); ++c) {
                // Lógica mais precisa para desenhar linhas/retângulos grossos
                // A abordagem mais simples é preencher o bounding box da parede grossa.
                // Para linhas que são principalmente horizontais ou verticais, isso funciona bem.
                
                // Para uma parede horizontal (y0 ~ y1)
                if (std::abs(p.y0 - p.y1) < p.largura) { // Considera-a horizontal
                    if ( (c * escala >= min_x_parede_fisica && c * escala < max_x_parede_fisica + p.largura) && // Verifica X
                         (r * escala >= min_y_parede_fisica && r * escala < max_y_parede_fisica) ) {        // Verifica Y
                        matrizLabirinto[r][c] = 1; // Marca como parede
                    }
                } 
                // Para uma parede vertical (x0 ~ x1)
                else if (std::abs(p.x0 - p.x1) < p.largura) { // Considera-a vertical
                     if ( (r * escala >= min_y_parede_fisica && r * escala < max_y_parede_fisica + p.largura) && // Verifica Y
                          (c * escala >= min_x_parede_fisica && c * escala < max_x_parede_fisica) ) {        // Verifica X
                        matrizLabirinto[r][c] = 1; // Marca como parede
                    }
                }
                // (Esta lógica de rasterização pode precisar de ajustes para precisão,
                // especialmente se houver paredes diagonais ou requisitos muito específicos
                // sobre como a espessura é mapeada para as células.
                // Para paredes alinhadas com os eixos, preencher o retângulo da parede é uma boa aproximação.)
            }
        }
    }
    return true;
}



struct Node {
    double x;
    double y;
    int visitas;
    Node* pai; // De onde o robô veio para descobrir/alcançar este nó NESTA TRAVESSIA

    Node* filhoEsquerda;
    Node* filhoFrente;
    Node* filhoDireita;

    // Construtor
    Node(double coordX, double coordY) {
        x = coordX;
        y = coordY;
        visitas = 0; // Começa com 0, incrementa quando o robô *chega* ao nó
        pai = nullptr;
        filhoEsquerda = nullptr;
        filhoFrente = nullptr;
        filhoDireita = nullptr;
        // std::cout << "Nó protótipo criado para (" << x << ", " << y << ")" << std::endl;
    }
    bool RegistraNode(Node* nodeAtual){
    	if(nodeAtual!=){
    		Node* meuNo = new Node(edubotLib->getX(),edubotLib->getY());
    		
    		}
    }
};	

int main(){

	EdubotLib *edubotLib = new EdubotLib();
	
	//try to connect on robot
	if(edubotLib->connect()){//setup
		
		double velocidade;
		double sonar[7];
		int rotacao = 0;
		edubotLib->sleepMilliseconds(1000);//tempo para inicializar
		Node* Primeiro = new Node(edubotLib->getX(),edubotLib->getY());


		while(edubotLib->isConnected()){//loop principal
		
			for(int i = 0; i < 7; i++) {
				sonar[i] = edubotLib->getSonar(i);
				cout << "Sonar[" << i << "] = " << sonar[i] << endl;
			}
	
			velocidade=sonar[3]/2;
	
			
				edubotLib->move(velocidade);
				edubotLib->sleepMilliseconds(100);
			if((sonar[3]>TAMANHO)&&((sonar[0]>TAMANHO)||(sonar[6]>TAMANHO))){//achou intersecao
				if(verfificaNode()){//verifica se o node ja existe
					
					}
				
				
				
				}
		}
		
		while(edubotLib->isConnected()){

			for(int i = 0; i < 7; i++) {
			    sonar[i] = edubotLib->getSonar(i);
			    cout << "Sonar[" << i << "] = " << sonar[i] << endl;
			}

			
			velocidade=sonar[3]/2;
			cout<<sonar[3]<<endl;
			
			if (sonar[3]<0.20){
				if(sonar[0]>0.6){
					rotacao = -90;
				}else if(sonar[6]>0.6){
					rotacao = 90;
				}else{
				rotacao = 180;
				}
				edubotLib->rotate(rotacao);
				edubotLib->sleepMilliseconds(1000);
				cout <<edubotLib-> getTheta() <<endl;
				
			}else{
				edubotLib->move(velocidade);
				edubotLib->sleepMilliseconds(100);
				cout << "para frente"<<endl;
	
			}
		}
		
	}
	else{
		std::cout << "Could not connect on robot!" << std::endl;
	}

	return 0;
}

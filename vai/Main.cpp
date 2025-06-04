/*
 * EDUBOT HELLO WORLD EXAMPLE
 * @Author: Maik Basso <maik@maikbasso.com.br>
*/

#include <iostream>
#include <cmath>

#include "libs/EdubotLib.hpp"
using namespace std;


#define linha 3
#define coluna 100
#define TAMANHO 0.5
double intersecao[linha][coluna]={0};
int i=0;



void treumax(){//solver para labirinto
	
	while(edubotLib->isConnected()){
		
		for(int i = 0; i < 7; i++) {
			sonar[i] = edubotLib->getSonar(i);
			cout << "Sonar[" << i << "] = " << sonar[i] << endl;
		}

		velocidade=sonar[3]/2;

		do{//vai para frente ate encontrar intersecao
			edubotLib->move(velocidade);
			edubotLib->sleepMilliseconds(100);
		}while(!((sonar[3]>TAMANHO)&&((sonar[0]>TAMANHO)||(sonar[6]>TAMANHO))));
		intersecao[0][i] = getX();
		intersecao[1][i] = getY();
		intersecao[2][i]++;
		i++;
	}
}	

int main(){

	EdubotLib *edubotLib = new EdubotLib();
	
	//try to connect on robot
	if(edubotLib->connect()){
		
		double velocidade;
		double sonar[7];
		int rotacao = 0;
		edubotLib->sleepMilliseconds(1000);


		while(edubotLib->isConnected()){
		
			for(int i = 0; i < 7; i++) {
				sonar[i] = edubotLib->getSonar(i);
				cout << "Sonar[" << i << "] = " << sonar[i] << endl;
			}
	
			velocidade=sonar[3]/2;
	
			do{//vai para frente ate encontrar intersecao
				edubotLib->move(velocidade);
				edubotLib->sleepMilliseconds(100);
			}while(!((sonar[3]>TAMANHO)&&((sonar[0]>TAMANHO)||(sonar[6]>TAMANHO))));
			intersecao[0][i] = getX();
			intersecao[1][i] = getY();
			intersecao[2][i]++;
			i++;
			if()
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

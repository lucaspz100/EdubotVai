# -*- coding: utf-8 -*-
# 1. Importando as bibliotecas necessárias
import matplotlib.pyplot as plt
import csv
import math

# --- DOCUMENTAÇÃO ---
# Este script lê um arquivo de dados de um robô móvel, contendo coordenadas (x, y)
# e orientação (theta), e gera um mapa 2D da sua trajetória.
#
# BIBLIOTECAS:
# - matplotlib.pyplot: Usada para criar o gráfico (o mapa).
# - csv: Usada para ler e interpretar o arquivo de dados formatado com vírgulas.
# - math: Usada para converter o ângulo 'theta' de graus para radianos,
#         necessário para calcular a direção das setas de orientação.

def criar_mapa(nome_arquivo='dados_do_mapa.txt'):
    """
    Função principal para ler os dados e gerar o mapa.

    Parâmetros:
    - nome_arquivo (str): O nome do arquivo contendo os dados do robô.
    """

    # 2. Preparando as listas para armazenar os dados
    # Vamos guardar as coordenadas X, Y e a orientação Theta em listas separadas.
    coordenadas_x = []
    coordenadas_y = []
    orientacoes_theta = []

    # 3. Lendo e processando o arquivo de dados
    try:
        with open(nome_arquivo, mode='r', encoding='utf-8') as arquivo_csv:
            # O leitor de CSV facilita a separação dos valores em cada linha
            leitor_csv = csv.reader(arquivo_csv)

            # Pula a primeira linha (o cabeçalho: "x,y,theta,...")
            next(leitor_csv)

            # Itera sobre cada linha do arquivo
            for linha in leitor_csv:
                # Converte os valores de texto para números (float) e os adiciona às listas
                # linha[0] é o valor de x
                # linha[1] é o valor de y
                # linha[2] é o valor de theta
                coordenadas_x.append(float(linha[0]))
                coordenadas_y.append(float(linha[1]))
                orientacoes_theta.append(float(linha[2]))

    except FileNotFoundError:
        print(f"Erro: O arquivo '{nome_arquivo}' não foi encontrado.")
        print("Por favor, verifique se o arquivo está na mesma pasta que o script.")
        return
    except Exception as e:
        print(f"Ocorreu um erro ao ler o arquivo: {e}")
        return

    # 4. Criando o mapa com Matplotlib
    print("Dados lidos com sucesso! Gerando o mapa...")

    # Cria uma figura e eixos para o gráfico.
    # 'figsize' controla o tamanho da janela do gráfico.
    plt.figure(figsize=(10, 8))

    # Desenha a trajetória do robô conectando os pontos.
    # 'marker='o'' adiciona um círculo em cada ponto de coordenada.
    # 'linestyle='-'' conecta os marcadores com uma linha.
    plt.plot(coordenadas_x, coordenadas_y, marker='o', linestyle='-', label='Trajetória do Robô', color='cornflowerblue')

    # 5. Adicionando as setas de orientação
    # Para cada ponto, vamos desenhar uma pequena seta mostrando a direção 'theta'.
    for i in range(len(coordenadas_x)):
        x = coordenadas_x[i]
        y = coordenadas_y[i]
        theta_graus = orientacoes_theta[i]

        # Converte o ângulo de graus para radianos
        theta_radianos = math.radians(theta_graus)

        # Define o comprimento da seta de orientação no gráfico
        comprimento_seta = 0.1

        # Calcula o deslocamento da seta nos eixos X e Y
        dx = comprimento_seta * math.cos(theta_radianos)
        dy = comprimento_seta * math.sin(theta_radianos)

        # Desenha a seta
        plt.arrow(x, y, dx, dy, head_width=0.05, head_length=0.08, fc='red', ec='red')

    # 6. Configurando a aparência do gráfico
    plt.title('Mapa de Trajetória do Robô Móvel')  # Título do mapa
    plt.xlabel('Coordenada X (metros)')              # Rótulo do eixo X
    plt.ylabel('Coordenada Y (metros)')              # Rótulo do eixo Y
    plt.grid(True)                                  # Adiciona uma grade para facilitar a visualização
    plt.legend()                                    # Mostra a legenda ('Trajetória do Robô')
    plt.axis('equal')                               # Garante que as escalas dos eixos X e Y sejam iguais

    # 7. Exibindo o mapa
    plt.show()

# --- Execução do programa ---
if __name__ == "__main__":
    criar_mapa()
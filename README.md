# Mega Senha

Aplicação cliente-servidor desenvolvida em C, utilizando sockets TCP e Threads, como parte do trabalho 1 de Redes de Computadores. O projeto consiste na implementação do jogo Mega Senha, um jogo de adivinhação de palavras para dois jogadores, com controle de partidas, tempo, pontuação e ranking.

## Integrantes

* Henrique Ribeiro de Figueiredo - [henriquefigueiredo0](https://github.com/henriquefigueiredo0)
* João Pedro Lopes de Melo - [jp-lopes](https://github.com/jp-lopes) 
* Pedro Camelo Gonzaga - [Pedrocg2014](https://github.com/Pedrocg2014)

## Funcionamento

O sistema utiliza uma arquitetura cliente-servidor com comunicação por sockets TCP. O servidor aceita múltiplos clientes simultaneamente e utiliza uma thread para tratar a comunicação com cada cliente. Ao se conectar, o cliente escolhe um nome e entra no chat global. A partir dele, pode entrar na fila de espera para uma partida. Quando existem dois jogadores disponíveis, o servidor realiza o pareamento e cria uma nova partida.

O servidor funciona como o juiz do jogo, sendo responsável por controlar o estado da partida, sortear as palavras, contabilizar os pontos, controlar o tempo e alternar as funções dos jogadores. As palavras utilizadas como senha são carregadas de um arquivo CSV e armazenadas em uma tabela hash. O cliente também utiliza um dicionário para verificar as palavras digitadas antes do envio.

Durante a partida, um jogador recebe a senha e envia dicas para o outro jogador tentar adivinhar. Quando a senha é acertada, a pontuação é atualizada e uma nova senha é sorteada. Também é possível utilizar o comando `/pass` para pular a palavra atual. Ao final do primeiro tempo, os jogadores trocam de função. Depois do segundo tempo, a partida é encerrada e o resultado é armazenado no ranking.

### Comunicação

A comunicação entre cliente e servidor é realizada por sockets TCP, com as mensagens delimitadas pelo caractere `\n`.

Como o TCP fornece um fluxo contínuo de bytes, uma mensagem pode ser recebida em partes ou várias mensagens podem chegar em uma única chamada de `recv()`. Para tratar esse comportamento, os dados recebidos são armazenados em um acumulador e processados somente quando uma mensagem completa é identificada.

O envio utiliza uma função auxiliar que garante a transmissão de toda a mensagem, realizando chamadas sucessivas de `send()` caso apenas parte dos dados seja enviada. Dessa forma, tanto o envio quanto o recebimento são tratados independentemente da forma como os dados são divididos pela camada TCP.

### Tratamento de erros

O programa realiza verificações nas principais operações de rede e gerenciamento de recursos. São tratados erros na criação e configuração dos sockets, no estabelecimento das conexões, na transmissão e no recebimento de dados, além de falhas na criação de threads e alocação de memória.

Durante a comunicação, retornos inválidos de `send()` e `recv()` são identificados. O recebimento de um valor menor ou igual a zero em `recv()` é tratado como encerramento ou falha da conexão. O acumulador utilizado para as mensagens também possui verificação de capacidade, evitando escrita além dos limites do buffer.

O servidor também trata situações relacionadas ao funcionamento simultâneo de vários clientes. São verificados os limites de clientes e partidas e, caso não haja recursos disponíveis ou ocorra uma falha durante a criação de uma partida, os jogadores são informados e retornam ao estado anterior quando possível.

Caso um jogador se desconecte durante uma partida, ela é encerrada e o adversário é informado, retornando ao chat global. Para evitar condições de corrida entre as threads, o acesso às estruturas compartilhadas de clientes, partidas e ranking é protegido por mutex. O cliente utiliza o mesmo mecanismo para proteger seu estado compartilhado entre suas threads.

Ao encerrar normalmente, o cliente finaliza a conexão com `shutdown()` e libera o socket com `close()`.


## Compilação

Clonar o repositório e acessar o diretório raiz:

```bash
git clone https://github.com/jp-lopes/Trabalho-Redes
cd Trabalho-Redes
```

Compilar a partir do Makefile:
```bash
make
```

## Execução

Iniciar o servidor:

```bash
./bin/server
```

Ao executar o cliente, é possível especificar o IP do servidor:
```bash
./bin/client {IP_do_servidor}
```

Se esse IP não for especificado, utiliza-se por padrão o IP local 127.0.0.1.

## Repositório

https://github.com/jp-lopes/Trabalho-Redes

## Uso de ferramentas externas e IA

Durante o desenvolvimento do projeto foram utilizadas ferramentas de Inteligência Artificial como auxílio em algumas tarefas. A IA foi utilizada principalmente para filtrar as palavras mais usadas de um dicionário externo e as melhores palavras para serem a Senha do jogo. Após a geração, os dados foram revisados e adaptados para o funcionamento do programa.

Também foi utilizada como apoio na implementação e revisão de algumas funções auxiliares, principalmente:

* implementação da tabela hash utilizada para armazenar e consultar as palavras do dicionário a fim de minimizar colisões;
* tratamento e padronização de caracteres das palavras, incluindo letras maiúsculas/minúsculas, acentos e ç;
* organização e filtragem das palavras provenientes dos dicionários externos.

Os dicionários e listas de frequência foram utilizados como fonte de dados para obter um conjunto maior de palavras em português. Esses dados foram posteriormente processados para adequação às regras do jogo.
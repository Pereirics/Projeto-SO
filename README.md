CLIENTE
    - Interface com o utilizador via linha de comandos

SERVIDOR
    - Cliente interage com o servidor
    - Mantém em memória e em ficheiros a informação para suportar as funcionalidades

Cliente e servidor vão comunicar usando pipes com nome.


FUNCIONALIDADES BÁSICAS

    CLIENTE
        - Execução de programas por parte do utilizador
            - execute -u programa, irá executar programas do tipo cat, grep, wc, etc
            - Número de argumentos é variável
            - A execução do programa deve ser idêntica para o utilizador como quando corre na linha de comando (stdin e stdout)
        
        - Novo programa do utilizador
            - Cliente deve informar ao servidor com:
                - PID do processo a executar
                - Nome do programa a executar
                - Timestamp atual
            - Cliente deve informar o utilizador sobre o PID do programa que será executado
            - O PID serve como id visto que os utilizadores podem executar várias vezes programas com o mesmo nome
            - Usar o gettimeofday para o timestamp
        
        - Terminação de um programa
            - O cliente deve informar o servidor com:
                - PID do processo que terminou
                - Timestamp atual
            - O cliente deve informar o utilizador do tempo de execução do programa em milisegundos
    
    CONSULTA DE PROGRAMAS EM EXECUÇÃO
        - Opção status
            - Feito pelo servidor, o cliente apenas envia o pedido e recebe a resposta, apresentando-a ao utlizador
            - Listar um por linha, os programas em execução no momento, deve conter:
                - PID
                - Nome
                - Tempo de execução até ao momento (milisegundos)

    SERVIDOR
        - O servidor deve suportar o processamento concorrente de pedidos, evitando que clientes a realizar pedidos que obriguem a um maior tempo de processamento possam bloquear a interação de outros clientes com o servidor
Funcionalidades Básicas
Tokenizer:
	Derivado da criação da funcionalidade “execute -u”, onde será necessário analisar o conteúdo da string fornecida pelo utilizador ao cliente, surgiu a necessidade da criação de um sistema auxiliar que nos permita dividir esta em seus vários componentes (cujo parâmetro de divisão seja o caracter espaço).
	Com isto em mente, recorreu-se a uma função tokenizer, aonde se dá a separação do input introduzido pelo utilizador em tokens de formato string (que posteriormente serão armazenados array), o que então possibilita o uso do conteúdo do programa em si – neste caso, o seu comando e respetivos argumentos – em outras funcionalidades do projeto.

Execute -u:

	Para então prosseguirmos com a execução dos programas em questão, definiu-se uma função execute onde aplicamos um fork ao pid em questão (para permitir a execução sem uma terminação prévia do programa) e então concretizar a leitura deste mesmo comando para que este seja executado com os seus respetivos argumentos. Foram também escritas mensagens de erro caso aconteçam durante o correr do código, desde a impossibilidade de criação de um processo filho a execução através do execvp.
	
Time values, etc...
	A funcionalidade cor do projeto, o execute, procede com a criação de um processo pai que estará conectado a um pipe que servirá para futuro intercambio de informação com os processos a correr. Prossegue-se então com um fork para então executarmos os comandos que serão introduzidos pelo utilizador seguido da função.
	Com a função gettimeofday, procedemos a atribuição do tempo inicial ao qual um programa começa a executar e passa-se essa informação para o processo pai através de uma e o servidor, onde será devidamente guardado.

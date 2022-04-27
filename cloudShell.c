#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include "./libs/readln.h"

int pidFilhoExecutar = -1, pidCloudShell = -1, pidComunicador = -1, pidEnviarPerc = -1;
int entradaPipe, fd[2];
char nome[20];

void enviarCPUPercentagem(int s){ // envia de 1 em 1 seg a percentagem que o processo está a gastar da cpu
	char str[6];
	sprintf(str,"%d", pidFilhoExecutar);
	
	if(pidEnviarPerc=fork() == 0){
		execlp("ps","ps","-p", str , "-o", "%cpu",NULL);
	}
	else wait(NULL);
}

void executar(int argc, char *argv[]){
	int pidenviarCPUPer;

	pidFilhoExecutar = fork();
	if(pidFilhoExecutar == 0){	/* executamos o pretendido */
		if(argc >= 1){
			execvp(argv[0], &argv[0]);
		}
	}
	else{
		pidenviarCPUPer = fork();
		if(pidenviarCPUPer == 0){	/* abre outro filho pra enviar %cpu */
			dup2(fd[1],1);
			close(fd[1]);
			close(fd[0]);
			signal(SIGALRM, enviarCPUPercentagem);
			while(1){
				alarm(1);
				pause();
			}
		}
		wait(NULL);
		pidFilhoExecutar = -1;
		kill(pidenviarCPUPer,SIGKILL);
	}
	wait(NULL);
}

void criarComunicador(){
	char output[50];
	char tralha[50];

	if(pidComunicador = fork() ==0){
		close(fd[1]);
		int tamStr = strlen(nome),i=0,z;
		float percentagem;
		char *token;

		memcpy(output+sizeof(float),&pidCloudShell,sizeof(int));
		strcat(output + sizeof(float)+sizeof(int),nome);

		while(z=read(fd[0], tralha, 50)){
			i=0;
			token = strtok(tralha," \n");	
			while(token){
				i++;
				if(i==2){	// guarda a percentagem
					percentagem = atof(token);
					break;
				}
				token = strtok(NULL, " \n");
			}
			memcpy(output,&percentagem,sizeof(float));
			write(entradaPipe,output,(sizeof(int)+sizeof(float)+tamStr+1));
		}
	}
}

void logIN(){
	char pass[20], dados[50];
	int tamanhoNome, tamanhoPass;
	int pipeEntradaLogIN = open("./pipes/fifoLogIN",O_WRONLY);

	printf("Login: ");
	scanf(" %s", nome);nome[strlen(nome)] = '\0';
	printf("Pass: ");
	scanf(" %s", pass);nome[strlen(pass)] = '\0';

	// vou enviar o meu PID, nome e pass
	memcpy(dados,&pidCloudShell,sizeof(int));
	tamanhoNome = strlen(nome);
	memcpy(dados+sizeof(int),nome,tamanhoNome);
	dados[sizeof(int) + strlen(nome)] = '|';
	tamanhoPass = strlen(pass);
	memcpy(dados + sizeof(int) + strlen(nome) + 1, pass,tamanhoPass);
	dados[sizeof(int) + tamanhoNome + 1 + tamanhoPass] = '\0';
	write(pipeEntradaLogIN,dados,sizeof(int) + tamanhoNome + 1 + tamanhoPass);
	pause();
	close(pipeEntradaLogIN);
}

void passErrada(int s){
	char aux[20] = "Login incorreto.\n";
	write(1,aux,17);
	exit(1);
}

void passCerta(int s){
	char bemVindo[30] =  "Bem-Vindo ";
	strcat(bemVindo,nome);
	bemVindo[strlen(bemVindo)] = '\n';
	bemVindo[strlen(bemVindo) + 1] = '\0';
	write(1,bemVindo,strlen(bemVindo));
}

void semSaldo(int s){
	char esc;
	write(1,"\t\tSALDO INSUFICIENTE! \n",24);

	//mata o processo a ser executar
	if(pidFilhoExecutar != -1)
		killpg(pidFilhoExecutar, SIGKILL);
	//mata o processo q seg a seg envia a perc
	if(pidEnviarPerc != -1)
		killpg(pidEnviarPerc,SIGKILL);
	//mata o processo a comunicar
	if(pidComunicador != -1)
		killpg(pidComunicador,SIGKILL);
	//suicida-se
	exit(1);
}

int main(){
	pidCloudShell = getpid();	// guarda na var global o pid da cloud
 	entradaPipe = open("./pipes/fifo", O_WRONLY);

	signal(SIGUSR2, passCerta);	// espera pelo sinal emitido quando a pass está correta
	signal(SIGQUIT, passErrada);	// espera pelo sinal emitido quando a pass está incorreta
	signal(SIGUSR1, semSaldo);	// espera pelo sinal emitido quando acaba o saldo

	logIN();			// manda o longIN para o servidor e espera pelo um sinal de resposta

	pipe(fd);			// cria um pipe anonimo
	criarComunicador();		// cria um processo que comunica com o servidor de contabilizacao

	int i = 0;
	char linha[50], *token, **argumentos;
	
	

	while(readLine(0,linha, 20)){
		argumentos = (char **)malloc(sizeof(char *) * 20);
		for(i=0;i<20;i++) argumentos[i] = (char *)malloc(sizeof(char) * 20);

		i = 0;
		token = strtok(linha," \n");	
		while(token){
			strcpy(argumentos[i++],token);
			token = strtok(NULL, " \n");
		}
		argumentos[i] = NULL;
		executar(i,argumentos);
		free(argumentos);
	}

	close(entradaPipe);
}

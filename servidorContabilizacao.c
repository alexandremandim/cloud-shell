#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include "./libs/readln.h"

int pipeSaidaNormal, pipeSaidaLogIN;

typedef struct user{
	char nome[20];
	char pass[20];
	float saldo;
} *USER;

typedef struct usersgroup{
	USER u;
	struct usersgroup *prox;
} *USERSGROUP;

USERSGROUP addUser(USERSGROUP ug, USER u){
	USERSGROUP aux = ug;
	
	USERSGROUP new = (USERSGROUP)malloc(sizeof(struct usersgroup));
	new->u = u;
	new->prox = NULL;

	if(aux == NULL){
		return new;
	}

	while(aux->prox != NULL){
		aux = aux->prox;
	}
	aux->prox = new;
	return ug;
}

USER getUserNome(USERSGROUP ug, char* nome){	// dado um usersgroup e um nome, retorna o user
	USERSGROUP aux = ug;
	while(aux){
		if(strcmp(aux->u->nome,nome) == 0){
			return aux->u;
		}
		aux = aux->prox;
	}
	return NULL;
}

void escreverFicheiro(USERSGROUP ug){
	USERSGROUP aux = ug;
	int fdFicheiro = open("./log/saldos", O_CREAT | O_WRONLY | O_TRUNC, 0666); 
	char floatString[20];

	while(aux){
		write(fdFicheiro,aux->u->nome,strlen(aux->u->nome));
		write(fdFicheiro,"\n",1);
		write(fdFicheiro,aux->u->pass,strlen(aux->u->pass));
		write(fdFicheiro,"\n",1);
		sprintf(floatString,"%f",aux->u->saldo);
		write(fdFicheiro,floatString,strlen(floatString));
		write(fdFicheiro,"\n",1);	
		aux = aux->prox;
	}
	close(fdFicheiro);
}

void atualizaSaldo(float perc,char *nome, int pidAtual, USERSGROUP u){
	USER eu = getUserNome(u,nome);
	if(eu==NULL)return;

	// Calcula novo saldo
	eu->saldo = eu->saldo - (perc)/10;
	// Guardar no ficheiro
	escreverFicheiro(u);
	// Mata o processo se saldo insuficiente
	if(eu->saldo <= 0){kill(pidAtual, SIGUSR1);}
}

USERSGROUP abrirDadosFicheiro(){
	USERSGROUP new = NULL;

	int pass=0,z;
	int fdF = open("./log/saldos", O_RDONLY);
	char aux[20], *token;
	USER novo = NULL;
	USERSGROUP newUG = NULL;

	if(fdF == -1) return NULL;

	while(readLine(fdF,aux,20)){
		token = strtok(aux,"\n\r ");
		if(token){
			if(novo == NULL){// é um nome
				novo = (USER)malloc(sizeof(struct user));
				strcpy(novo->nome,token);
				pass = 1;
			}
			else if(pass==1){
				strcpy(novo->pass,token);
				pass = 0;
			}
			else{	// é um saldo
				novo->saldo = atof(token);
				newUG = addUser(newUG,novo);
				novo = NULL;
			}
		}
	}
	close(fdF);
	return newUG;
}


int tentativaLogIN(USERSGROUP ug, char *dados, int pid){ // 0 false, 1 true
	char nome[20],pass[20];
	int i,z;
	for(i=0;i<40 && dados[i] != '\0';i++){	// get nome
		if(dados[i] == '|'){
			break;
		}
		else{
			nome[i] = dados[i];
		}
	}
	nome[i++] = '\0';

	for(z=0;dados[i] != '\0';i++){	// get pass
		pass[z++] = dados[i];
	}
	pass[z] = '\0';

	USER u = getUserNome(ug,nome);
	if(u==NULL) kill(pid,SIGQUIT);
	else{
		if(strcmp(pass,u->pass) == 0){ 
			if(u->saldo >= 0){		
				kill(pid,SIGUSR2);
			}
			else{
				kill(pid,SIGUSR1);
				return 0;
			}
		}
		else	// pass incorreta
			kill(pid,SIGQUIT);
	}
}

void logIN(USERSGROUP ug){
	if(fork() == 0){
		int pid,l;
		char dados[40]; //string onde recebe o nome, / e a pass

		while(read(pipeSaidaLogIN,&pid,sizeof(int))){	// espera que alguem tente logar
			l = read(pipeSaidaLogIN,dados,40);
			dados[l] = '\0';
			ug = abrirDadosFicheiro();
			tentativaLogIN(ug,dados,pid);	// tenta fazer logIN
		}
		exit(1);
	}
}

void alwaysOpenFifo(){
	if(fork() == 0){
		int fifoN = open("./pipes/fifo",O_WRONLY);
		pause();
		close(fifoN);
	}
}

void alwaysOpenFifoLogIN(){
	if(fork() == 0){
		int fifoL = open("./pipes/fifoLogIN",O_WRONLY);
		pause();
		close(fifoL);
	}
}

int main(){
	USERSGROUP nossaLista = abrirDadosFicheiro();	// Carregar ficheiros

	mkfifo("./pipes/fifo",0777);mkfifo("./pipes/fifoLogIN",0777); // cria pipe de logIN e %cpu
	alwaysOpenFifo();alwaysOpenFifoLogIN();
	pipeSaidaLogIN = open("./pipes/fifoLogIN",O_RDONLY);
	pipeSaidaNormal = open("./pipes/fifo",O_RDONLY);

	logIN(nossaLista);	// cria fork para esperar por logIN's
	fflush(stdout);
	float cpuPerc;	//guarda a percentagem do CPU
	char user[30];	//guarda o user
	int pidCloud;	// guarda o pid

	while(read(pipeSaidaNormal, &cpuPerc, sizeof(float))){
		read(pipeSaidaNormal, &pidCloud, sizeof(int));
		read(pipeSaidaNormal, user,29);
		printf("PID Cloud: %d -> Consumiu %f; USER: %s \n",pidCloud, cpuPerc, user);
		atualizaSaldo(cpuPerc,user,pidCloud,nossaLista);
	}
	close(pipeSaidaLogIN);
	close(pipeSaidaNormal);
}

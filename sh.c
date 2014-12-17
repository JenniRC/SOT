/* 
	gcc -c -Wall -Wshadow  mysh.c && gcc -o mysh mysh.o && ./mysh
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>

#define MAX 51
#define PROMPT ">> "

	/* Estructura para los comandos */
	typedef struct command {
		struct command *next;
		char *cmd[];
	}Command;

	typedef	struct cmdlist{
    	struct command *first;
		int length;
	}Mycmdlist;
	
/************************** Funciones para la listas de comandos y etc*******************/
int
emptylist(Mycmdlist* list){
	return list->first == NULL;
 }
void 
initlist (Mycmdlist *list){
	list->first = NULL;
	list->length = 0;
}

int 
addlist (Mycmdlist *list, Command *cmmd){
  
  if (emptylist(list)){
  	cmmd->next=NULL;
  	list->first = cmmd;
  	printf("intentando añadir en add %s\n",cmmd->cmd[0]);
  		printf("intentando añadir en add %s\n",cmmd->cmd[1]);
  			printf("intentando añadir en add %s\n",cmmd->cmd[2]);
  			printf("intentando añadir en add %s\n",cmmd->cmd[3]);
  }else{
  	cmmd->next = list->first;
  	list->first = cmmd;
  	  	printf("intentando añadir en add %s\n",cmmd->cmd[0]);
  		printf("intentando añadir en add %s\n",cmmd->cmd[1]);
  			printf("intentando añadir en add %s\n",cmmd->cmd[2]);
  			printf("intentando añadir en add %s\n",cmmd->cmd[3]);
  }

  list->length++;
  printf("salgo del añadir\n");
  return 0;
}

void
printlist(Mycmdlist *list){
	Command *cmd_aux = list->first;
		while(cmd_aux != NULL){
			printf(" %s , %s ",cmd_aux->cmd,cmd_aux->cmd[0]);
			cmd_aux = cmd_aux->next;
		} 
}	
//(cmmd = (Command *) malloc (sizeof (Command))) == NULL)//Command * cmmd;

/******************************************************************************************************/
int
mytoken(char *cadena,char *separador,char *argv[]){
    char *str1, *token;
    char *saveptr1;
    int j;
    for (j = 1, str1 = cadena;; j++, str1 = NULL) {

        token = strtok_r(str1, separador, &saveptr1);
            
            if (token == NULL)
                   break;
           argv[j-1]=token;
      //  printf("%d: %s\n", j, token); /*si es " " al final, lo cuenta como 1,asi que cuidado ahí*/
    }
    return j;
}

/* La función principal main contiene el bucle que se ejecutará de forma contínua hasta que
el usuario escriba FIN en el terminal.Recibe una cadena, la analiza, y a partir de las 
funciones que hemos creado, hará lo que debe de hacer */
void 
initbuf(char *buf){
	buf = malloc(1024*sizeof (char));
}
int
static is_file(char *d_name){
	struct stat st;
	stat(d_name, &st);
	return ((st.st_mode & S_IFMT)==S_IFREG);
}
int
flagP(char *argv[],int nc){
	int i=0;
	int flag=0;

	while (i<nc-1){

		if (strcmp(argv[i],"|")==0)
			flag++;
		i++;
	}
	return flag;
}

int
flagFO(char *argv[],int nc){
	int i=0;
	int flag=0;

	while (i<nc-1){
		
		if (strcmp(argv[i],">")==0)
			flag=i+2;
		i++;
	}
	return flag;
}
int
flagB(char *argv[],int nc){
	int flag=0;
		
	if (strcmp(argv[nc-1],"&")==0)
			flag=1;
	return flag;
}

int
flagFI(char *argv[],int nc){
	int i=0;
	int flag;

	while (i<nc-1){
		if (strcmp(argv[i],"<")==0)
			flag=i+2;
		
		i++;
	}
	return flag;
}
int
flagVE(char *argv[],int nc){
	int i=0;
	int flag;

	while (i<nc-1){
		if (strcmp(argv[i],"$")==0)
			flag=i+2;
		i++;
	}
	return flag;
}


/* Construye un path de una ruta para por ejemplo,buscar con access si esta el ejecutable*/
char *
built_path(char *s1, char *path){
	char * aux;
	int n1, n2;

	n1 = strlen(s1);
	n2 = strlen(path);
	aux = malloc (n1+n2+2);
	sprintf(aux, "%s/%s", path, s1);
	return aux;
}
/*Si existe el fichero retorna su fd, sino crea uno y devuelve fd tb -1*/
int 
test_file(char* dir,char* argv){
	DIR *d = opendir(dir);
	struct dirent *de;
	int fd;

	if(d == NULL) {
		err(1,"%s" ,dir);
	}
	char *buf =malloc(sizeof (PATH_MAX));
	fd =-1;
	while((de = readdir(d)) != NULL) {
		buf = built_path(de->d_name,dir);
			if (is_file(buf) && (strcmp(de->d_name,argv)==0)) {
				fd = open(buf, O_RDWR);
				break;
			}
	}
	fd = creat(argv, O_RDWR);
	if(fd < 0)
		err(1, "No way to create a file\n"); 

	return fd;
free(buf);
closedir(d);

}
int
my_pipe_file(int fi,char** cmd,int fo,int nc){ 
	int nr;
	// Parent process creates all needed pipes at the beginning
	// 1 Pipe for 2 command (number of command - 1)
	int fd[2*(nc-1)];
	int i;
	for( i = 0; i < (nc-1); i++ ){
    	if( pipe(fd + i*2) < 0 ){
		printf("Pipe creation failed\n");
		return 1;    
		}
	}

	// Loop for all commands to execute -
	nr = -1;
	int j=0;
	for(j=0; j < nc; j++){

		// create the child
		int pid;
		if((pid = fork()) < 0)
		{
			printf("fork failed\n");
			return 2;
		}

		if (pid == 0)	
		{
			if(j == 0){
				// First command
				dup2(fi,0); // connect fi to stdin
			}
			if(j == (nc-1)){
				// Last command
				dup2(fo,1); // connect fo to stdout
			}
			if(j!=0){
				// NOT First Command
				dup2(fd[(j-1)*2],0); // get imput from previous command
			}
			if(j != (nc-1)){
				// NOT last command to process
				dup2(fd[j*2+1],1); // connect output to next command
			}
			// close pipes;
			int q;
            for(q = 0; q < 2*(nc-1); q++){
                    close(fd[q]);
            }

			// execute command
			int k = j+5;
			execlp(cmd[k], cmd[k], NULL);
			printf("Execute command failed"); // if execlp returns, we get an error
		}
	}
	return nr;
} 
int
my_pipe_no_file(char** cmd,int nc){ 
	int nr;

	// Parent process creates all needed pipes at the beginning
	// 1 Pipe for 2 command (number of command - 1)
	int fd[2*(nc-1)];
	int i;
	for( i = 0; i < (nc-1); i++ ){
    	if( pipe(fd + i*2) < 0 ){
		printf("Pipe creation failed\n");
		return 1;    
		}
	}

	// Loop for all commands to execute -
	nr = -1;
	int j=0;
	for(j=0; j < nc; j++){

		// create the child
		int pid;
		if((pid = fork()) < 0)
		{
			printf("fork failed\n");
			return 2;
		}

		if (pid == 0)	
		{
			if(j!=0){
				// NOT First Command
				dup2(fd[(j-1)*2],0); // get imput from previous command
			}
			if(j != (nc-1)){
				// NOT last command to process
				dup2(fd[j*2+1],1); // connect output to next command
			}
			// close pipes;
			int q;
            for(q = 0; q < 2*(nc-1); q++){
                    close(fd[q]);
            }

			// execute command
			int k = j+1;
			execlp(cmd[k], cmd[k], NULL);
			printf("Execute command failed\n"); // if execlp returns, we get an error
		}
	}
	return nr;
} 
void
choose(char *argv[],int flagp, int flagb, int flagfo,int flagfi){
	
if ( flagp > 0 ){
	if (flagfi!=0 && flagfo != 0){
//		my_pipe_file();
	}
	if (flagfi!=0 && flagfo == 0){
//		my_pipe_file();/*con fo = 1;*/
	}
	if (flagfi==0 && flagfo != 0){
//		my_pipe_file();/*con fi = 0;*/
	}

}

}
/*Para cambiar de directorio -este es el built in que había que hacer-. Si tengo arg en
	el comando cd, voy a ese path, si no, voy a $HOME */
void
mycd(char * path){
	if (path==0){
		path = getenv("HOME");
		chdir(path);
	}else{
		chdir(path);
	}
}

/*para sacar las rutas de la variable path ( se usa en el tercer caso de ejecutable)
	retorna nc = numero de rutas posibles si todo ok, o -1 si no ok*/
int
rutas(char *path,char *rutaux[]){
	int nc;
	nc = mytoken(path,":",rutaux);
	
	if (nc>0){
		return nc;
	}else{
		return -1;
	}
}
//*path[] es el array de rutas de la variable PATH,conseguido por 'rutas',char * cmd = cmd[0] de un comando
//me devolverá la ruta dentro de PATH en la que está el ejecutable,para luego llamar a exec
char*
execruta(char * path[],char * cmd){
	int i;
	char * aux=NULL;
	for (i=1;i< sizeof path; i++){
		aux = built_path(cmd,path[i]);
		if (!(access(aux,X_OK))){
			return aux;
			break;
		}
		i++;
	}
		return aux;
}
/* Le paso mi array de comando con pipes,redireciones y tal, y voy separandolos en comandos..y añadiendo a lista*/
void
getcmd(Mycmdlist *cmdlist,char * argv[],int nc){//addlist (Mylist *list, char *word){
	int j;
	int i=0;
	Command *comando= malloc(sizeof (Command));
	printf("llego al principio del getcmd\n");
	while(j < nc){
		if ((strcmp(argv[j],"|")) && (strcmp(argv[j],">")) && (strcmp(argv[j],"<"))) { 
			comando->cmd[i]= argv[j];
			printf("comando cmd %s\n",comando->cmd[i] );
			i++;
			j++;
		}
	}
	addlist(cmdlist,comando);
	i=0;
	memset(comando->cmd,0,i);
}

int main(int argc, char *argv[]){ 


int fin=0;//i,segplano=0, 
int Stdout = 1; 
int Stdin = 0;
char *buf =(char*) calloc(1024 ,sizeof(char));

int nc=0;
/*flags para pipes,output file,input file*/
int flagp;
int flagfo=0;
int flagfi=0;
int flagb=0;
char *myarg[21];

char *path[21];
char *posiblesrutas[21];
/*mi variable de entorno path */
char *varpath = getenv("PATH");
Command comando;
Mycmdlist *cmdlist=malloc(sizeof(Command));
			//printf("%s\n",varpath);
/*Bucle infinito,para estar leyendo siempre */
while(fin==0) {
        dup2(Stdout,1); // Asigno la salida estándar, es decir, la consola.
        dup2(Stdin,0);
        printf(PROMPT);
        /*Recogo linea a linea desde la consola o desde archivo*/
        while(fgets(buf, 1024, stdin ) != NULL) { 
			nc = mytoken(buf," ",myarg);
			/*Aqui añado comandos a la lista*/
			getcmd(cmdlist,myarg,nc);
			//char *myarg[nc+1];
			// Will copy nc characters from array1 to array2
			//strncpy(myargaux, myarg, nc);
		/*Esto es para comprobar lo que tengo en mi array de comandos y tal*/
			/*int i=0;
			while (i<nc-1){
				printf("comando %d : %s\n",i,myarg[i]);
				i++;
			}*/
				/* flagp para pipes,flagb para Background*/
			flagp=flagP(myarg,nc);
			flagb=flagB(myarg,nc);

			if (flagFI(myarg,nc)!=0){
				int s = flagFI(myarg,nc); 
				int fc = mytoken(myarg[s-1],"/",path);
					if (fc==1){
						int	fi = test_file(".",path[fc-1]);
						printf("fi=%d\n",fi);
					}else{
						int	fi = test_file(path[0],path[fc-1]);
						printf("fi=%d\n",fi);
					}	
			}
			if (flagFO(myarg,nc)!=0){
				int s = flagFI(myarg,nc); 
				int fc = mytoken(myarg[s-1],"/",path);
					if (nc==1){
						int	fo = test_file(".",path[fc-1]);
						printf("fo=%d\n",fo);
					}else{
						int	fo = test_file(path[0],path[fc-1]);
						printf("fo=%d\n",fo);
					}	
				int	fo = test_file(path[0],path[fc-1]);
				printf("fo=%d\n",fo );
			}

			/************* EJECUTAR CD *************/
			if (!strcmp(myarg[0],"cd")){
				printf("entro aqui\n" );
				char *np =(char*) calloc(35 ,sizeof(char));
					if (nc==1){
						np = myarg[1];
						mycd(np);
					}
					if(nc==0){
						printf("entro en 0\n" );
						mycd(0);
					}
			}
			printf("flags ..fp %d,fo %d,fi %d\n",flagp,flagFO(myarg,nc),flagFI(myarg,nc));
		//separarcomandos();
			/*Si no es un built in (como cd) seran comandos ejecutables en el directorio actual*/

			/*Si no son comandos ejecutables en el directorio actual,son en la variable path*/
			int nr = rutas(varpath,posiblesrutas);
			char *rutadefinitiva =(char*) calloc(1024 ,sizeof(char));
			rutadefinitiva=execruta (posiblesrutas,"cat");
			printf("esta es la ruta para el exec :%s\n",rutadefinitiva );
		/*	printf("%d\n", nr);
			if(nr > 0){
				int i=0;
					while (i<nr-1){
					printf("ruta %d : %s\n",i,posiblesrutas[i]);
					i++;
				}
			}*/
	/*Aqui tenemos los flags, para saber */


			initbuf(buf);
			memset( myarg, 0, nc ) ;
			printf(PROMPT);
		}
		}
exit(0);

} 

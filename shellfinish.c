/* 
	gcc -c -Wall -Wshadow  shell.c && gcc -o shell shell.o && ./shell
	cat < idefix | sort | uniq > fo
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

#define MAX 256
/* Estructura para los comandos */
typedef struct {
	char *command;
	char *cmdarg[];
}Command;

/*Cambia el \n por \s (final de linea / final de string)*/
void
quit_end(char * str){
    int lon = strlen(str);
    if(str[lon-1]=='\n'){
        str[lon-1]='\0';        
    }
}

int
flag(char *line,char sep){
	int flag=0;
	char *flg;
	if ((flg = strchr(line,sep))) {
		flag=1;
	
			if(sep=='&'){
				flg[0]='\0';
			}
	}

	return flag;
}
/*Para cambiar de directorio -este es el built in que había que hacer-. Si tengo arg en
	el comando cd, voy a ese path, si no, voy a $HOME */
void
mycd(Command *cmd,char *lastcd){
	if (strcmp(lastcd,"") ==0){
		lastcd=cmd->cmdarg[1];
		//printf("ENT%s\n",cmd->cmdarg[1] );
	}

	if (cmd->cmdarg[1] == NULL){
		lastcd="HOME";
		chdir(getenv("HOME"));
		printf("CD A HOME\n");

	}else{
		if(strcmp(cmd->cmdarg[1],"-")==0){
			chdir(lastcd);
			printf("CD A %s \n",lastcd);
			
		}else{
			if(chdir(cmd->cmdarg[1])<0){
           		fprintf(stderr,"error cd $%s \n",cmd->cmdarg[1]); 
            
        	}
		}
	}
	lastcd=cmd->cmdarg[1];
}
/* Construye un path de una ruta para buscar con access si esta el ejecutable*/
char *
built_path(char *s1, char *path){
	char * aux;
	int n1, n2;

	n1 = strlen(s1);
	n2 = strlen(path);
	aux = malloc (n1+n2+1);
	sprintf(aux, "%s/%s", path, s1);
	return aux;
	free(aux);
}

Command*
mytoken(char *cadena,Command *cmd,char *sep){
    char *token;
    char *saveptr1;
    int j=0;
    token = strtok_r(cadena,sep,&saveptr1);
    cmd->command = token;   		
    while(token){
    	if (strcmp(token," ") !=0)
       		cmd->cmdarg[j]=token;
       		quit_end(cmd->cmdarg[j]);
    	j++;
       	token = strtok_r(saveptr1,sep,&saveptr1);/*si es " " al final, lo cuenta como 1,asi que cuidado ahí*/
   	}
    return cmd;
}
/*Para hacer la asignación en variables de entorno*/
void
to_asig(char *cadena){
 	//printf ("vars %s,%s,%s \n", var->cmdarg[0],var->cmdarg[1],var->cmdarg[2]);
    Command* var=(Command*)malloc(512*sizeof(Command));
	var = mytoken(cadena,var," ");
	if((var->cmdarg[1]!=NULL) && (var->cmdarg[2]!=NULL)){//x = y 
		if(setenv(var->cmdarg[0],var->cmdarg[2],1)<0){
       	    fprintf(stderr,"Impossible to make an assignament from %s to %s\n",var->cmdarg[0],var->cmdarg[2]);
       	}
	}
	if ((var->cmdarg[1]!=NULL )&& (var->cmdarg[2]==NULL)) {//x =y
		Command* aux=(Command*)malloc(512*sizeof(Command));
		char * line =malloc(128*sizeof(char));
		sprintf(line, "%s%s", var->cmdarg[0], var->cmdarg[1]);
		aux = mytoken(line,aux,"=");
		if(setenv(aux->cmdarg[0],aux->cmdarg[1],1)<0){
       	    fprintf(stderr,"Impossible to make an assignament from %s to %s\n",aux->cmdarg[0],aux->cmdarg[1]);
       	}
	}
	if ((var->cmdarg[1]==NULL) && (var->cmdarg[2]==NULL)) {//x=y
		Command* aux=(Command*)malloc(512*sizeof(Command));
		aux = mytoken(var->cmdarg[0],aux,"=");
		if(setenv(aux->cmdarg[0],aux->cmdarg[1],1)<0){
       	    fprintf(stderr,"Impossible to make an assignament from %s to %s\n",var->cmdarg[0],var->cmdarg[1]);
       	}
	}
}
/*Para hacer la sustitucion en variables de entorno,devuelvo otra linea con el nombre de las variables cambiadas*/
char *
to_sust(char *cadena){
	char *line=(char*) malloc(1024*sizeof(char));	
   	strcpy(line, cadena);
   	int i=0;
   	char *line_aux=(char*) malloc(1024*sizeof(char));	
   	Command* var=(Command*)malloc(512*sizeof(Command));
	var = mytoken(line,var," ");
	while(var->cmdarg[i] != NULL){
		if ((strchr(var->cmdarg[i],'$'))!=NULL){
			Command* aux=(Command*)malloc(512*sizeof(Command));
			aux = mytoken(var->cmdarg[i],aux,"$");
			quit_end(aux->cmdarg[0]);
        	if(!getenv(aux->cmdarg[0])){
            	fprintf(stderr,"error: var %s does not exist\n",aux->cmdarg[i]);
            	break;
      		}else{
      			var->cmdarg[i]=getenv(aux->cmdarg[0]);
       			free(aux);
      		}
   		}
		sprintf(line_aux,"%s %s", line_aux,var->cmdarg[i]); 
		i++;
	}
	//printf("%s\n",line_aux);
	return line_aux;
	free(line);
	free(var);
	free(line_aux);
}
//me devolverá la ruta dentro de PATH en la que está el ejecutable,para luego llamar a exec
char*
execruta(char *path, char * cmd,int j){
	int i=0;
	char *aux=NULL;
	if (j>0){
		aux = built_path(cmd,path);
			if (access(aux,X_OK)!=0){
				return NULL;
			}
	}
	if(j<0){
		Command* rutaux =(Command*)malloc(256*sizeof(Command));
		rutaux = mytoken(path,rutaux,":");
		while(rutaux->cmdarg[i] != NULL){
			aux = built_path(cmd,rutaux->cmdarg[i]);
			
			if(access(aux,X_OK)==0){
				break;	
			}else{
				i++;
				aux=NULL;	
			}
		}
	} 	
	return aux;
}
Command*
to_give(Command * mycmd){
	char *varpath = getenv("PATH");
	char cwd[1024];
	char *rutadefinitiva =(char*) malloc(1024*sizeof(char));
	if ((getcwd(cwd, sizeof(cwd)) != NULL)&&(rutadefinitiva=execruta(cwd,mycmd->command,1))!=NULL){
  			mycmd->command=rutadefinitiva;
	}else{
	/*Si no son comandos ejecutables en el directorio actual,son en la variable path*/
		char *path2=(char*) malloc(1024*sizeof(char));	
   		strcpy(path2, varpath);
		if((rutadefinitiva=execruta(path2,mycmd->command,-1))!=NULL){
			mycmd->command=rutadefinitiva;
			}
	}
	return mycmd;
}
void 
to_fork_pipes(int fi,Command *cmd,int fo,int nc,int towait) {
	int   p[2];
	pid_t pid;
	int   fd_in = fi;
	int i;
	for(i = 0; i < nc ; i++){
    	pipe(p);
    	if ((pid = fork()) == -1){
         	exit(EXIT_FAILURE);
        
        }else if (pid == 0){
        	dup2(fd_in, 0); //change the input according to the old one 
          	if (i != nc-1){
          		dup2(p[1], 1);
          	}else if(i == (nc-1)){
				dup2(fo,1);
			}           
          		if(towait==1 && fi ==0 ){ // No hay que esperar
	    		int back = open("/dev/null",  O_RDONLY);
               	if(back < 0)
                   	fprintf(stderr,"Cannot open /dev/null\n");
               	dup2(back, 0);
            	close(back);
        	}
        	close(p[0]);
          	Command* caux=(Command*)malloc(256*sizeof(Command));
			caux = mytoken(cmd->cmdarg[i],caux," ");
			caux = to_give(caux);
           	execv(caux->command,caux->cmdarg);
			printf("Execute command failed\n");
          	exit(EXIT_FAILURE);
        }else{
          wait(NULL);
          close(p[1]);
          fd_in = p[0]; //save the input for the next command
        }
    }
}
/*Para procesar un comando sin pipes*/
//int b =flagB(line);int g =flagFI(line);int h =flagFO(line);p=pipe or not
void 
to_fork(Command *cmd,int b,int i,int o){	
	//int sts;
	int pid;

	pid = fork();
 
	switch(pid){
		case -1: // Si pid es -1 quiere decir que ha habido un error
			perror("Cannot make a child\n");
			break;
		case 0: // Cuando pid es cero quiere decir que es el proceso hijo
			if(b==1 && i ==0 ){ // No hay que esperar
	    		int back = open("/dev/null",  O_RDONLY);
                if(back < 0)
                    fprintf(stderr,"Cannot open /dev/null\n");
                dup2(back, 0);
            	close(back);
        	}
		dup2(i, 0);
		dup2(o, 1);
		//close(o);
			execv(cmd->command,cmd->cmdarg);
			//perror("ERRORRRR");
			exit(0);
		default:	
          wait(NULL);
          				// que termine el hijo
	}
}

int
flagFO(char *line){
	int flag=1;
	char *flg;
	if ((flg = strchr(line,'>'))) {
		Command* caux =(Command*)malloc(256*sizeof(Command));
		caux = mytoken(line,caux,">");
		Command* caux2 =(Command*)malloc(256*sizeof(Command));
		caux2 = mytoken(caux->cmdarg[1],caux2," ");
		if((access(caux2->cmdarg[0],F_OK)) == 0){
		    if(unlink(caux2->cmdarg[0])<0){
             	fprintf(stderr,"error al borrar el fichero");
                return -1;
            }
        flag = open(caux2->cmdarg[0], O_CREAT|O_RDWR,0660);
        caux->cmdarg[1]=NULL;
        line=caux->cmdarg[0];
        return flag;
        }
    }

	return flag;
}
/*Si != 0 hay Redirecion si 0,no hay */
int
flagFI(char *line){
	int flag=0;
	char *flg;
	
	if ((flg = strchr(line,'<'))) {
		Command* caux =(Command*)malloc(256*sizeof(Command));
		caux = mytoken(line,caux,"<");
		Command* caux2 =(Command*)malloc(256*sizeof(Command));
		caux2 = mytoken(caux->cmdarg[1],caux2," ");
		    if((access(caux2->cmdarg[0],F_OK)) == 0){
            	flag = open(caux2->cmdarg[0], O_RDONLY);
            	caux->cmdarg[1]=NULL;
            	line = caux->cmdarg[0];
        	}else{
            	fprintf(stderr,"El fichero de entrada %s no existe.\n", caux->cmdarg[1]);
            return -1;
        	}
    }
	return flag;
}
void
to_proccess_line(char *line,int b,int g,int h,int p,char *lastcd){

	Command* mycmd=(Command*)malloc(256*sizeof(Command));
	char *line_aux=(char*) malloc(1024*sizeof(char));	
   	strcpy(line_aux, line);
	mycmd=mytoken(line,mycmd," ");

		if (strcmp(mycmd->command,"cd") == 0){
			mycd(mycmd,lastcd);
		}

		if ((p==0) &&(strcmp(mycmd->command,"cd") != 0)){//NO HAY PIPES
				mycmd=to_give(mycmd);
				to_fork(mycmd,b,g,h);
			}
		if ((p!=0) && (strcmp(mycmd->command,"cd") != 0)){//HAY PIPES			//uso aux y divido line_aux por pipes..

			int o = flagFO(line_aux);//solo puede haber redirecion en el ultimo comando
			Command* myc=(Command*)malloc(256*sizeof(Command));
			myc=mytoken(line_aux,myc,"|");
			int j=0;
			while(myc->cmdarg[j]!=NULL){
				j++;
			}
			int i = flagFI(myc->command);//Solo puede haber redirecion en la entrada del primer comando
			to_fork_pipes(i,myc,o,j,b);
			close(o);
		}
	free(mycmd); 
}

int main(int argc, char *argv[]){

	char myline [MAX];
	char *line;
	char *lastcd="";

while(1){
printf ("$ ");
	line = fgets(myline,sizeof(myline),stdin);
	
	if(line != NULL && strcmp(line,"\n")!=0){
		/*Compruebo redirecciones , pipes, &, si hay asignaciones o igualaciones*/
		int o=0;
		int i=0;
		int b = flag(line,'&');
		int p = flag(line,'|');
		int a = flag(line,'=');
		int c = flag(line,'$');
		
		if(a==1)
			to_asig(line);
		if(c==1)
			line=to_sust(line);
		if (p==0){
			o = flagFO(line);
			i = flagFI(line);
		}

		to_proccess_line(line,b,i,o,p,lastcd);

	}
}
return 0;
}

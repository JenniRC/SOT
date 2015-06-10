/* 
	gcc -c -Wall -Wshadow  shell.c && gcc -o shell shell.o && ./shell
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

#define MAX 101

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
/*Si 1, hay Background, si 0 no*/
int
flagB(char *line){
	int flag=0;
	char *flg;
	if( (flg = strchr(line,'&'))) 
		flag=1;
	return flag;
}
/*Si 1, hay Asignacion si 0 no*/
int
flagASIG(char *line){
	int flag=0;
	char *flg;
	if ((flg = strchr(line,'='))) 
		flag=1;
	
	return flag;
}
/*Si 1,hay que dar valores a las variables de entorno*/ 
int
flagC(char *line){
	int flag=0;
	char *flg;
	if ((flg = strchr(line,'$'))) 
		flag=1;
	
	return flag;
}
/*Para cambiar de directorio -este es el built in que había que hacer-. Si tengo arg en
	el comando cd, voy a ese path, si no, voy a $HOME */
void
mycd(Command *cmd){
	
	if (cmd->cmdarg[1] == NULL ){//(cmd->cmdarg[0]==NULL)strcmp(mycmd->command,"cd") ==0
		chdir(getenv("HOME"));
		printf("CD A HOME\n");
	}else{
		//chdir(getenv(cmd->cmdarg[1]));
		//quit_end(cmd->cmdarg[1]);
		if(chdir(getenv(cmd->cmdarg[1]))<0){
            fprintf(stderr,"error cd $%s \n",cmd->cmdarg[1]); 
        }
		//printf("CD A %s\n", cmd->cmdarg[1]);
	}
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
       	token = strtok_r(saveptr1,sep,&saveptr1);
       	 /*si es " " al final, lo cuenta como 1,asi que cuidado ahí*/
   	}
    return cmd;
}
/*Para hacer la asignación en variables de entorno*/
void
to_asig(char *cadena){
 
    Command* var=(Command*)malloc(512*sizeof(Command));
	var = mytoken(cadena,var," ");
	if((var->cmdarg[1]!=NULL) && (var->cmdarg[2]!=NULL)){//x = y 
		if(setenv(var->cmdarg[0],var->cmdarg[2],1)<0){
       	    fprintf(stderr,"Impossible to make an assignament from %s to %s\n",var->cmdarg[0],var->cmdarg[2]);
       	}
	printf ("vars %s,%s,%s \n", var->cmdarg[0],var->cmdarg[1],var->cmdarg[2]);
	}
	if ((var->cmdarg[1]!=NULL )&& (var->cmdarg[2]==NULL)) {//x =y
		Command* aux=(Command*)malloc(512*sizeof(Command));
		char * line =malloc(128*sizeof(char));
		sprintf(line, "%s%s", var->cmdarg[0], var->cmdarg[1]);
		aux = mytoken(line,aux,"=");
		if(setenv(aux->cmdarg[0],aux->cmdarg[1],1)<0){
       	    fprintf(stderr,"Impossible2 to make an assignament from %s to %s\n",aux->cmdarg[0],aux->cmdarg[1]);
       	}
	printf ("vars2 %s,%s\n", aux->cmdarg[0],aux->cmdarg[1]);
	}
	if ((var->cmdarg[1]==NULL) && (var->cmdarg[2]==NULL)) {//x=y
		Command* aux=(Command*)malloc(512*sizeof(Command));
		aux = mytoken(var->cmdarg[0],aux,"=");
		if(setenv(aux->cmdarg[0],aux->cmdarg[1],1)<0){
       	    fprintf(stderr,"Impossible3 to make an assignament from %s to %s\n",var->cmdarg[0],var->cmdarg[1]);
       	}
	printf ("vars3 %s,%s\n", aux->cmdarg[0],aux->cmdarg[1]);
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
	printf("line=%s\n",line_aux);
	return line_aux;
	free(line);
	free(var);
	free(line_aux);
}
/*para sacar las rutas de la variable path ( se usa en el tercer caso de ejecutable)
	retorna 1 si todo ok, o -1 si no ok*/
int
rutas(char *path){
	
	Command* rutaux=(Command*)malloc(256*sizeof(Command));
	rutaux = mytoken(path,rutaux,":");
	
	if (rutaux->cmdarg[1]!=0){
		return 1;
	}else{
		return -1;
	}
	free(rutaux);
}
//*  PATH,conseguido por 'rutas',char * cmd = cmd[0] de un comando
//me devolverá la ruta dentro de PATH en la que está el ejecutable,para luego llamar a exec
char*
execruta(char *path, char * cmd,int j){
	int i=0;
	char *aux=NULL;
//	printf("%s\n",path);
//	printf("%s\n",cmd);
	if (j>0){
		aux = built_path(cmd,path);
			if (access(aux,X_OK)!=0){
				return NULL;
			}
	}
	if(j<0){
		Command* rutaux =(Command*)malloc(256*sizeof(Command));
		rutaux = mytoken(path,rutaux,":");
		//aux = built_path(cmd,rutaux->cmdarg[0]);
		while(rutaux->cmdarg[i] != NULL){
			aux = built_path(cmd,rutaux->cmdarg[i]);
			
			if(access(aux,X_OK)==0){
				break;	
			}else{
				i++;
				aux=NULL;	
			}
		}//aux=NULL;
	} 	
	//printf("doing sdfs %s\n",aux);
	return aux;
	
}
/*Para procesar un comando sin redirecciones ni pipes*/
void 
to_fork(Command *cmd,int flagB){	
	int sts=0;
	int pid;
/* Comprobamos si el hijo se creó bien */
	if(( pid = fork()) < 0){

		printf("ERROR Creacion de proceso"); 

	}else if (pid==0) {
	/*char *prog[] = { "ls", "-la", NULL };
	execv("/bin/ls", prog);
	perror("fallo en exec")*/
		printf("ruta :%s\n",cmd->command);
		printf("prog :%s\n",cmd->cmdarg[0]);
		printf("arg1 :%s\n",cmd->cmdarg[1]);
		execv(cmd->command,cmd->cmdarg);
		perror("ERRORRRR");
		exit(0);
	}else{ 
		
		if (flagB == 0) 
			pid=wait(&sts);
	}

}
//int b =flagB(line);int g =flagFI(line);int h =flagFO(line);int a = flagASIG(line);int c = flagC(line);
void
to_proccess_line(char *line,int b,int g,int h,int a,int c){
	char *varpath = getenv("PATH");
	printf("PATH :%s\n",varpath);
	Command* mycmd=(Command*)malloc(256*sizeof(Command));
	mycmd=mytoken(line,mycmd," ");
		//printf ("cmd %s,%s \n", mycmd->command,mycmd->cmdarg[0]);
		//mycmd=mytoken(line,mycmd," ");//oken(char *cadena,Command *cmd,char *sep){
		/*TODO ESTO PODRIA IR EN UNA FUNCION APARTE*/
		//printf ("cmd %s,%s \n", mycmd->command,mycmd->cmdarg[1]);
		if (strcmp(mycmd->command,"cd") ==0){
			mycd(mycmd);
				//free(mycmd);
				//printf("doing :%s\n",mycmd->command );
		}
		if (strcmp(mycmd->command,"cd") !=0){
			char cwd[1024];
			char *rutadefinitiva =(char*) malloc(1024*sizeof(char));

  				if ((getcwd(cwd, sizeof(cwd)) != NULL)&&(rutadefinitiva=execruta(cwd,mycmd->command,1))!=NULL){
  					mycmd->command=rutadefinitiva;
					to_fork(mycmd,b);
					printf("UP :\n");
				}else{
				/*Si no son comandos ejecutables en el directorio actual,son en la variable path*/
					char *path2=(char*) malloc(1024*sizeof(char));	
   					strcpy(path2, varpath);
   					//printf("After memcpy dest = %s\n", path2);
					if((rutadefinitiva=execruta(path2,mycmd->command,-1))!=NULL){
						//rutadefinitiva=execruta (path2,mycmd->command,-1);
						//printf("esta es la ruta para el exec :%s\n",rutadefinitiva );
					//varpath = getenv("PATH");
						mycmd->command=rutadefinitiva;
					//printf("PATH :%s\n",path2);
						printf("doing :%s\n",mycmd->command);
						//quit_end(mycmd->command);
						to_fork(mycmd,b);
					}
				}
				//free(rutadefinitiva);
				//free(varpath);
			}
			//free(line);
		free(mycmd);
}
/*Si != 0 hay Redirecion si 0,no hay */
int
flagFO(char *line){
	int flag=0;
	char *flg;
	int i=0;
	char *aux=(char*) malloc(1024*sizeof(char));	
   	strcpy(aux, line);
	if ((flg = strchr(aux,'>'))) {
		flg[0]='\0';
		flg++;
		Command* caux =(Command*)malloc(256*sizeof(Command));
		caux = mytoken(flg,caux," ");
		printf("file name :%s\n",caux->cmdarg[0]); //this is the file name
		    if((access(caux->cmdarg[0],F_OK)) == 0){
		    	if(unlink(caux->cmdarg[0])<0){
             	   fprintf(stderr,"error al borrar el fichero");
                return 0;
            	}
            	flag = open(caux->cmdarg[0], O_RDONLY);
        	}else{
            	flag = open(caux->cmdarg[0], O_CREAT|O_WRONLY, 0660);
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
	int i=0;
	char *aux=(char*) malloc(1024*sizeof(char));	
   	strcpy(aux, line);
	if ((flg = strchr(aux,'<'))) {
		flg[0]='\0';
		flg++;
		Command* caux =(Command*)malloc(256*sizeof(Command));
		caux = mytoken(flg,caux," ");
		printf("file name :%s\n",caux->cmdarg[0]); //this is the file name
		    if((access(caux->cmdarg[0],F_OK)) == 0){
            	flag = open(caux->cmdarg[0], O_RDONLY);
        	}else{
            	fprintf(stderr,"El fichero de entrada %s no existe.\n", caux->cmdarg[0]);
            return -1;
        }
    }//aqui empieza el nombre del archivo
	
	return flag;
}
int /*Para cuando hay pipes o redireciones*/
to_pass(int fd, int fi){
    int nw, nr;
    char  buf[2048];
    nr = read(fi, buf, sizeof(buf));
    while(nr!=0){
        if(nr<0){
            fprintf(stderr,"error r\n");
            return -1;
        }
        nw = write(fd, buf, nr);
        
        if(nw!=nr){
            fprintf(stderr,"error w\n");
            return -1;
        }
        nr = read(fi, buf, sizeof(buf));
    }
    return 1;           
}

int main(int argc, char *argv[]){

	char myline [MAX];
	char *line;

while(1){
	printf ("$ ");
	line = fgets(myline,sizeof(myline),stdin);
	
	if(line != NULL && strcmp(line,"\n")!=0){
		
		int b =flagB(line);
		printf("Background :%d\n",b);
		int g =flagFI(line);
		printf("Redirecion I :%d\n",g);
		int h =flagFO(line);
		printf("Redirecion O :%d\n",h);
		int a = flagASIG(line);
		printf("Asignacion :%d\n",a);
		int c = flagC(line);
		printf("Dando vaoloracion :%d\n",c);
		
		if(a==1)
			to_asig(line);
		if(c==1)
			line=to_sust(line);
		
		to_proccess_line(line,b,g,h,a,c);
	}

}
return 0;

}

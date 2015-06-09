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

int
static is_file(char *d_name){
	struct stat st;
	stat(d_name, &st);
	return ((st.st_mode & S_IFMT)==S_IFREG);
}
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
/*Si 1, hay Redirecion si 0 no*/
int
flagFO(char *line){
	int flag=0;
	char *flg;
	if ((flg = strchr(line,'>'))) 
		flag=1;
	
	return flag;
}
/*Si 1, hay Redirecion si 0 no*/
int
flagFI(char *line){
	int flag=0;
	char *flg;
	if ((flg = strchr(line,'<'))) 
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
	
	if (strcmp(cmd->cmdarg[1],"\n") ==0 ){//(cmd->cmdarg[0]==NULL)strcmp(mycmd->command,"cd") ==0
		chdir(getenv("HOME"));
		printf("CD A HOME\n");
		printf("____ %s\n", cmd->cmdarg[1]);

	}else{
		chdir(getenv(cmd->cmdarg[1]));
		printf("CD A %s\n", cmd->cmdarg[1]);
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
       	cmd->cmdarg[j]= token;
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
	printf("line=%s",line_aux);
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
			if(access(aux,X_OK)==0)
				break;	
				i++;
				aux=NULL;	//return aux;
			}
		}
	/*printf("doing %s\n",aux);*/
	return aux;
	free(aux);
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

int main(int argc, char *argv[]){

	char myline [MAX];
	char *line;
	char *varpath = getenv("PATH");
	printf("PATH :%s\n",varpath);
while(1){
	printf ("$ ");
	line = fgets(myline,sizeof(myline),stdin);
	
	if(line != NULL && strcmp(line,"\n")!=0){
		Command* mycmd=(Command*)malloc(256*sizeof(Command));
		int f =flagB(line);
		printf("Background :%d\n",f);
		int g =flagFI(line);
		printf("Redirecion I :%d\n",g);
		int h =flagFO(line);
		printf("Redirecion O :%d\n",h);
		int a =flagASIG(line);
		printf("Asignacion :%d\n",a);
		int c =flagC(line);
		printf("Dando vaoloracion :%d\n",c);
		
		if(a==1)
			to_asig(line);
		if(c==1)
			line=to_sust(line);

		mycmd=mytoken(line,mycmd," ");
		//printf ("cmd %s,%s \n", mycmd->command,mycmd->cmdarg[0]);
		//mycmd=mytoken(line,mycmd," ");//oken(char *cadena,Command *cmd,char *sep){
		
		//printf ("cmd %s,%s \n", mycmd->command,mycmd->cmdarg[1]);
			if (strcmp(mycmd->command,"cd") ==0){
				mycd(mycmd);
				//free(mycmd);
				//printf("doing :%s\n",mycmd->command );
			}
			if (strcmp(mycmd->command,"cd") !=0){

				char cwd[1024];
				char *rutadefinitiva =(char*) malloc(1024*sizeof(char));

  				if ((getcwd(cwd, sizeof(cwd)) != NULL)&&(execruta(cwd,mycmd->command,1))!=NULL){
					//HAREMOS COSAS
					printf("UP :\n");
				}else{
				/*Si no son comandos ejecutables en el directorio actual,son en la variable path*/
					char *path2=(char*) malloc(1024*sizeof(char));	
   					strcpy(path2, varpath);
   					//printf("After memcpy dest = %s\n", path2);
					rutadefinitiva = execruta (path2,mycmd->command,-1);
					printf("esta es la ruta para el exec :%s\n",rutadefinitiva );
					//varpath = getenv("PATH");
					//printf("PATH :%s\n",path2);

				}
				free(rutadefinitiva);
				//free(varpath);
			}
		free(mycmd);
	}

}
return 0;

}

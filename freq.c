// Jenifer Rodriguez Casas - Freq 
/*
	gcc -c -Wall -Wshadow  freq.c && gcc -o freq freq.o && ./freq prueba
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define NUM 62
#define NUN 46
int TOTAL;
	/*	My type */
struct Ch {
	char word;
	int fq;
	int init;
	int end; 
};

int option(char * arg){
  if(strcmp( arg,"-i") == 0 )
    return 0;

  return 1;
}

int
isword(char s){
  if (isalnum(s)){
    return 0;
  }else{
    return 1;
  }
}

int
pos(char s,int op ){
  int i = toascii(s);
  if(op==0){
    if(i >= 48 && i <=57){
      return i-48;
    }else if(i >= 65 && i <= 90){
      return i-55;
    }else if(i >= 97 && i <=122){
      return i-61;
    }else{
    return 0;
    }
  }else{
    if(i >= 48 && i <=57){
      return i-48;
    }else if(i >= 65 && i <= 90){
      return i-55;
    }else {
      return i-87;
    }
  }
}

void 
add(char c,struct Ch arry[],int in,int f,int op){
   int i = pos(c,op);
   arry[i].word = c;
   arry[i].fq++;
   if(in==1)    arry[i].init++;
   if(f==1)    arry[i].end++;
   TOTAL++;
}

float 
porcentaje(int fq){
  double res = (double) fq/TOTAL*100 ;
  return res;
}

void
print (struct Ch arry[]){
  int i=0;

  for(i=0;i<NUM;i++){
    if(arry[i].fq>0){
        printf("%c ", arry[i].word);
        printf(" %.2f%%", porcentaje(arry[i].fq));
        printf(" %d", arry[i].init);
        printf(" %d\n", arry[i].end);
    }
  }
}

void
toSeparate(char * word, struct Ch arry[],int op){
  //Tengo que separar palabra en letras y añadir a la lista
  int i;
  int nt=0;
  int nf=0;
  int tam = strlen(word);
  struct Ch *aux =malloc(sizeof (struct Ch));
    for (i=0; i<tam; i++) {
      aux->word = word[i];
      if(i==0)           nt=1;
      if(i==tam-1)       nf=1;
      add(aux->word,arry,nt,nf,op);
      nt=nf=0;
    }

  
}

void
quitspaces(char *c) {
  int i;
  int tam = strlen(c);
  
    for (i=0; i<tam; i++) {

        if (isword(c[i])==1){
            c[i] = '\0';       
            while(isword(c[i+1])==0){
                i++;          
            }
        }
    }
}

int
mytokenize(char *p_str, char **p_args){
  int longt, numargs,i;
  longt = strlen(p_str); //tamaño total
    
    quitspaces(p_str);
    p_args[0] = &p_str[0];
    numargs  = 1;
    i=0;
    for (i=0;numargs <= longt; i++){
        if (p_str[i]=='\0'){
            p_args[numargs] = &p_str[i+1];
          while(isword(p_str[i+1])==0){
            //printf("%c\n", p_str[i+1]);
            i++;          
          }
          if (i!=longt){
              numargs++;       
          }else{
              return --numargs ;
          }
        }
    }
    return --numargs;
}

int
files(char* filename ){
  int fd;

  if((fd = open(filename, O_RDONLY)) < 0) {
    err(1, "bad filename");
    exit(1);
  }
  
  return fd;
}
void 
init(struct Ch arry[]){
  int i=0;
  for(i=0;i<NUM;i++){
    arry[i].fq=0;
    arry[i].init=0;
    arry[i].end=0; 
  }
}

void
r(int fd, struct Ch arry[],int op){
  char  buffer[1024];
  char* args[512];
  int nr;
  for(;;){
    nr = read(fd, buffer, sizeof buffer );
      if(nr == 0)           break;
      if(nr < 0)            err(1, "bad reading");
      if(nr > 0){
        int num = mytokenize(buffer,args);
        int i = 0;
        while (i<num){
            toSeparate(args[i],arry,op);
            i++;
          }
      }
}
}

int
main(int argc,char* argv[]){

if (argc == 1){ //Entrada estandar
  struct Ch arry [NUM];
  init(arry);
  r(0,arry,1); 
  print(arry);
	exit(0);
}
int op=option(argv[1]);

if (argc != 1 && op == 0){ //De ficheros y con -i
  int i =0;
  struct Ch arry [NUM];
  init(arry);
  for ( i = 2; i < argc; i++){
    int fd = files(argv[i]);   
    r(fd,arry,op); 
    close(fd);
  }
  print(arry);
  exit(0);
}
if (argc != 1 && op == 1){ //De ficheros y sin -i
  int i =0;
  struct Ch arry [NUN];
  init(arry);
  for ( i = 1; i < argc; i++){
    int fd = files(argv[i]);   
    r(fd,arry,op); 
    close(fd);
  }
  print(arry);
}
	exit(0);
}

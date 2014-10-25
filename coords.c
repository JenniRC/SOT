/*
	gcc -c -Wall -Wshadow coords.c
	gcc -o 8.coords coords.o
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>

		/*	My type */
	typedef struct coord {
		int x;
		int y;
		struct coord* next;
	}Coord;

	typedef	struct mylist
	{
    	struct coord *first;
		struct coord *last;
		int length;
	}Mylist;

/* List Functions*/
int
static emptylist(Mylist* list){
	return list->first == NULL;
 }

void 
static initlist (Mylist *list){
		list->first = NULL;
		list->last = NULL;
		list->length = 0;
}

int 
static addlist (Mylist *list, int x , int y){
	  Coord *coord;
   /*test if we have enough memory for this new element*/
	if ((coord = (Coord *) malloc (sizeof (Coord))) == NULL)
  		return -1;

 coord->x = x;
 coord->y = y;
  if (emptylist(list)){
   	coord->next=NULL;
  	list->first = coord;
  	list->last = coord;

  }else{
  	coord->next = list->first;
  	list->first = coord;
  }
  list->length++;
  return 0;
}

void
static printlist(Mylist *list){
	Coord *coord_aux = list->first;
	while(coord_aux != NULL){
		printf("(%d,%d)\n",coord_aux->x,coord_aux->y);
		coord_aux = coord_aux->next;
	} 
}
// Coords functions //

int
static givemenum(char* c){

	return (atoi(c));
}

int 
main(int argc,char*argv[])
{

	int i,nw,nr,fd,j =0;
	int buffer [96*sizeof(Coord)];
	
	Mylist *coordlist=malloc(sizeof(Coord));
	initlist(coordlist);

	if (argc == 1){						//no hay fichero,no hay nada//
		fprintf(stderr,"error :bad command,try -w\n");
		exit(0);
	}
	
	if (argc == 2){					// hay fichero para leer//
		fd = open(argv[1], O_RDONLY);
		Coord *coord_aux=malloc(sizeof(Coord));
			if(fd < 0)
				err(1, "%s", argv[1]); 
			for(;;){
				nr = read(fd, buffer, sizeof buffer );
					if(nr == 0)						break;
									
					if(nr < 0)						err(1, "bad reading");
					if(nr > 0){
						nr = nr/(sizeof(int));

						for (j = 0 ;j < nr;j++){
							coord_aux->x=buffer[j];
							j++;
							coord_aux->y=buffer[j];
							addlist(coordlist,coord_aux->x,coord_aux->y);
						}
					printlist(coordlist);
					}
				}
		close(fd);
	exit(0);
	}

	i = strcmp(argv[1],"-w");
	if ((argc == 3) && (i==0)){//Tengo en cuenta el -w//

		int n=givemenum(argv[2]);
		Coord *coord_aux=malloc(sizeof(Coord));

		for (j = 0 ;j <= n;j++){
			coord_aux->x = j;
			coord_aux->y = j;
			nw = write(1, coord_aux,sizeof(coord_aux));
			if(nw < 0)						err(1, "bad writting");
			addlist(coordlist,j,j);
		}
		exit(0);
	}

	if ((argc == 3) && (i!=0)){//Tengo en cuenta el -w//
		fprintf(stderr,"error :bad command,try -w\n");
		exit(0);
	}
	exit(0);
}

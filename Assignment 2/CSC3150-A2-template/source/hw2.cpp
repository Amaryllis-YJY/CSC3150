#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <termios.h>
#include <fcntl.h>

#define ROW 10
#define COLUMN 50 

pthread_mutex_t mutex;
int flag;


struct Node{
	int x , y; 
	Node( int _x , int _y ) : x( _x ) , y( _y ) {}; 
	Node(){} ; 
} frog ; 


char map[ROW+10][COLUMN] ; 

// Determine a keyboard is hit or not. If yes, return 1. If not, return 0. 
int kbhit(void){
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);

	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}


void *logs_move( void *t ){
	long id=(long) t;
	/*  Move the logs  */
	if(!id)
	{
		bool check_bound=false;
		while(!flag)
		{
			usleep(100000);
			pthread_mutex_lock(&mutex);
			if(frog.x!=ROW&&frog.x!=0)
				if(frog.y<0||frog.y>=COLUMN-1)
					check_bound=true;
			if(check_bound)
			{
				flag=2;
				pthread_mutex_unlock(&mutex);
				break; 
			}		

			char log[COLUMN];
			for(int i=1;i<ROW;i++)
			{
				for(int x=0;x<COLUMN;x++)
					log[x]=map[i][x];
				
				if(i%2==0)
				{
					for(int j=0;j<COLUMN-1;j++)
						map[i][j]=log[(j+1)%(COLUMN-1)];
                    if(frog.x==i)
						frog.y--;
					
				}
				else
				{
					for(int j=0;j<COLUMN-1;j++)
						map[i][j]=log[(j-1+COLUMN-1)%(COLUMN-1)]; //Adding COLUMN-1 is to avoid some j that j-1<0
					if(frog.x==i)
						frog.y++;
				}
			}
			
			printf("\033[H\033[2J");
			for(int i=0;i<=ROW;i++)	
				puts(map[i]);
			pthread_mutex_unlock(&mutex);
		}
		pthread_exit(NULL);
	}
	/*  Check keyboard hits, to change frog's position or quit the game. */
	else
	{
		while(!flag)
		{
            //printf("YES\n");
			pthread_mutex_lock(&mutex);
            //printf("YES\n");
			if(kbhit())
			{
				char dir=getchar();
                //printf("GET\n");
				if(dir=='q'||dir=='Q')
				{
					flag=3;//quit flag
					pthread_mutex_unlock(&mutex);
					break;
				}
				if(dir=='w'||dir=='W')
				{
					if(map[frog.x-1][frog.y]==' ')
					{
						flag=2;//fail flag;
						pthread_mutex_unlock(&mutex);
					}
					else
					{
						if(frog.x==10)
							map[frog.x][frog.y]='|';
						else if(frog.x<10&&frog.x>0)
							map[frog.x][frog.y]='=';
						frog.x--;
						map[frog.x][frog.y]='0';
					}
				}
				else if(dir=='s'||dir=='S')
				{
					if(map[frog.x+1][frog.y]==' ')
					{
						flag=2;//fail flag;
						pthread_mutex_unlock(&mutex);
					}
					else
					{
						if(frog.x<10&&frog.x>0)
							map[frog.x][frog.y]='=';
						frog.x++;
						map[frog.x][frog.y]='0';
					}
				}
				else if(dir=='a'||dir=='A')
				{
					if(frog.x==10)
					{
						if(frog.y!=0)
						{
							map[frog.x][frog.y]='|';
							frog.y--;
							map[frog.x][frog.y]='0';
						}
					}
					else
					{
						if(map[frog.x][frog.y-1]==' ')
						{
							flag=2;
							pthread_mutex_unlock(&mutex);
						}
						else
						{
							map[frog.x][frog.y]='=';
							frog.y--;
							map[frog.x][frog.y]='0';
						}
					}
				}
				else if(dir=='d'||dir=='D')
				{
					if(frog.x==10)
					{
						if(frog.y!=COLUMN)
						{
							map[frog.x][frog.y]='|';
							frog.y++;
							map[frog.x][frog.y]='0';
						}
					}
					else
					{
						if(map[frog.x][frog.y+1]==' ')
						{
							flag=2;
							pthread_mutex_unlock(&mutex);
						}
						else
						{
							map[frog.x][frog.y]='=';
							frog.y++;
							map[frog.x][frog.y]='0';
						}
					}
				}

				/*  Check game's status  */
				if(frog.x==0)
				{
					flag=1;
					pthread_mutex_unlock(&mutex);
				}
				
				/*  Print the map on the screen  */
                printf("\033[H\033[2J");
                for(int i=0;i<=ROW;i++)	
                    puts(map[i]);
			}
            pthread_mutex_unlock(&mutex);
		}

        pthread_exit(NULL);
	}	
	/*  Check game's status  */
}

int main( int argc, char *argv[] ){

	// Initialize the river map and frog's starting position
	memset( map , 0, sizeof( map ) ) ;
	int i , j ; 
	for( i = 1; i < ROW; ++i ){	
		for( j = 0; j < COLUMN - 1; ++j )	
			map[i][j] = ' ' ;  
	}	

	for( j = 0; j < COLUMN - 1; ++j )	
		map[ROW][j] = map[0][j] = '|' ;

	for( j = 0; j < COLUMN - 1; ++j )	
		map[0][j] = map[0][j] = '|' ;

	frog = Node( ROW, (COLUMN-1) / 2 ) ; 
	map[frog.x][frog.y] = '0' ; 

	flag=0;
	srand(time(NULL));
	pthread_mutex_init(&mutex,NULL);

	//initializing the logs
	int head;
	for(int i=1;i<=9;i++)
	{
		head=rand()%COLUMN;
		for(int j=0;j<15;j++)
			map[i][(head+j)%(COLUMN-1)]='=';
	}

	//Print the map into screen
	for( i = 0; i <= ROW; ++i)	
		puts( map[i] );


	/*  Create pthreads for wood move and frog control.  */
	pthread_t thread_logs,thread_frog;
	int rc_logs,rc_frog;
	rc_logs=pthread_create(&thread_logs,NULL,logs_move,(void*) 0);
	rc_frog=pthread_create(&thread_frog,NULL,logs_move,(void*) 1);

	//waiting the end of threads
	pthread_join(thread_frog,NULL);
	pthread_join(thread_logs,NULL);

	/*  Display the output for user: win, lose or quit.  */
	printf("\033[H\033[2J");
	if(flag==1)
		printf("You win the game.\n");
	else if(flag==2)
		printf("You lose the game.\n");
	else
		printf("You exit the game.\n");	
	
	pthread_mutex_destroy(&mutex);
	pthread_exit(NULL);
	return 0;
}

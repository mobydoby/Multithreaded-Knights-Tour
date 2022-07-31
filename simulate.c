#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

extern int next_thread_id;
extern int max_squares;
extern int total_tours;
int m;
int n;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void printboard(int** board){
	printf("\n");
	for (int i=0; i<m;i++){
		for (int j=0; j<n; j++){
			printf("%d ", board[i][j]);
		}
		printf("\n");
	}
}

void freeboard(int** b){
	for (int i = 0; i<m; i++)
		free(b[i]);
	free(b);
}

/* inputs: board, total rows, total columns, possible new row (r) and column index (c)
 * effect: checks if the proposed position is valid
 * output: boolean
 */
int isValid(int** board, int r, int c){
	if (r<0 || r>=m) return 0;
	if (c<0 || c>=n) return 0; 
	//check if already visited
	if (board[r][c] == 1) return 0;
	return 1;
}

/* returns the number of possible moves starting at location r,c on the board 
   the moves are stored in an array moves[] where the index is the 
   following relative location

		     0   7
		   1       6
			     S
		   2       5
		   	 3   4
   moves[0|1|2|3|4|5|6|7|8]
   the 8th index is to keep track of total available moves
   if the move is available, it stores 1 into the moves array at that index
*/
int* check_moves(int** board, int r, int c){
	int* moves = (int*)calloc(9, sizeof(int));
	//start at top and go clockwise
	if (isValid(board, r-2, c-1)){moves[0] = 1; moves[8]++;}
	if (isValid(board, r-2, c+1)){moves[7] = 1; moves[8]++;}
	if (isValid(board, r-1, c+2)){moves[6] = 1; moves[8]++;}
	if (isValid(board, r+1, c+2)){moves[5] = 1; moves[8]++;} 
	if (isValid(board, r+2, c+1)){moves[4] = 1; moves[8]++;} 
	if (isValid(board, r+2, c-1)){moves[3] = 1; moves[8]++;}
	if (isValid(board, r+1, c-2)){moves[2] = 1; moves[8]++;}
	if (isValid(board, r-1, c-2)){moves[1] = 1; moves[8]++;}
	return moves;
}

typedef struct NextMove{
	int** board;
	unsigned int move_num;
	int r, c, id;
} NextMove;

//copy function for boards
int** cpyboard(int** prev){
	int** new_board = calloc(m, sizeof(int*));
  for(int i = 0; i<m; i++){
    new_board[i] = calloc(n, sizeof(int));
  }

  for(int i = 0; i<m; i++){
  	for(int j = 0; j<n; j++){
  		new_board[i][j] = prev[i][j];
  	}
  }
	return new_board;
}

/* Effect: returns a struct with attributes filled
 * Inputs: ith position of moves array (which indicates if that move is valid *see check moves for move array)
 * Output: filled out struct
 */
NextMove* loadNext(int i, int** board, int in_r, int in_c, int current_move_num, int id){
	NextMove* Next_Pos = (NextMove*)calloc(1, sizeof(NextMove));
	Next_Pos->board = cpyboard(board);	
	Next_Pos->id = id;
	//copy board over
	Next_Pos->move_num = current_move_num+1;
	switch(i){
		case 0:
			Next_Pos->r = in_r-2;
			Next_Pos->c = in_c-1;
			break;
		case 7:
			Next_Pos->r = in_r-2;
			Next_Pos->c = in_c+1;
			break;
		case 6:
			Next_Pos->r = in_r-1; 
			Next_Pos->c = in_c+2;
			break;
		case 5:
			Next_Pos->r = in_r+1; 
			Next_Pos->c = in_c+2;
			break;
		case 4:
			Next_Pos->r = in_r+2; 
			Next_Pos->c = in_c+1;
			break;
		case 3:
			Next_Pos->r = in_r+2; 
			Next_Pos->c = in_c-1;
			break;
		case 2:
			Next_Pos->r = in_r+1; 
			Next_Pos->c = in_c-2;
			break;
		case 1:
			Next_Pos->r = in_r-1;
			Next_Pos->c = in_c-2;
			break;
	}
	// printf("from functin: i:%d r:%d c:%d\n", i, in_r, in_c);
	// printf("from function after r:%d c:%d\n", Next_Pos->r, Next_Pos->c);
	return Next_Pos;
}

/* Driver function to explore -
 * 	search_t is a recursive function that also utilizes threads
 * Effects:
 * Input: 
 * Output: 
 */
void* search_t( void* arg){
	
	int rc;
	NextMove Pos = *(NextMove *) arg;

	#ifdef DEBUG
		printf("starting new search call\n");
	#endif
	int* moves = check_moves(Pos.board, Pos.r, Pos.c);
	Pos.board[Pos.r][Pos.c] = 1;

	//set name
	char* name;
	if (Pos.id==0){
		name = "MAIN";
	} else{
		char temp[64] = {};
		sprintf(temp, "%d", Pos.id);;
		char temp2[64] = "T";
		name = strcat(temp2, temp);
	}

	#ifdef DEBUG
		printboard(Pos.board);
		for (int i = 0; i<9; i++){
			printf("%d", moves[i]);
		}
		printf("\n%d\n", moves[8]);
	#endif

	//if no more moves
	if (moves[8] == 0){
		#ifdef DEBUG
			printf("starting no moves left call\n");
		#endif
		//mutex lock for global comparison
		pthread_mutex_lock(&mutex);
		int update = Pos.move_num>max_squares;
		if (update) max_squares = Pos.move_num;
		int found_tour = (Pos.move_num == m*n);
		if (found_tour) total_tours++;
		pthread_mutex_unlock(&mutex);

		if (found_tour){
			printf("%s: Sonny found a full knight's tour; incremented total_tours\n", name);
		}
		else if (update){
			printf("%s: Dead end at move #%d; updated max_squares\n", name, Pos.move_num);
		} else{
			printf("%s: Dead end at move #%d\n", name, Pos.move_num);
		}
		free(moves);
		freeboard(((NextMove*)arg)->board);
		free(arg);
		if (Pos.id != 0){
			unsigned int * y = malloc( sizeof( unsigned int ) );
		  *y = pthread_self();
		  pthread_exit( y );
		}
	}

	//multiple moves possible
	else if (moves[8] > 1){

		printf("%s: %d possible moves after move #%d; creating %d child threads...\n", name, moves[8], Pos.move_num, moves[8]);
		
		//create an array of tids 
		pthread_t tid[moves[8]];
		int tid_count[moves[8]];
		int ttracker = 0;

		#ifdef DEBUG
			printf("if more than 1 possible path:\n");
		#endif

		for (int i = 0; i<8; i++){
			
			if (moves[i] == 0) continue;

			#ifdef DEBUG
			printf("start of thread creation loop, loop num: %d\n", i);
			#endif

			NextMove* next_place;

			next_place = loadNext(i, Pos.board, Pos.r, Pos.c, Pos.move_num, Pos.id);
			pthread_mutex_lock(&mutex);
			tid_count[ttracker] = next_thread_id;
			next_place->id = next_thread_id;
			++next_thread_id;
			pthread_mutex_unlock(&mutex);
			//create another thread that executes same function
			rc = pthread_create(&tid[ttracker], NULL, search_t, next_place);
			if ( rc != 0 )
	    {
	    	// printf("from parent: %d", rc);
	      fprintf( stderr, "%s: Could not create thread (%d)\n", name, rc );
	      exit(1);
	    }



	    #ifdef NO_PARALLEL
	    	unsigned int * x;	    	
			  rc = pthread_join( tid[ttracker], (void**)&x);
			  if ( rc != 0 )
			  {
					fprintf( stderr, "%s: Could not join thread (%d)\n", name, rc );
			 	}
			  else
			  {
			  	#ifdef DEBUG
			  	printf("no parallel branch ttracker: %d, value: %d\n", ttracker, tid_count[ttracker]);
			  	#endif
			 		printf( "%s: T%d joined\n", name, tid_count[ttracker]);
				}
				
	    #endif
			ttracker++;
			#ifdef DEBUG
				printf("end of else if\n");
			#endif
		}
		#ifndef NO_PARALLEL
			//join threads here
			for (int i = 0; i<ttracker; i++){
				unsigned int * x;
				// printf("%ld", tid[i]);
				rc = pthread_join( tid[i], (void**)&x);

			  if ( rc != 0 )
			  {
					fprintf( stderr, "%s: Could not join thread (%d)\n", name, rc );
			 	}
			  else
			  {
			 		printf( "%s: T%d joined\n", name, tid_count[i]);
			 		free(x);
				}
			}
		#endif
		freeboard(((NextMove*)arg)->board);
		free(arg);
		free(moves);
	}
	// only 1 move possible
	else{
		#ifdef DEBUG
			printf("1 move possible\n");
		#endif
		//copy moves to static array
		int moves_cpy[9];
		for (int i = 0; i<9; i++){
			moves_cpy[i] = moves[i];
		}
		free(moves);
		for (int i = 0; i<8; i++){
			if (moves_cpy[i] == 1){
				NextMove* next_place;
				//load info into the next place struct
				next_place = loadNext(i, Pos.board, Pos.r, Pos.c, Pos.move_num, Pos.id);
		 		freeboard(((NextMove*)arg)->board);
				free(arg);
				search_t(next_place);
			}
		}
	}

	//never gets here
	#ifdef DEBUG
		printf("end of search\n");
	#endif
	return 0;
}

int simulate( int argc, char * argv[] ){
	//initialize input arguments
  m = atoi(argv[1]);
  n = atoi(argv[2]); 
  int r = atoi(argv[3]);
  int c = atoi(argv[4]);

  if (!(argc==5 && r<m && c<n && n>2 && m>2)){
    fprintf(stderr, "Usage: <m> <n> <r> <c>\n       where m and n > 2\n       and r<m and c<n\n");
    return EXIT_FAILURE;
  }
  
  int** start_board = calloc(m, sizeof(int*));
  for(int i = 0; i<m; i++){
    start_board[i] = calloc(n, sizeof(int));
  }

  //create a starting struct
  NextMove* start_point = (NextMove*)calloc(1, sizeof(NextMove));
  start_point->board = start_board;
  start_point->r = r;
  start_point->c = c;
  start_point->move_num = 1;
  start_point->id = 0;

  //starting 
  printf("MAIN: Solving Sonny's knight's tour problem for a %dx%d board\n", m, n);
  printf("MAIN: Sonny starts at row %d and column %d (move #1)\n", r, c);

  //main search function. 
  search_t(start_point);

  //print final result
  if(total_tours==1){
  	printf("MAIN: Search complete; found 1 possible path to achieving a full knight's tour\n");
  }
  else if (total_tours>0){
  	printf("MAIN: Search complete; found %d possible paths to achieving a full knight's tour\n", total_tours);
  }
  else{

  	printf("MAIN: Search complete; best solution(s) visited %d %s out of %d\n", max_squares, max_squares==1?"square":"squares", m*n);
  }
  return 1;
}
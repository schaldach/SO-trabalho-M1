// Código Template shm_linux_consumer.c do Viel
/**
 * Consumidor - Memória compartilhada - Linux
 * Para compilar
 *	gcc shm-posix-consumer.c -lrt
*/

#include "banco.h"
#define THREAD_NUM 1

Task taskQueue[256];
int taskCount = 0;

pthread_mutex_t mutexPrint;

pthread_mutex_t mutexBanco;

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;

Query parseQuery(Task* task){
    Query q = {
        .reg = {
            .id = 99,
            .nome = "harry"
        },
        .command = 1
    };

    return q;
}

void executeTask(Task* task){
    Query q = parseQuery(task);

    pthread_mutex_lock(&mutexPrint);
    printf("ALOOOOO");
    pthread_mutex_lock(&mutexPrint);

    FILE *fptr;
    FILE *fptr2; 
    char currentLine[DB_LINE_SIZE];
    int id;

    switch(q.command){
       case 0: // Delete
           pthread_mutex_lock(&mutexBanco);
           fptr = fopen(dbfile, "r");
           fptr2 = fopen(tempfile, "a");

           while(fgets(currentLine, DB_LINE_SIZE, fptr)){
               id = atoi(currentLine);
               if(id != q.reg.id){
                   fprintf(fptr2, "%s", currentLine);
               }
           }

           fclose(fptr);
           fclose(fptr2);
       
           remove(dbfile);
           rename(tempfile, dbfile); 
           pthread_mutex_unlock(&mutexBanco);

       break;
       
       case 1: // Insert
           pthread_mutex_lock(&mutexBanco);
           fptr = fopen(dbfile, "a+");
           bool id_exists = false;

           while(fgets(currentLine, DB_LINE_SIZE, fptr)){
               id = atoi(currentLine);
               if(id == q.reg.id){
                   id_exists = true;
               }
           }

           if(!id_exists) fprintf(fptr, "%d,%s\n", q.reg.id, q.reg.nome);
           else printf("id já existe");
           
           fclose(fptr);
           pthread_mutex_unlock(&mutexBanco);

       break;
       
       case 2: // Update
           pthread_mutex_lock(&mutexBanco);
           fptr = fopen(dbfile, "r");
           fptr2 = fopen(tempfile, "a");

           while(fgets(currentLine, DB_LINE_SIZE, fptr)){
               id = atoi(currentLine);
               if(id != q.reg.id){
                   fprintf(fptr2, "%s", currentLine);
               }
               else{
                   fprintf(fptr2, "%d,%s\n", id, q.reg.nome);
               }
           }

           fclose(fptr);
           fclose(fptr2);
   
           remove(dbfile);
           rename(tempfile, dbfile); 
           pthread_mutex_unlock(&mutexBanco);

       break;
       
       case 3: // Select
           pthread_mutex_lock(&mutexBanco);
           fptr = fopen(dbfile, "r");
           Registro reg; // poderia ser um array para permitir uma consulta maior?

           while(fgets(currentLine, DB_LINE_SIZE, fptr)){
               id = atoi(currentLine);
               if(id == q.reg.id){ // só buscando pelo id, ainda não tenho certeza como permitir buscar pelo nome também...
                   reg.id = id;

                   bool is_on_str = false;
                   int str_position = 0;
                   for(int i=0;i<DB_LINE_SIZE;i++){
                       if(is_on_str){
                           reg.nome[i-str_position] = currentLine[i];
                       }
                       else if(currentLine[i] == ','){
                           is_on_str = true;
                           str_position = i+1;
                       }
                   }
               }
           }

           printf("id:%d - nome:%s", reg.id, reg.nome);
           fclose(fptr);
           pthread_mutex_unlock(&mutexBanco);

       break;

       default: 
           printf("Comando inválido");
       break;
    }
}

void submitTask(Task task) {
    pthread_mutex_lock(&mutexQueue);
    taskQueue[taskCount] = task;
    taskCount++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}

void* startThread(void* args) {
    while (1) {
        Task task;

        pthread_mutex_lock(&mutexQueue);
        while (taskCount == 0) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        task = taskQueue[0];
        int i;
        for (i = 0; i < taskCount - 1; i++) {
            taskQueue[i] = taskQueue[i + 1];
        }
        taskCount--;
        pthread_mutex_unlock(&mutexQueue);
        executeTask(&task);
    }
}

int main(){
    int fd;
    mkfifo(myfifo, 0666);
 
    pthread_t th[THREAD_NUM];
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_mutex_init(&mutexBanco, NULL);
    pthread_cond_init(&condQueue, NULL);

    int i;
    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_create(&th[i], NULL, &startThread, NULL) != 0) {
            perror("Failed to create the thread");
        }
    } 

    char query[QUERY_SIZE];
    while(1){
        fd = open(myfifo, O_RDONLY);
		read(fd, query, QUERY_SIZE);
        printf("%s", query);

        Task t = {
            .query = query
        };
        submitTask(t);

		close(fd);
    }
      
    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }
    
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);

    return 0;
 }
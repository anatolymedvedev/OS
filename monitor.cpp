#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <Windows.h>

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
bool ready = false;

void* provide(void*)
{
    //бесконечный поток-поставщик
    while (true)
    {
        //блокировка мьютекса
        pthread_mutex_lock(&lock);
        //выполнение действий необходимых для наступления события
        if (ready)
        {
            pthread_mutex_unlock(&lock);
            continue;
        }
        ready = true;
        //уведомление потока-потребителя о наступившем событии
        printf("A new resource for consumer presented\n");
        pthread_cond_signal(&cond1);
        //освобождение мьютекса
        pthread_mutex_unlock(&lock);
        Sleep(1);
    }
    return NULL;
}


void* consume(void*)
{
    //бесконечный поток-потребитель
    while (true)
    {
        pthread_mutex_lock(&lock);
        while (!ready)
        {
            
            pthread_cond_wait(&cond1, &lock);
            printf("The consumer awoke, saw resource\n");
        }
        
        ready = false;
        printf("Resource consumed by consumer\n");
        
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main()
{
    pthread_t provider;
    pthread_t user;

    pthread_create(&provider, NULL, provide, NULL);
    pthread_create(&user, NULL, consume, NULL);

    pthread_join(provider, NULL);
    //следующий join можно убрать, т.к не достигнем его, потому что поток бесконечный
    pthread_join(user, NULL);
    return 0;
}

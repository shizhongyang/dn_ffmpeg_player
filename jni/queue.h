

typedef struct _Queue Queue;

//释放队列中元素所占用的内存
typedef void*(queue_free_func)(void *elem);

Queue *queue_init(int size);

void  queue_free(Queue* queue);

int   queue_get_next(Queue* queue,int current);

void  *queue_push(Queue *queue);

void  *queue_pop(Queue *queue);

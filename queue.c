/* Очередь запросов */
struct queue {
    struct request *front;
    struct request *rear;
};

void init_queue(struct queue *q) {
    q->front = q->rear = NULL;
}

void
enqueue(struct queue *q, struct snmp_session *session,
        struct oid *oid)
{
    struct request *new_request = (struct request *)malloc(sizeof(struct request));
    new_request->session = session;
    new_request->oid = oid;
    new_request->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = new_request;
    } else {
        q->rear->next = new_request;
        q->rear = new_request;
    }
}


struct request *
dequeue(struct queue *q)
{
    if (q->front == NULL) {
        return NULL;
    }

    struct request *temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    return temp;
}
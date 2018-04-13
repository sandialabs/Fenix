int __fenix_callback_register(void (*recover)(MPI_Comm, int, void *), void *callback_data)
{
    int error_code = FENIX_SUCCESS;
    if (__fenix_g_fenix_init_flag) {
        fenix_callback_func *fp = s_malloc(sizeof(fenix_callback_func));
        fp->x = recover;
        fp->y = callback_data;
        __fenix_callback_push( &__fenix_g_callback_list, fp);
    } else {
        error_code = FENIX_ERROR_UNINITIALIZED;
    }
    return error_code;
}

void __fenix_callback_invoke_all(int error)
{
    fenix_callback_list_t *current = __fenix_g_callback_list;
    while (current != NULL) {
        (current->callback->x)((MPI_Comm) * __fenix_g_new_world, (int) *error,
                               (void *) current->callback->y);
        current = current->next;
    }
}

void __fenix_callback_push(fenix_callback_list_t **head, fenix_callback_func *fp)
{
    fenix_callback_list_t *callback = malloc(sizeof(fenix_callback_list_t));
    callback->callback = fp;
    callback->next = *head;
    *head = callback;
}

int __fenix_callback_destroy(fenix_callback_list_t *callback_list)
{
    int error_code = FENIX_SUCCESS;

    if ( __fenix_g_fenix_init_flag ) {

        fenix_callback_list_t *current = callback_list;

        while (current != NULL) {
            fenix_callback_list_t *old;
            old = current;
            current = current->next;
            free( old->callback );
            free( old );
        }

    } else {
        error_code = FENIX_ERROR_UNINITIALIZED;
    }

    return error_code;
}


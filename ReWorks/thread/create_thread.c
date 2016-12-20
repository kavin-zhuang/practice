int pthread_create2(pthread_t *pthread , 
					char *name, 
					int priority, 
					int options, 
					int stacksize, 
					void*(*start_routine)(void*), 
					void *argu);

// name len <= 32
void flexcan_test_init(void)
{
	pthread_t pid;
	pthread_create2(&pid, "name", 100, 0, 4096, thread_entry, NULL);
}

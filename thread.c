//apt-get install gcc-multilib

#include "thread.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

# define PAGE_SIZE 4096
// thread metadata
struct thread {
	void *esp ;
	struct thread *next;
	struct thread *prev;
	void* stack_base ;
};
int WAIT_LIST_FLAG = 0 ;
struct thread *ready_list = NULL;     // ready list
struct thread *cur_thread = NULL;     // current thread
int EXIT_FLAG = 0 ;
struct thread *exit_thrd = NULL ;

void size_ready_list() {
	int size = 1 ;
	if(ready_list == NULL)
	{
		printf("list is empty\n" );
		return ;
	}
	struct thread* tmp = ready_list ;
	while(tmp->next != NULL)
	{
		size++ ;
		tmp = tmp->next ;
	}
	printf("size of list is %d \n", size);
}

// defined in context.s
void context_switch(struct thread *prev, struct thread *next);

// insert the input thread to the end of the ready list.
static void push_back(struct thread **list , struct thread *t)
{
	if(t == NULL)	return ;	// no thread to push in ready_list
	if(*list == NULL) {	// empty list
		*list = t ;//	t->next = NULL ;
		return ;
	}
	struct thread* thrd = *list ;	// tmp thread
	while (thrd->next != NULL)	// interating over threads
		thrd = thrd->next ;
	thrd->next = t ;	//	setting the threads next
	t->prev = thrd ;	//	t->next = NULL ;
}

// remove the first thread from the ready list and return to caller.
static struct thread *pop_front(struct thread **list)
{
	if(*list == NULL)	return NULL ;	// list is NULL, no thread to exit
	struct thread* thrd = *list ;
	*list = (*list)->next ;		// moving list by one
	if(*list != NULL)	// if list is not emplty then push NULL in next
		(*list)->prev = NULL ;
	if(thrd->next == NULL)	*list = NULL ;	// if there is no thread in ready_list after this pop()
	thrd->next = NULL ;
	return thrd ;	//	returning the poped thread
}

// the next thread to schedule is the first thread in the ready list.
// obtain the next thread from the ready list and call context_switch.
static void schedule()
{
	int flag_prev_isNULL = 0 ;
	struct thread *prev = cur_thread ;
	struct thread *next = pop_front(&ready_list);
	cur_thread = next;
	if(prev == NULL) {
		flag_prev_isNULL = 1 ;
		prev = malloc(sizeof(struct thread)) ;	// making a dummy threads
		prev->stack_base = malloc(PAGE_SIZE) ;
	}
	// if(cur_thread == NULL)	return ;
	if(EXIT_FLAG == 1)
		exit_thrd = prev ;
	context_switch(prev, next);	//calling the context_switch
	// printf("After context_switch\n" );
	if(EXIT_FLAG == 1)
	{
		free(exit_thrd->stack_base) ;
		free(exit_thrd) ;
		exit_thrd = NULL ;
		EXIT_FLAG = 0 ;
	}
	// if(flag_prev_isNULL = 1)
	// 	free(prev) ;
}

// push the cur_thread to the end of the ready list and call schedule
// if cur_thread is null, allocate struct thread for cur_thread
static void schedule1()
{
	if(cur_thread == NULL)
		cur_thread = malloc(sizeof(struct thread)) ;	// making a dummy threads
	push_back(&ready_list , cur_thread) ;		// pushing the current thread in ready_list
	schedule() ;
}

// allocate stack and struct thread for new thread
// save the callee-saved registers and parameters on the stack
// set the return address to the target thread
// save the stack pointer in struct thread
// push the current thread to the end of the ready list
void create_thread(func_t func, void *param)
{
	struct thread* new_thrd = malloc(sizeof(struct thread)) ;	// dynamically allocating the strucure
	unsigned* stack = malloc(PAGE_SIZE) ;	// dynamically allocating the stack
	new_thrd->stack_base = stack ;
	stack += (PAGE_SIZE/4) ;

	stack-- ;	*(func_t*)stack = param ;	// filling parameters in stack
	stack-- ;	*stack = 0 ;
	stack-- ;	*(func_t*)stack = func ;
	stack-- ;	*stack = 0 ;
	stack-- ;	*stack = 0 ;
	stack-- ;	*stack = 0 ;
	stack-- ;	*stack = 0 ;

	new_thrd->esp = stack ;	// filling thread metadata
	new_thrd->next = NULL ;
	new_thrd->prev = NULL ;
	push_back(&ready_list , new_thrd) ;	// pushing the new_thrd in ready_list
}

// call schedule1
void thread_yield()
{
	schedule1() ;
}

// call schedule
void thread_exit()
{
	EXIT_FLAG = 1 ;
	schedule() ;
}

// call schedule1 until ready_list is null
void wait_for_all()
{	// TODO : handle for the various wait_list also
	// printf("In wait_for_all\n" );
	// size_ready_list() ;
	// while(ready_list != NULL || (wait_lists != NULL && (*(struct thread**)wait_lists) != NULL) )
	// while(ready_list != NULL)
	while(ready_list != NULL || WAIT_LIST_FLAG != 0 )
	{
		schedule1() ;
		// printf("schedule\n" );
	}
	// printf("End of wait_for_all\n" );
}

void sleep(struct lock *lock)
{
	// printf("%p %p %p\n",wait_lists , &(lock->wait_list) , lock->wait_list );
	push_back((struct thread** )(&(lock->wait_list)) , cur_thread ) ;
	WAIT_LIST_FLAG ++ ;

	// struct thread** t = (void*) wait_lists ;
	// printf("%p %p %p\n",*t , (*(struct thread**)wait_lists) , lock->wait_list );
	// printf("sleep\n" );
	schedule() ;
	// printf("%p %p\n",lock->wait_list ,(struct thread*)lock->wait_list );
}

void wakeup(struct lock *lock)
{
	struct thread* thrd = pop_front((struct thread** )&(lock->wait_list)) ;
	if(thrd != NULL)
		WAIT_LIST_FLAG-- ;
	push_back(&ready_list , thrd) ;
	// printf("wakeup\n" );
	// schedule() ;
}

//apt-get install gcc-multilib

#include "thread.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#define PAGE_SIZE 4096
int num = 0 ;
// thread metadata
struct thread {
	void *esp;
	struct thread *next;
	struct thread *prev;
};

struct thread *ready_list = NULL;     // ready list
struct thread *cur_thread = NULL;     // current thread

void size_ready_list() {	// a function to know the length of ready_list
	int size = 1 ;
	if(ready_list == NULL)
	{	// list is empty
		printf("list is empty\n" );
		return ;
	}
	struct thread* tmp = ready_list ;
	while(tmp->next != NULL)
	{// iterating over the ready_list
		size++ ;
		tmp = tmp->next ;
	}
	printf("size of list is %d \n", size);
}
// defined in context.s
void context_switch(struct thread *prev, struct thread *next);

// insert the input thread to the end of the ready list.
static void push_back(struct thread *t)
{
	if(t == NULL)	return ;	// no thread to push in ready_list
	if(ready_list == NULL) {	// empty ready_list
		ready_list = t ;//	t->next = NULL ;
		return ;
	}
	struct thread* thrd = ready_list ;	// tmp thread
	while (thrd->next != NULL)	// interating over threads
		thrd = thrd->next ;
	thrd->next = t ;	//	setting the threads next
	t->prev = thrd ;	//	t->next = NULL ;
}

// remove the first thread from the ready list and return to caller.
static struct thread *pop_front()
{
	if(ready_list == NULL)	return NULL ;	// ready_list is NULL, no thread to exit
	struct thread* thrd = ready_list ;
	ready_list = ready_list->next ;		// moving ready_list by one
	if(ready_list != NULL)	// if list is not emplty then push NULL in next
		ready_list->prev = NULL ;
	if(thrd->next == NULL)	ready_list = NULL ;	// if there is no thread in ready_list after this pop()
	thrd->next = NULL ;
	return thrd ;	//	returning the poped thread
}

// the next thread to schedule is the first thread in the ready list.
// obtain the next thread from the ready list and call context_switch.
static void schedule()
{
	struct thread *prev = cur_thread ;
	struct thread *next = pop_front(ready_list);
	cur_thread = next;
	if(cur_thread == NULL)	return ;
	context_switch(prev, next);	//calling the context_switch
}

// push the cur_thread to the end of the ready list and call schedule
// if cur_thread is null, allocate struct thread for cur_thread
static void schedule1()
{
	if(cur_thread == NULL)
		cur_thread = malloc(sizeof(struct thread)) ;	// making a dummy threads
	push_back(cur_thread) ;		// pushing the current thread in ready_list
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
	unsigned* stack = (malloc(PAGE_SIZE) + PAGE_SIZE);	// dynamically allocating the stack

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
	push_back(new_thrd) ;	// pushing the new_thrd in ready_list
}

// call schedule1
void thread_yield()
{
	// printf("in thread_yield\n");
	schedule1() ;
}

// call schedule
void thread_exit()
{
	schedule() ;
}

// call schedule1 until ready_list is null
void wait_for_all()
{
	while(ready_list != NULL)
		schedule1() ;
}

#include <stdio.h>
#include <stdlib.h>

#define STACK_MAX_SIZE 256
#define IGCT 8;

typedef enum
{
	INT,
	TWIN
} ObjectType;

typedef struct sObject
{
	ObjectType type;
	unsigned char marked;

	struct sObject* next;

	union
	{
		int value;

		struct
		{
			struct sObject* head;
			struct sObject* tail;
		};
	};
} Object;

typedef struct
{
	Object* stack[STACK_MAX_SIZE];
	int stackSize;

	Object* firstObject;

	int numObjects;

	int maxObjects;
} VM;

void assert(int condition, const char* message)
{
	if (!condition)
	{
		printf("%s\n", message);
		exit(1);
	}
}

void push(VM* vm, Object* value)
{
	assert(vm->stackSize < STACK_MAX_SIZE, "Stack overflow!");
	vm->stack[vm->stackSize++] = value;
}

Object* pop(VM* vm)
{
	assert(vm->stackSize > 0, "Stack underflow");
	return vm->stack[--vm->stackSize];
}

VM* newVM()
{
	VM* vm = (VM*)malloc(sizeof(VM));
	vm->stackSize = 0;
	vm->firstObject = NULL;
	vm->numObjects = 0;
	vm->maxObjects = IGCT;
	return vm;
}

void mark(Object* object)
{
	if (object->marked)
		return;

	object->marked = 1;

	if (object->type == TWIN)
	{
		mark(object->head);
		mark(object->tail);
	}
}

void markAll(VM* vm)
{
	for (int i = 0; i < vm->stackSize; i++)
		mark(vm->stack[i]);
}

void marksweep(VM* vm)
{
	Object** object = &vm->firstObject;

	while (*object)
	{
		if (!(*object)->marked)
		{
			Object* unreached = *object;
			*object = unreached->next;
			free(unreached);

			vm->numObjects--;
		}
		else
		{
			(*object)->marked = 0;
			object = &(*object)->next;
		}
	}
}

void gc(VM* vm)
{
	int numObjects = vm->numObjects;

	markAll(vm);
	marksweep(vm);

	vm->maxObjects = vm->numObjects * 2;

	printf("Collected %d objects, %d left.\n", numObjects - vm->numObjects, vm->numObjects);
}

Object* newObject(VM* vm, ObjectType type)
{
	if (vm->numObjects == vm->maxObjects)
		gc(vm);

	Object* object = (Object*)malloc(sizeof(Object));
	object->type = type;
	object->next = vm->firstObject;
	vm->firstObject = object;
	object->marked = 0;

	vm->numObjects++;

	return object;
}

void pushInt(VM* vm, int intValue)
{
	Object* object = newObject(vm, INT);
	object->value = intValue;

	push(vm, object);
}

Object* pushTwin(VM* vm)
{
	Object* object = newObject(vm, TWIN);
	object->tail = pop(vm);
	object->head = pop(vm);

	push(vm, object);
	return object;
}

void objectPrint(Object* object)
{
	switch (object->type)
	{
	case INT:
		printf("%d", object->value);
		break;
	case TWIN:
		printf("(");
		objectPrint(object->head);
		printf(", ");
		objectPrint(object->tail);
		printf(")");
		break;
	}
}

void freeVM(VM *vm)
{
	vm->stackSize = 0;
	gc(vm);
	free(vm);
}

void preserving()
{
	printf("1: Objects on the stack are preserved.\n");
	VM* vm = newVM();
	pushInt(vm, 1);
	pushInt(vm, 2);

	gc(vm);
	assert(vm->numObjects == 2, "Should have preserved objects.");
	freeVM(vm);
}

void collections()
{
	printf("2: Unreached objects are collected.\n");
	VM* vm = newVM();
	pushInt(vm, 1);
	pushInt(vm, 2);
	pop(vm);
	pop(vm);

	gc(vm);
	assert(vm->numObjects == 0, "Should have collected objects.");
	freeVM(vm);
}

void reaching()
{
	printf("3: Reach th nested objects.\n");
	VM* vm = newVM();
	pushInt(vm, 1);
	pushInt(vm, 2);
	pushTwin(vm);
	pushInt(vm, 3);
	pushInt(vm, 4);
	pushTwin(vm);
	pushTwin(vm);

	gc(vm);
	assert(vm->numObjects == 7, "Should have reached objects");
	freeVM(vm);
}

void handling()
{
	printf("4: Handle cycles.\n");
	VM* vm = newVM();
	pushInt(vm, 1);
	pushInt(vm, 2);
	Object* a = pushTwin(vm);
	pushInt(vm, 3);
	pushInt(vm, 4);
	Object* b = pushTwin(vm);

	a->tail = b;
	b->tail = a;

	gc(vm);
	assert(vm->numObjects == 4, "Should have collected objects.");
	freeVM(vm);

}

void performance()
{
	printf("Performance of GC.\n");
	VM* vm = newVM();

	for (int i = 0; i < 1000; i++)
	{
		for (int j = 0; j < 20; j++)
			pushInt(vm, i);

		for (int k = 0; k < 20; k++)
			pop(vm);
	}
	freeVM(vm);
}


int main(void)
{
	preserving();
	collections();
	reaching();
	handling();
	performance();

	return(0);
}